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

constexpr int MIDI_ENABLE = 0;
constexpr int MIDI_DELAY = 1;
constexpr int MIDI_FEEDBACK = 2;
constexpr int MIDI_MIX = 3;


AudioEffectAnalogDelay::AudioEffectAnalogDelay(float maxDelay)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(maxDelay);
	m_maxDelaySamples = calcAudioSamples(maxDelay);
}

AudioEffectAnalogDelay::AudioEffectAnalogDelay(size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(numSamples);
	m_maxDelaySamples = numSamples;
}

// requires preallocated memory large enough
AudioEffectAnalogDelay::AudioEffectAnalogDelay(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);
	m_maxDelaySamples = slot->size();
	m_externalMemory = true;
}

AudioEffectAnalogDelay::~AudioEffectAnalogDelay()
{
	if (m_memory) delete m_memory;
}

void AudioEffectAnalogDelay::update(void)
{
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

	if (!inputAudioBlock) {
		// create silence
		inputAudioBlock = allocate();
		if (!inputAudioBlock) { return; } // failed to allocate
		else {
			clearAudioBlock(inputAudioBlock);
		}
	}

	if (m_enable == false) {
		// release all held memory resources
		transmit(inputAudioBlock);
		release(inputAudioBlock); inputAudioBlock = nullptr;

		if (m_previousBlock) {
			release(m_previousBlock); m_previousBlock = nullptr;
		}
		if (!m_externalMemory) {
			while (m_memory->getRingBuffer()->size() > 0) {
				audio_block_t *releaseBlock = m_memory->getRingBuffer()->front();
				m_memory->getRingBuffer()->pop_front();
				if (releaseBlock) release(releaseBlock);
			}
		}
	}

	if (m_callCount < 1024) {
		if (inputAudioBlock) {
			transmit(inputAudioBlock, 0);
			release(inputAudioBlock);
		}
		m_callCount++; return;
	}


	m_callCount++;
	//Serial.println(String("AudioEffectAnalgDelay::update: ") + m_callCount);

	// Preprocessing
	audio_block_t *preProcessed = allocate();
	m_preProcessing(preProcessed, inputAudioBlock, m_previousBlock);

	audio_block_t *blockToRelease = m_memory->addBlock(preProcessed);
	if (blockToRelease) release(blockToRelease);

//	if (inputAudioBlock) {
//		transmit(inputAudioBlock, 0);
//		release(inputAudioBlock);
//	}
//	return;

	// OUTPUT PROCESSING
	audio_block_t *blockToOutput = nullptr;
	blockToOutput = allocate();

	// copy the output data
	if (!blockToOutput) return; // skip this time due to failure
	// copy over data
	m_memory->getSamples(blockToOutput, m_delaySamples);
	// perform the mix
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

	if ((m_midiConfig[MIDI_ENABLE][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIDI_ENABLE][MIDI_CONTROL] == control)) {
		// Enable
		if (value >= 65) { enable(); Serial.println(String("AudioEffectAnalogDelay::enable: ON")); }
		else { disable(); Serial.println(String("AudioEffectAnalogDelay::enable: OFF")); }
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

void AudioEffectAnalogDelay::mapMidiEnable(int control, int channel)
{
	m_midiConfig[MIDI_ENABLE][MIDI_CHANNEL] = channel;
	m_midiConfig[MIDI_ENABLE][MIDI_CONTROL] = control;
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
		alphaBlend(out, dry, wet, m_mix);
	} else if (dry) {
		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}
}

}


