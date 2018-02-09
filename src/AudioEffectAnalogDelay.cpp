/*
 * AudioEffectAnalogDelay.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include "AudioEffectAnalogDelay.h"

namespace BAGuitar {

constexpr int MIDI_NUM_PARAMS = 4;
constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr int MIDI_BYPASS = 0;
constexpr int MIDI_DELAY = 1;
constexpr int MIDI_FEEDBACK = 2;
constexpr int MIDI_MIX = 3;

// BOSS DM-3 Filters
constexpr unsigned NUM_IIR_STAGES = 4;
constexpr unsigned IIR_COEFF_SHIFT = 2;
constexpr int32_t DEFAULT_COEFFS[5*NUM_IIR_STAGES] = {
		536870912,            988616936,            455608573,            834606945,           -482959709,
		536870912,           1031466345,            498793368,            965834205,           -467402235,
		536870912,           1105821939,            573646688,            928470657,           -448083489,
    2339,                 5093,                 2776,            302068995,              4412722
};


AudioEffectAnalogDelay::AudioEffectAnalogDelay(float maxDelay)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(maxDelay);
	m_maxDelaySamples = calcAudioSamples(maxDelay);
	m_iir  = new IirBiQuadFilterHQ(NUM_IIR_STAGES, reinterpret_cast<const int32_t *>(&DEFAULT_COEFFS), IIR_COEFF_SHIFT);
}

AudioEffectAnalogDelay::AudioEffectAnalogDelay(size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(numSamples);
	m_maxDelaySamples = numSamples;
	m_iir = new IirBiQuadFilterHQ(NUM_IIR_STAGES, reinterpret_cast<const int32_t *>(&DEFAULT_COEFFS), IIR_COEFF_SHIFT);
}

// requires preallocated memory large enough
AudioEffectAnalogDelay::AudioEffectAnalogDelay(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);
	m_maxDelaySamples = slot->size();
	m_externalMemory = true;
	m_iir = new IirBiQuadFilterHQ(NUM_IIR_STAGES, reinterpret_cast<const int32_t *>(&DEFAULT_COEFFS), IIR_COEFF_SHIFT);
}

AudioEffectAnalogDelay::~AudioEffectAnalogDelay()
{
	if (m_memory) delete m_memory;
	if (m_iir) delete m_iir;
}

void AudioEffectAnalogDelay::update(void)
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
	if (blockToRelease) release(blockToRelease);

	// BACK TO OUTPUT PROCESSING
//	audio_block_t *blockToOutput = nullptr;
//	blockToOutput = allocate();

	// copy the output data
//	if (!blockToOutput) return; // skip this time due to failure
//	// copy over data
//	m_memory->getSamples(blockToOutput, m_delaySamples);

	// Check if external DMA, if so, we need to copy out of the DMA buffer
	if (m_externalMemory && m_memory->getSlot()->isUseDma()) {
	    // Using DMA
	    m_memory->readDmaBufferContents(blockToOutput);
	}

	// perform the wet/dry mix mix
	m_postProcessing(blockToOutput, inputAudioBlock, blockToOutput);
	transmit(blockToOutput);

	release(inputAudioBlock);
	release(m_previousBlock);
	m_previousBlock = blockToOutput;
}

void AudioEffectAnalogDelay::delay(float milliseconds)
{
	size_t delaySamples = calcAudioSamples(milliseconds);

	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
		QueuePosition queuePosition = calcQueuePosition(milliseconds);
		Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();

		if (!slot) { Serial.println("ERROR: slot ptr is not valid"); }
		if (!slot->isEnabled()) {
			slot->enable();
			Serial.println("WEIRD: slot was not enabled");
		}
	}

	m_delaySamples = delaySamples;
}

void AudioEffectAnalogDelay::delay(size_t delaySamples)
{
	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
		QueuePosition queuePosition = calcQueuePosition(delaySamples);
		Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();
		if (!slot->isEnabled()) {
			slot->enable();
		}
	}
	m_delaySamples= delaySamples;
}


void AudioEffectAnalogDelay::processMidi(int channel, int control, int value)
{
	float val = (float)value / 127.0f;

	if ((m_midiConfig[MIDI_DELAY][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIDI_DELAY][MIDI_CONTROL] == control)) {
		// Delay
		m_maxDelaySamples = m_memory->getSlot()->size();
		Serial.println(String("AudioEffectAnalogDelay::delay: ") + val + String(" out of ") + m_maxDelaySamples);
		delay((size_t)(val * (float)m_maxDelaySamples));
		return;
	}

	if ((m_midiConfig[MIDI_BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIDI_BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectAnalogDelay::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectAnalogDelay::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[MIDI_FEEDBACK][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIDI_FEEDBACK][MIDI_CONTROL] == control)) {
		// Feedback
		Serial.println(String("AudioEffectAnalogDelay::feedback: ") + val);
		feedback(val);
		return;
	}

	if ((m_midiConfig[MIDI_MIX][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIDI_MIX][MIDI_CONTROL] == control)) {
		// Mix
		Serial.println(String("AudioEffectAnalogDelay::mix: ") + val);
		mix(val);
		return;
	}

}
void AudioEffectAnalogDelay::mapMidiDelay(int control, int channel)
{
	m_midiConfig[MIDI_DELAY][MIDI_CHANNEL] = channel;
	m_midiConfig[MIDI_DELAY][MIDI_CONTROL] = control;
}

void AudioEffectAnalogDelay::mapMidiBypass(int control, int channel)
{
	m_midiConfig[MIDI_BYPASS][MIDI_CHANNEL] = channel;
	m_midiConfig[MIDI_BYPASS][MIDI_CONTROL] = control;
}

void AudioEffectAnalogDelay::mapMidiFeedback(int control, int channel)
{
	m_midiConfig[MIDI_FEEDBACK][MIDI_CHANNEL] = channel;
	m_midiConfig[MIDI_FEEDBACK][MIDI_CONTROL] = control;
}

void AudioEffectAnalogDelay::mapMidiMix(int control, int channel)
{
	m_midiConfig[MIDI_MIX][MIDI_CHANNEL] = channel;
	m_midiConfig[MIDI_MIX][MIDI_CONTROL] = control;
}


void AudioEffectAnalogDelay::m_preProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
{
	if ( out && dry && wet) {
		alphaBlend(out, dry, wet, m_feedback);
	} else if (dry) {
		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}
}

void AudioEffectAnalogDelay::m_postProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
{
	if ( out && dry && wet) {
		// Simulate the LPF IIR nature of the analog systems
		m_iir->process(wet->data, wet->data, AUDIO_BLOCK_SAMPLES);
		alphaBlend(out, dry, wet, m_mix);
	} else if (dry) {
		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}
}

}


