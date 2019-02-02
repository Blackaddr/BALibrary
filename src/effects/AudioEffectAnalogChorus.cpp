/*
 * AudioEffectAnalogChorus.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include <cmath>
#include "AudioEffectAnalogChorusFilters.h"
#include "AudioEffectAnalogChorus.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr float DELAY_REFERENCE_F = static_cast<float>(AUDIO_BLOCK_SAMPLES/2);

AudioEffectAnalogChorus::AudioEffectAnalogChorus()
: AudioStream(1, m_inputQueueArray)
{
    m_memory = new AudioDelay(m_DEFAULT_AVERAGE_DELAY_MS + m_DELAY_RANGE);
    m_maxDelaySamples = calcAudioSamples(m_DEFAULT_AVERAGE_DELAY_MS + m_DELAY_RANGE);
    m_averageDelaySamples = static_cast<float>(calcAudioSamples(m_DEFAULT_AVERAGE_DELAY_MS));
    m_delayRange          = static_cast<float>(calcAudioSamples(m_DELAY_RANGE));

    m_constructFilter();
    m_lfo.setWaveform(Waveform::TRIANGLE);
    m_lfo.setRateAudio(4.0f); // Default to 4 Hz
}

// requires preallocated memory large enough
AudioEffectAnalogChorus::AudioEffectAnalogChorus(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);
	m_maxDelaySamples = (slot->size() / sizeof(int16_t));
	m_averageDelaySamples = static_cast<float>(calcAudioSamples(m_DEFAULT_AVERAGE_DELAY_MS));
	m_delayRange          = static_cast<float>(calcAudioSamples(m_DELAY_RANGE));

	m_externalMemory = true;
	m_constructFilter();
	m_lfo.setWaveform(Waveform::TRIANGLE);
	m_lfo.setRateAudio(4.0f); // Default to 4 Hz
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

void AudioEffectAnalogChorus::setWaveform(BALibrary::Waveform waveform)
{
    switch(waveform) {
    case Waveform::SINE :
    case Waveform::TRIANGLE :
    case Waveform::SAWTOOTH :
        m_lfo.setWaveform(waveform);
        break;
    default :
        Serial.println("AudioEffectAnalogChorus::setWaveform: Unsupported Waveform");
    }
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
		// do not transmit or proess any audio, return as quickly as possible.
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
    // We need to grab two blocks of audio since the modulating delay value from the LFO
    // can exceed the length of one audio block during the time frame of one audio block.
    int16_t extendedBuffer[(2*AUDIO_BLOCK_SAMPLES)]; // need one more sample for interpolating between 128th and 129th (last sample)

    // Get next vector of lfo values, they will range range from -1.0 to +1.0f.
    float *lfoValues = m_lfo.getNextVector();
    //float lfoValues[128];
    for (int i=0; i<128; i++) { lfoValues[i] = lfoValues[i] * m_lfoDepth; }

    // Calculate the starting delay from the first lfo sample. This will represent the 'reference' delay
    // for this output block
    float referenceDelay  = m_averageDelaySamples + (lfoValues[0] * m_delayRange);
    unsigned delaySamples = static_cast<unsigned>(referenceDelay); // round down to the nearest audio sample for indexing into AudioDelay class

    // From a given current delay value, while reading out the next 128, the delay could slew up or down
    // AUDIO_BLOCK_SAMPLES/2 cycles of delay. For example...
    // Pitching up  : current + 128 +  64
    // Pitching down: current -  64 + 128
    // We need to grab 2*AUDIO_BLOCK_SAMPLES. Be aware that audio samples are stored BACKWARDS in the buffers.
//    m_memory->getSamples(extendedBuffer                      , delaySamples - (AUDIO_BLOCK_SAMPLES/2), AUDIO_BLOCK_SAMPLES);
//    m_memory->getSamples(extendedBuffer + AUDIO_BLOCK_SAMPLES, delaySamples +( AUDIO_BLOCK_SAMPLES/2), AUDIO_BLOCK_SAMPLES);
    m_memory->getSamples(extendedBuffer + AUDIO_BLOCK_SAMPLES, delaySamples - (AUDIO_BLOCK_SAMPLES/2), AUDIO_BLOCK_SAMPLES);
    m_memory->getSamples(extendedBuffer                      , delaySamples +( AUDIO_BLOCK_SAMPLES/2), AUDIO_BLOCK_SAMPLES);
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
	    // Using DMA so we have to busy-wait here until DMA is done
		while (m_memory->getSlot()->isReadBusy()) {}
	}

	double bufferIndexFloat;
	int delayIndex;
	for (int i=0, j=AUDIO_BLOCK_SAMPLES-1; i<AUDIO_BLOCK_SAMPLES; i++,j--) {
	    // each output sample will be an interpolated value between two samples
	    // the precise delay value will be based on the LFO vector values.
	    // For each output sample, calculate the floating point delay offset from the reference delay.
	    // This will be an offset from location AUDIO_BLOCK_SAMPLES/2 (e.g. 64) in the buffer.
	    float offsetDelayFromRef = m_averageDelaySamples + (lfoValues[i] * m_delayRange) - referenceDelay;
	    float bufferPosition = DELAY_REFERENCE_F + offsetDelayFromRef;

	    // Get the interpolation coefficients from the fractional part of the buffer position
	    float fraction1 = modf(bufferPosition, &bufferIndexFloat);
	    float fraction2 = 1.0f -  fraction1;
	    //fraction1 = 0.5f;
	    //fraction2 = 0.5f;
	    delayIndex = static_cast<unsigned>(bufferIndexFloat);
	    if ( (delayIndex < 0) || (delayIndex > 256) ) {
	        Serial.println(String("lfoValues[") + i + String("]:") + lfoValues[i] +
	                       String(" referenceDelay:") + referenceDelay +
	                       String(" bufferPosition:") + bufferPosition +
	                       String(" delayIndex:") + delayIndex) ;
	    }

	    //delayIndex = 64+i;
	    blockToOutput->data[j] = static_cast<int16_t>(
	                               (static_cast<float>(extendedBuffer[j+delayIndex])   * fraction1) +
	                               (static_cast<float>(extendedBuffer[j+delayIndex+1]) * fraction2) );
	    //blockToOutput->data[i] = extendedBuffer[64+i];
	}

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
    // TODO: Clean this up with proper preprocessing
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
    setDelayConfig(calcAudioSamples(averageDelayMs), calcAudioSamples(delayRangeMs));
}

void AudioEffectAnalogChorus::setDelayConfig(size_t averageDelayNumSamples, size_t delayRangeNumSamples)
{
    size_t delaySamples = averageDelayNumSamples + delayRangeNumSamples;
    m_averageDelaySamples = averageDelayNumSamples;
    m_delayRange = delayRangeNumSamples;

    if (delaySamples > m_memory->getMaxDelaySamples()) {
        // this exceeds max delay value, limit it.
        delaySamples = m_memory->getMaxDelaySamples();
        m_averageDelaySamples = delaySamples/2;
        m_delayRange = delaySamples/2;
    }

    if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

    if (m_externalMemory) {
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


