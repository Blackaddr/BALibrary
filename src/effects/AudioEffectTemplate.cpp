/*
 * AudioEffectTemplate.cpp
 *
 *  Created on: June 20, 2019
 *      Author: slascos
 */
#include <cmath> // std::roundf
#include "AudioEffectTemplate.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

AudioEffectTemplate::AudioEffectTemplate()
: AudioStream(1, m_inputQueueArray)
{
}

AudioEffectTemplate::~AudioEffectTemplate()
{
}

void AudioEffectTemplate::update(void)
{
	audio_block_t *inputAudioBlock = receiveWritable(); // get the next block of input samples

	// Check is block is disabled
	if (m_enable == false) {
		// do not transmit or process any audio, return as quickly as possible.
		if (inputAudioBlock) release(inputAudioBlock);
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

	// DO PROCESSING HERE

	transmit(inputAudioBlock);
	release(inputAudioBlock);
}

void AudioEffectTemplate::processMidi(int channel, int control, int value)
{

	float val = (float)value / 127.0f;

	if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectTemplate::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectTemplate::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
		// Volume
		Serial.println(String("AudioEffectTemplate::volume: ") + 100*val + String("%"));
		volume(val);
		return;
	}

}

void AudioEffectTemplate::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
	if (parameter >= NUM_CONTROLS) {
		return ; // Invalid midi parameter
	}
	m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
	m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

}



