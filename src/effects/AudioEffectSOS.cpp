/*
 * AudioEffectSOS.cpp
 *
 *  Created on: Apr 14, 2018
 *      Author: blackaddr
 */

#include "AudioEffectSOS.h"
#include "LibBasicFunctions.h"

using namespace BAGuitar;
using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr float MAX_GATE_OPEN_TIME_MS = 3000.0f;
constexpr float MAX_GATE_CLOSE_TIME_MS = 3000.0f;

AudioEffectSOS::AudioEffectSOS(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(slot);
    m_maxDelaySamples = (slot->size() / sizeof(int16_t));
    m_delaySamples = m_maxDelaySamples;
    m_externalMemory = true;
}

AudioEffectSOS::~AudioEffectSOS()
{

}

void AudioEffectSOS::update(void)
{
    audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

    // Check is block is disabled
    if (m_enable == false) {
        // do not transmit or process any audio, return as quickly as possible.
        if (inputAudioBlock) release(inputAudioBlock);

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

    // Check is block is bypassed, if so either transmit input directly or create silence
    if (m_bypass == true) {
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

    // Otherwise perform normal processing
    // In order to make use of the SPI DMA, we need to request the read from memory first,
    // then do other processing while it fills in the back.
    audio_block_t *blockToOutput = nullptr; // this will hold the output audio
    blockToOutput = allocate();
    if (!blockToOutput) return; // skip this update cycle due to failure

    // get the data. If using external memory with DMA, this won't be filled until
    // later.
    m_memory->getSamples(blockToOutput, m_delaySamples);

    // If using DMA, we need something else to do while that read executes, so
    // move on to input preprocessing

    // Preprocessing
    audio_block_t *preProcessed = allocate();
    // mix the input with the feedback path in the pre-processing stage
    m_preProcessing(preProcessed, inputAudioBlock, m_previousBlock);

    // consider doing the BBD post processing here to use up more time while waiting
    // for the read data to come back
    audio_block_t *blockToRelease = m_memory->addBlock(preProcessed);


    // BACK TO OUTPUT PROCESSING
    // Check if external DMA, if so, we need to be sure the read is completed
    if (m_externalMemory && m_memory->getSlot()->isUseDma()) {
        // Using DMA
        while (m_memory->getSlot()->isReadBusy()) {}
    }

    // perform the wet/dry mix mix
    //m_postProcessing(blockToOutput, inputAudioBlock, blockToOutput);
    transmit(blockToOutput);

    release(inputAudioBlock);
    release(m_previousBlock);
    m_previousBlock = blockToOutput;

    if (m_blockToRelease) release(m_blockToRelease);
    m_blockToRelease = blockToRelease;
}


void AudioEffectSOS::gateOpenTime(float milliseconds)
{
    // TODO - change the paramter automation to an automation sequence
    m_openTimeMs = milliseconds;
    //m_inputGateAuto.reconfigure();
}

void AudioEffectSOS::gateCloseTime(float milliseconds)
{
    m_closeTimeMs = milliseconds;
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
        Serial.println(String("AudioEffectSOS::gate close time (ms): ") + m_openTimeMs);
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
        // The gate is trigged by any value
        m_inputGateAuto.trigger();
        Serial.println(String("AudioEffectSOS::Gate Triggered!"));
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
        // then mix together.
        float gateVol = m_inputGateAuto.getNextValue();
        audio_block_t tempAudioBuffer;
        gainAdjust(out, input, gateVol, 0); // last paremeter is coeff shift, 0 bits
        gainAdjust(&tempAudioBuffer, delayedSignal, m_feedback, 0); // last paremeter is coeff shift, 0 bits
        combine(out, out, &tempAudioBuffer);
    } else if (input) {
        memcpy(out->data, input->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
    }
}


} // namespace BAEffects



