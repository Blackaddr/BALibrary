/*
 * AudioEffectTremolo.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <cmath> // std::roundf
#include "AudioEffectTremolo.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr float MAX_RATE_HZ = 20.0f;

AudioEffectTremolo::AudioEffectTremolo()
: AudioStream(1, m_inputQueueArray)
{
	m_osc.setWaveform(m_waveform);
}

AudioEffectTremolo::~AudioEffectTremolo()
{
}

void AudioEffectTremolo::update(void)
{
    // Check is block is disabled
    if (m_enable == false) {
        // do not transmit or process any audio, return as quickly as possible.
        return;
    }

	audio_block_t *inputAudioBlock = receiveWritable(); // get the next block of input samples

	// Check is block is bypassed, if so either transmit input directly or create silence
	if ((m_bypass == true) || (!inputAudioBlock)) {
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

	// DO PROCESSING
	// apply modulation wave
	float *mod = m_osc.getNextVector();
	for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
		mod[i] = (mod[i] + 1.0f) / 2.0f;
		mod[i] = (1.0f - m_depth) + mod[i]*m_depth;
		mod[i] = m_volume * mod[i];
		float sample = std::roundf(mod[i] * (float)inputAudioBlock->data[i]);
		inputAudioBlock->data[i] = (int16_t)sample;
	}
	//Serial.println(String("mod: ") + mod[0]);



	//float mod = (m_osc.getNext()+1.0f)/2.0f; // value between -1.0 and +1.0f
	//float modVolume =  (1.0f - m_depth) + mod*m_depth; // value between 0 to depth
	//float finalVolume = m_volume * modVolume;

	// Set the output volume
	//gainAdjust(inputAudioBlock, inputAudioBlock, finalVolume, 1);

	transmit(inputAudioBlock);
	release(inputAudioBlock);
}

void AudioEffectTremolo::rate(float rateValue)
{
	float rateAudioBlock = rateValue * MAX_RATE_HZ;
	m_osc.setRateAudio(rateAudioBlock);
}

void AudioEffectTremolo::setWaveform(Waveform waveform)
{
	m_waveform = waveform;
	m_osc.setWaveform(waveform);
}

void AudioEffectTremolo::processMidi(int channel, int control, int value)
{

	float val = (float)value / 127.0f;

	if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectTremolo::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectTremolo::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[RATE][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[RATE][MIDI_CONTROL] == control)) {
		// Rate
		rate(val);
		Serial.println(String("AudioEffectTremolo::rate: ") + m_rate);
		return;
	}

	if ((m_midiConfig[DEPTH][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[DEPTH][MIDI_CONTROL] == control)) {
		// Depth
		depth(val);
		Serial.println(String("AudioEffectTremolo::depth: ") + m_depth);
		return;
	}

	if ((m_midiConfig[WAVEFORM][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[WAVEFORM][MIDI_CONTROL] == control)) {
		// Waveform
		if (value < 16) {
			m_waveform = Waveform::SINE;
		} else if (value < 32) {
			m_waveform = Waveform::TRIANGLE;
		} else if (value < 48) {
			m_waveform = Waveform::SQUARE;
		} else if (value < 64) {
			m_waveform = Waveform::SAWTOOTH;
		} else if (value < 80) {
			m_waveform = Waveform::RANDOM;
		}

		Serial.println(String("AudioEffectTremolo::waveform: ") + static_cast<unsigned>(m_waveform));
		return;
	}

	if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
		// Volume
		Serial.println(String("AudioEffectTremolo::volume: ") + 100*val + String("%"));
		volume(val);
		return;
	}

}

void AudioEffectTremolo::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
	if (parameter >= NUM_CONTROLS) {
		return ; // Invalid midi parameter
	}
	m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
	m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

}



