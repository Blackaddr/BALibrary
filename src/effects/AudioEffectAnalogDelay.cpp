/*
 * AudioEffectAnalogDelay.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include "AudioEffectAnalogDelayFilters.h"
#include "AudioEffectAnalogDelay.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

AudioEffectAnalogDelay::AudioEffectAnalogDelay(float maxDelayMs)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(maxDelayMs);
	m_maxDelaySamples = calcAudioSamples(maxDelayMs);
	m_constructFilter();
}

AudioEffectAnalogDelay::AudioEffectAnalogDelay(size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(numSamples);
	m_maxDelaySamples = numSamples;
	m_constructFilter();
}

// requires preallocated memory large enough
AudioEffectAnalogDelay::AudioEffectAnalogDelay(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);

	// the delay cannot be exactly equal to the slot size, you need at least one sample so the wr/rd are not on top of each other.
	m_maxDelaySamples = (slot->size() / sizeof(int16_t))-AUDIO_BLOCK_SAMPLES;
	m_externalMemory = true;
	m_constructFilter();
}

AudioEffectAnalogDelay::~AudioEffectAnalogDelay()
{
	if (m_memory) delete m_memory;
	if (m_iir) delete m_iir;
}

// This function just sets up the default filter and coefficients
void AudioEffectAnalogDelay::m_constructFilter(void)
{
	// Use DM3 coefficients by default
	m_iir = new IirBiQuadFilterHQ(DM3_NUM_STAGES, reinterpret_cast<const int32_t *>(&DM3), DM3_COEFF_SHIFT);
}

void AudioEffectAnalogDelay::setFilterCoeffs(int numStages, const int32_t *coeffs, int coeffShift)
{
	m_iir->changeFilterCoeffs(numStages, coeffs, coeffShift);
}

void AudioEffectAnalogDelay::setFilter(Filter filter)
{
	switch(filter) {
	case Filter::WARM :
		m_iir->changeFilterCoeffs(WARM_NUM_STAGES, reinterpret_cast<const int32_t *>(&WARM), WARM_COEFF_SHIFT);
		break;
	case Filter::DARK :
		m_iir->changeFilterCoeffs(DARK_NUM_STAGES, reinterpret_cast<const int32_t *>(&DARK), DARK_COEFF_SHIFT);
		break;
	case Filter::DM3 :
	default:
		m_iir->changeFilterCoeffs(DM3_NUM_STAGES, reinterpret_cast<const int32_t *>(&DM3), DM3_COEFF_SHIFT);
		break;
	}
}

void AudioEffectAnalogDelay::update(void)
{

    // Check is block is disabled
    if (m_enable == false) {
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

	// Otherwise perform normal processing
	// In order to make use of the SPI DMA, we need to request the read from memory first,
	// then do other processing while it fills in the back.
	audio_block_t *blockToOutput = nullptr; // this will hold the output audio
    blockToOutput = allocate();
    if (!blockToOutput) {
        transmit(inputAudioBlock, 0);
        release(inputAudioBlock);
        return; // skip this update cycle due to failure
    }

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
	m_postProcessing(blockToOutput, inputAudioBlock, blockToOutput);
	transmit(blockToOutput);

	release(inputAudioBlock);
	if (m_previousBlock) { release(m_previousBlock); }
	m_previousBlock = blockToOutput;

	if (m_blockToRelease) release(m_blockToRelease);
	m_blockToRelease = blockToRelease;
}

void AudioEffectAnalogDelay::delay(float milliseconds)
{
	size_t delaySamples = calcAudioSamples(milliseconds);

	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); return; }

	if (!m_externalMemory) {
		// internal memory
	    m_maxDelaySamples = m_memory->getMaxDelaySamples();
		//QueuePosition queuePosition = calcQueuePosition(milliseconds);
		//Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		ExtMemSlot *slot = m_memory->getSlot();
		m_maxDelaySamples = (slot->size() / sizeof(int16_t))-AUDIO_BLOCK_SAMPLES;

		if (!slot) { Serial.println("ERROR: slot ptr is not valid"); }
		if (!slot->isEnabled()) {
			slot->enable();
			Serial.println("WEIRD: slot was not enabled");
		}
	}

    if (delaySamples > m_maxDelaySamples) {
        // this exceeds max delay value, limit it.
        delaySamples = m_maxDelaySamples;
    }
	m_delaySamples = delaySamples;
}

void AudioEffectAnalogDelay::delay(size_t delaySamples)
{
	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
	    m_maxDelaySamples = m_memory->getMaxDelaySamples();
		//QueuePosition queuePosition = calcQueuePosition(delaySamples);
		//Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		//Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();
		m_maxDelaySamples = (slot->size() / sizeof(int16_t))-AUDIO_BLOCK_SAMPLES;
		if (!slot->isEnabled()) {
			slot->enable();
		}
	}

	if (delaySamples > m_maxDelaySamples) {
        // this exceeds max delay value, limit it.
        delaySamples = m_maxDelaySamples;
    }
    m_delaySamples = delaySamples;
}

void AudioEffectAnalogDelay::delayFractionMax(float delayFraction)
{
	size_t delaySamples = static_cast<size_t>(static_cast<float>(m_memory->getMaxDelaySamples()) * delayFraction);

	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
	    m_maxDelaySamples = m_memory->getMaxDelaySamples();
		//QueuePosition queuePosition = calcQueuePosition(delaySamples);
		//Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		//Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();
		m_maxDelaySamples = (slot->size() / sizeof(int16_t))-AUDIO_BLOCK_SAMPLES;
		if (!slot->isEnabled()) {
			slot->enable();
		}
	}

	if (delaySamples > m_maxDelaySamples) {
        // this exceeds max delay value, limit it.
        delaySamples = m_maxDelaySamples;
    }
    m_delaySamples = delaySamples;
}

void AudioEffectAnalogDelay::m_preProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
{
	if ( out && dry && wet) {
		alphaBlend(out, dry, wet, m_feedback);
		m_iir->process(out->data, out->data, AUDIO_BLOCK_SAMPLES);
	} else if (dry) {
		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}
}

void AudioEffectAnalogDelay::m_postProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
{
	if (!out) return; // no valid output buffer

	if ( out && dry && wet) {
		// Simulate the LPF IIR nature of the analog systems
		//m_iir->process(wet->data, wet->data, AUDIO_BLOCK_SAMPLES);
		alphaBlend(out, dry, wet, m_mix);
	} else if (dry) {
		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
	}
	// Set the output volume
	gainAdjust(out, out, m_volume);

}


void AudioEffectAnalogDelay::processMidi(int channel, int control, int value)
{

	float val = (float)value / 127.0f;

	if ((m_midiConfig[DELAY][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[DELAY][MIDI_CONTROL] == control)) {
		// Delay
		if (m_externalMemory) { m_maxDelaySamples = (m_memory->getSlot()->size() / sizeof(int16_t))-AUDIO_BLOCK_SAMPLES; }
		size_t delayVal = (size_t)(val * (float)(m_maxDelaySamples));
		delay(delayVal);
		Serial.println(String("AudioEffectAnalogDelay::delay (ms): ") + calcAudioTimeMs(delayVal)
				+ String(" (samples): ") + delayVal + String(" out of ") + m_maxDelaySamples);
		return;
	}

	if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectAnalogDelay::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectAnalogDelay::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[FEEDBACK][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[FEEDBACK][MIDI_CONTROL] == control)) {
		// Feedback
		Serial.println(String("AudioEffectAnalogDelay::feedback: ") + 100*val + String("%"));
		feedback(val);
		return;
	}

	if ((m_midiConfig[MIX][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIX][MIDI_CONTROL] == control)) {
		// Mix
		Serial.println(String("AudioEffectAnalogDelay::mix: Dry: ") + 100*(1-val) + String("% Wet: ") + 100*val );
		mix(val);
		return;
	}

	if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
		// Volume
		Serial.println(String("AudioEffectAnalogDelay::volume: ") + 100*val + String("%"));
		volume(val);
		return;
	}

}

void AudioEffectAnalogDelay::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
	if (parameter >= NUM_CONTROLS) {
		return ; // Invalid midi parameter
	}
	m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
	m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

}


