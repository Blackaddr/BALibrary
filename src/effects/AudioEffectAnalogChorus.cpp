/*
 * AudioEffectAnalogChorus.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include "AudioEffectAnalogChorusFilters.h"
#include "AudioEffectAnalogChorus.h"

using namespace BALibrary;

//#define INTERPOLATED_DELAY Uncomment this line to test the inteprolated delay which adds 1/10th of a sample

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

AudioEffectAnalogChorus::AudioEffectAnalogChorus()
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(m_DEFAULT_DELAY_MS + m_DELAY_RANGE);
    m_maxDelaySamples = calcAudioSamples(m_DEFAULT_DELAY_MS + m_DELAY_RANGE);
    m_constructFilter();
}

// requires preallocated memory large enough
AudioEffectAnalogChorus::AudioEffectAnalogChorus(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);
	m_maxDelaySamples = (slot->size() / sizeof(int16_t));
	m_externalMemory = true;
	m_constructFilter();
}

AudioEffectAnalogChorus::~AudioEffectAnalogChorus()
{
	if (m_memory) delete m_memory;
	if (m_iir) delete m_iir;
}

// This function just sets up the default filter and coefficients
void AudioEffectAnalogChorus::m_constructFilter(void)
{
	// Use CE2 coefficients by default
	m_iir = new IirBiQuadFilterHQ(CE2_NUM_STAGES, reinterpret_cast<const int32_t *>(&CE2), CE2_COEFF_SHIFT);
}

void AudioEffectAnalogChorus::setFilterCoeffs(int numStages, const int32_t *coeffs, int coeffShift)
{
	m_iir->changeFilterCoeffs(numStages, coeffs, coeffShift);
}

void AudioEffectAnalogChorus::setFilter(Filter filter)
{
	switch(filter) {
	case Filter::WARM :
		m_iir->changeFilterCoeffs(WARM_NUM_STAGES, reinterpret_cast<const int32_t *>(&WARM), WARM_COEFF_SHIFT);
		break;
	case Filter::DARK :
		m_iir->changeFilterCoeffs(DARK_NUM_STAGES, reinterpret_cast<const int32_t *>(&DARK), DARK_COEFF_SHIFT);
		break;
	case Filter::CE2 :
	default:
		m_iir->changeFilterCoeffs(CE2_NUM_STAGES, reinterpret_cast<const int32_t *>(&CE2), CE2_COEFF_SHIFT);
		break;
	}
}

void AudioEffectAnalogChorus::update(void)
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
#ifdef INTERPOLATED_DELAY
    int16_t extendedBuffer[AUDIO_BLOCK_SAMPLES+1]; // need one more sample for intepolating between 128th and 129th (last sample)
    m_memory->getSamples(extendedBuffer, m_delaySamples, AUDIO_BLOCK_SAMPLES+1);
#else
    m_memory->getSamples(blockToOutput, m_delaySamples);
#endif


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

#ifdef INTERPOLATED_DELAY
	// TODO: partial delay testing
	// extendedBuffer is oversized
	//memcpy(blockToOutput->data, &extendedBuffer[1], sizeof(int16_t)*AUDIO_BLOCK_SAMPLES);
	m_memory->interpolateDelay(extendedBuffer, blockToOutput->data, 0.1f, AUDIO_BLOCK_SAMPLES);
#endif

	// perform the wet/dry mix mix
	m_postProcessing(blockToOutput, inputAudioBlock, blockToOutput);
	transmit(blockToOutput);

	release(inputAudioBlock);
	release(m_previousBlock);
	m_previousBlock = blockToOutput;

//	if (m_externalMemory && m_memory->getSlot()->isUseDma()) {
//	    // Using DMA
//		if (m_blockToRelease) release(m_blockToRelease);
//		m_blockToRelease = blockToRelease;
//	}

	if (m_blockToRelease) release(m_blockToRelease);
	m_blockToRelease = blockToRelease;
}

void AudioEffectAnalogChorus::m_preProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
{
    memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
//	if ( out && dry && wet) {
//		alphaBlend(out, dry, wet, m_feedback);
//		m_iir->process(out->data, out->data, AUDIO_BLOCK_SAMPLES);
//	} else if (dry) {
//		memcpy(out->data, dry->data, sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
//	}
}

void AudioEffectAnalogChorus::m_postProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet)
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
	gainAdjust(out, out, m_volume, 1);

}

void AudioEffectAnalogChorus::setDelayConfig(float averageDelayMs, float delayRangeMs)
{
    size_t delaySamples = calcAudioSamples(averageDelayMs + delayRangeMs);

    if (delaySamples > m_memory->getMaxDelaySamples()) {
        // this exceeds max delay value, limit it.
        delaySamples = m_memory->getMaxDelaySamples();
    }

    if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

    if (!m_externalMemory) {
        // internal memory
        // Do nothing
    } else {
        // external memory
        ExtMemSlot *slot = m_memory->getSlot();

        if (!slot) { Serial.println("ERROR: slot ptr is not valid"); }
        if (!slot->isEnabled()) {
            slot->enable();
            Serial.println("WEIRD: slot was not enabled");
        }
    }
}

void AudioEffectAnalogChorus::setDelayConfig(size_t averageDelayNumSamples, size_t delayRangeNumSamples)
{
    size_t delaySamples = averageDelayNumSamples + delayRangeNumSamples;

    if (delaySamples > m_memory->getMaxDelaySamples()) {
        // this exceeds max delay value, limit it.
        delaySamples = m_memory->getMaxDelaySamples();
    }

    if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

    if (!m_externalMemory) {
        // internal memory
        // Do nothing
    } else {
        // external memory
        ExtMemSlot *slot = m_memory->getSlot();

        if (!slot) { Serial.println("ERROR: slot ptr is not valid"); }
        if (!slot->isEnabled()) {
            slot->enable();
            Serial.println("WEIRD: slot was not enabled");
        }
    }
}

void AudioEffectAnalogChorus::rate(float rate)
{
    // update the LFO by mapping the rate into the MIN/MAX range, pass to LFO in milliseconds
    m_lfo.setRateAudio(m_LFO_MIN_RATE + (rate * m_LFO_RANGE));
}

void AudioEffectAnalogChorus::processMidi(int channel, int control, int value)
{

	float val = (float)value / 127.0f;

	if ((m_midiConfig[RATE][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[RATE][MIDI_CONTROL] == control)) {
		// Rate
        Serial.println(String("AudioEffectAnalogChorus::rate: ") + 100*val + String("%"));
        rate(val);
        return;
	}

	if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectAnalogChorus::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectAnalogChorus::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[DEPTH][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[DEPTH][MIDI_CONTROL] == control)) {
		// depth
		Serial.println(String("AudioEffectAnalogChorus::depth: ") + 100*val + String("%"));
		depth(val);
		return;
	}

	if ((m_midiConfig[MIX][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[MIX][MIDI_CONTROL] == control)) {
		// Mix
		Serial.println(String("AudioEffectAnalogChorus::mix: Dry: ") + 100*(1-val) + String("% Wet: ") + 100*val );
		mix(val);
		return;
	}

	if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
		// Volume
		Serial.println(String("AudioEffectAnalogChorus::volume: ") + 100*val + String("%"));
		volume(val);
		return;
	}

}

void AudioEffectAnalogChorus::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
	if (parameter >= NUM_CONTROLS) {
		return ; // Invalid midi parameter
	}
	m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
	m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

}


