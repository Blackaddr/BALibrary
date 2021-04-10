/*
 * AudioEffectSOS.cpp
 *
 *  Created on: Apr 14, 2018
 *      Author: blackaddr
 */

#include "AudioEffectSOS.h"
#include "LibBasicFunctions.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr float MAX_GATE_OPEN_TIME_MS = 3000.0f;
constexpr float MAX_GATE_CLOSE_TIME_MS = 1000.0f;

constexpr int GATE_OPEN_STAGE = 0;
constexpr int GATE_HOLD_STAGE = 1;
constexpr int GATE_CLOSE_STAGE = 2;

AudioEffectSOS::AudioEffectSOS(float maxDelayMs)
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(maxDelayMs);
    m_maxDelaySamples = calcAudioSamples(maxDelayMs);
    m_externalMemory = false;
}

AudioEffectSOS::AudioEffectSOS(size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(numSamples);
    m_maxDelaySamples = numSamples;
    m_externalMemory = false;
}

AudioEffectSOS::AudioEffectSOS(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(slot);
    m_externalMemory = true;
}

AudioEffectSOS::~AudioEffectSOS()
{
    if (m_memory) delete m_memory;
}

void AudioEffectSOS::setGateLedGpio(int pinId)
{
    m_gateLedPinId = pinId;
    pinMode(static_cast<uint8_t>(m_gateLedPinId), OUTPUT);
}

void AudioEffectSOS::enable(void)
{
    m_enable = true;
    if (m_externalMemory) {
        // Because we hold the previous output buffer for an update cycle, the maximum delay is actually
        // 1 audio block mess then the max delay returnable from the memory.
        m_maxDelaySamples = m_memory->getMaxDelaySamples() - AUDIO_BLOCK_SAMPLES;
        Serial.println(String("SOS Enabled with delay length ") + m_maxDelaySamples + String(" samples"));
    }
    m_delaySamples = m_maxDelaySamples;
    m_inputGateAuto.setupParameter(GATE_OPEN_STAGE, 0.0f, 1.0f, 1000.0f, ParameterAutomation<float>::Function::EXPONENTIAL);
    m_inputGateAuto.setupParameter(GATE_HOLD_STAGE, 1.0f, 1.0f, m_delaySamples, ParameterAutomation<float>::Function::HOLD);
    m_inputGateAuto.setupParameter(GATE_CLOSE_STAGE, 1.0f, 0.0f, 1000.0f, ParameterAutomation<float>::Function::EXPONENTIAL);

    m_clearFeedbackAuto.setupParameter(GATE_OPEN_STAGE, 1.0f, 0.0f, 1000.0f, ParameterAutomation<float>::Function::EXPONENTIAL);
    m_clearFeedbackAuto.setupParameter(GATE_HOLD_STAGE, 0.0f, 0.0f, m_delaySamples, ParameterAutomation<float>::Function::HOLD);
    m_clearFeedbackAuto.setupParameter(GATE_CLOSE_STAGE, 0.0f, 1.0f, 1000.0f, ParameterAutomation<float>::Function::EXPONENTIAL);
}

void AudioEffectSOS::update(void)
{

    // Check is block is disabled
    if (m_enable == false) {
        // do not transmit or process any audio, return as quickly as possible.
        // release all held memory resources
        if (m_previousBlock) {
            release(m_previousBlock); m_previousBlock = nullptr;
        }
        if (!m_externalMemory) {
            // when using internal memory we have to release all references in the ring buffer
            while (m_memory->getRingBuffer()->size() > 0) {
                audio_block_t *releaseBlock = m_memory->getRingBuffer()->front();
                m_memory->getRingBuffer()->pop_front();
                if (releaseBlock) release(releaseBlock);
            }
        }
        return;
    }

    audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

    // Check is block is bypassed, if so either transmit input directly or create silence
    if ( (m_bypass == true) || (!inputAudioBlock) ) {
        // transmit the input directly
        if (!inputAudioBlock) {
            // create silence
            inputAudioBlock = allocate();
            if (!inputAudioBlock) { return; } // failed to allocate
            else {
                clearAudioBlock(inputAudioBlock);
            }
        }
        transmit(inputAudioBlock, 0);
        release(inputAudioBlock);
        return;
    }

    if (!inputAudioBlock) return;

    // Otherwise perform normal processing
    // In order to make use of the SPI DMA, we need to request the read from memory first,
    // then do other processing while it fills in the back.
    audio_block_t *blockToOutput = nullptr; // this will hold the output audio
    blockToOutput = allocate();
    if (!blockToOutput) return; // skip this update cycle due to failure

    // get the data. If using external memory with DMA, this won't be filled until
    // later.
    m_memory->getSamples(blockToOutput, m_delaySamples);
    //Serial.println(String("Delay samples:") + m_delaySamples);
    //Serial.println(String("Use dma: ") + m_memory->getSlot()->isUseDma());

    // If using DMA, we need something else to do while that read executes, so
    // move on to input preprocessing

    // Preprocessing
    audio_block_t *preProcessed = allocate();
    // mix the input with the feedback path in the pre-processing stage
    m_preProcessing(preProcessed, inputAudioBlock, m_previousBlock);

    audio_block_t *blockToRelease = m_memory->addBlock(preProcessed);


    // BACK TO OUTPUT PROCESSING
    // Check if external DMA, if so, we need to be sure the read is completed
    if (m_externalMemory && m_memory->getSlot()->isUseDma()) {
        // Using DMA
        while (m_memory->getSlot()->isReadBusy()) {}
    }

    // perform the wet/dry mix mix
    m_postProcessing(blockToOutput, blockToOutput);
    transmit(blockToOutput);

    release(inputAudioBlock);

    if (m_previousBlock) { release(m_previousBlock); }
    m_previousBlock = blockToOutput;

    if (m_blockToRelease == m_previousBlock) {
        Serial.println("ERROR: POINTER COLLISION");
    }

    if (m_blockToRelease) { release(m_blockToRelease); }
    m_blockToRelease = blockToRelease;
}


void AudioEffectSOS::gateOpenTime(float milliseconds)
{
    // TODO - change the paramter automation to an automation sequence
    m_openTimeMs = milliseconds;
    m_inputGateAuto.setupParameter(GATE_OPEN_STAGE, 0.0f, 1.0f, m_openTimeMs, ParameterAutomation<float>::Function::EXPONENTIAL);
}

void AudioEffectSOS::gateCloseTime(float milliseconds)
{
    m_closeTimeMs = milliseconds;
    m_inputGateAuto.setupParameter(GATE_CLOSE_STAGE, 1.0f, 0.0f, m_closeTimeMs, ParameterAutomation<float>::Function::EXPONENTIAL);
}

////////////////////////////////////////////////////////////////////////
// MIDI PROCESSING
////////////////////////////////////////////////////////////////////////
void AudioEffectSOS::processMidi(int channel, int control, int value)
{

    float val = (float)value / 127.0f;

    if ((m_midiConfig[GATE_OPEN_TIME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[GATE_OPEN_TIME][MIDI_CONTROL] == control)) {
        // Gate Open Time
        gateOpenTime(val * MAX_GATE_OPEN_TIME_MS);
        Serial.println(String("AudioEffectSOS::gate open time (ms): ") + m_openTimeMs);
        return;
    }

    if ((m_midiConfig[GATE_CLOSE_TIME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[GATE_CLOSE_TIME][MIDI_CONTROL] == control)) {
        // Gate Close Time
        gateCloseTime(val * MAX_GATE_CLOSE_TIME_MS);
        Serial.println(String("AudioEffectSOS::gate close time (ms): ") + m_closeTimeMs);
        return;
    }

    if ((m_midiConfig[FEEDBACK][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[FEEDBACK][MIDI_CONTROL] == control)) {
        // Feedback
        Serial.println(String("AudioEffectSOS::feedback: ") + 100*val + String("%"));
        feedback(val);
        return;
    }

    if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
        // Volume
        Serial.println(String("AudioEffectSOS::volume: ") + 100*val + String("%"));
        volume(val);
        return;
    }

    if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
        // Bypass
        if (value >= 65) { bypass(false); Serial.println(String("AudioEffectSOS::not bypassed -> ON") + value); }
        else { bypass(true); Serial.println(String("AudioEffectSOS::bypassed -> OFF") + value); }
        return;
    }

    if ((m_midiConfig[GATE_TRIGGER][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[GATE_TRIGGER][MIDI_CONTROL] == control)) {
        // The gate is triggered by any value
        Serial.println(String("AudioEffectSOS::Gate Triggered!"));
        m_inputGateAuto.trigger();
        return;
    }

    if ((m_midiConfig[CLEAR_FEEDBACK_TRIGGER][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[CLEAR_FEEDBACK_TRIGGER][MIDI_CONTROL] == control)) {
        // The gate is triggered by any value
        Serial.println(String("AudioEffectSOS::Clear feedback Triggered!"));
        m_clearFeedbackAuto.trigger();
        return;
    }
}

void AudioEffectSOS::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
    if (parameter >= NUM_CONTROLS) {
        return ; // Invalid midi parameter
    }
    m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
    m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

//////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////
void AudioEffectSOS::m_preProcessing (audio_block_t *out, audio_block_t *input, audio_block_t *delayedSignal)
{
    if ( out && input && delayedSignal) {
        // Multiply the input signal by the automated gate value
        // Multiply the delayed signal by the user set feedback value
        // Then combine the two

        float gateVol     = m_inputGateAuto.getNextValue();
        float feedbackAdjust = m_clearFeedbackAuto.getNextValue();
        audio_block_t tempAudioBuffer;

        gainAdjust(out, input, gateVol, 0); // last paremeter is coeff shift, 0 bits
        gainAdjust(&tempAudioBuffer, delayedSignal, m_feedback*feedbackAdjust, 0); // last parameter is coeff shift, 0 bits
        combine(out, out, &tempAudioBuffer);

    } else if (input) {
        memcpy(out->data, input->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
    }

    // Update the gate LED
    if (m_gateLedPinId >= 0) {
        if (m_inputGateAuto.isFinished() && m_clearFeedbackAuto.isFinished()) {
            digitalWriteFast(m_gateLedPinId, 0x0);
        } else {
            digitalWriteFast(m_gateLedPinId, 0x1);
        }
    }
}

void AudioEffectSOS::m_postProcessing(audio_block_t *out, audio_block_t *in)
{
    gainAdjust(out, out, m_volume, 0);
}


} // namespace BAEffects



