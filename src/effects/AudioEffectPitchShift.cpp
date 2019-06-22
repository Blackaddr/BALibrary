/*
 * AudioEffectPitchShift.cpp
 *
 *  Created on: June 20, 2019
 *      Author: slascos
 */
#include <cmath> // std::roundf
#include "AudioEffectPitchShift.h"

using namespace BALibrary;

namespace BAEffects {

constexpr int MIDI_CHANNEL = 0;
constexpr int MIDI_CONTROL = 1;

constexpr unsigned NUM_AUDIO_BLOCKS = AudioEffectPitchShift::ANALYSIS_SIZE / AUDIO_BLOCK_SAMPLES;
constexpr uint32_t FFT_FORWARD = 0;
constexpr uint32_t FFT_INVERSE = 1;
constexpr uint32_t FFT_DO_BIT_REVERSE = 1;

AudioEffectPitchShift::AudioEffectPitchShift()
: AudioStream(1, m_inputQueueArray)
{
    // clear the audio buffer to avoid pops
    for (unsigned i=0; i<AudioEffectPitchShift::ANALYSIS_SIZE; i++) {
        m_analysisBuffer[i] = 0.0f;
    }

    // Configure the FFT
//    arm_rfft_init_f32(&rfftForwardInst, &cfftForwardInst, AudioEffectPitchShift::ANALYSIS_SIZE,
//                      FFT_FORWARD, FFT_DO_BIT_REVERSE);
//    arm_rfft_init_f32(&rfftInverseInst, &cfftInverseInst, AudioEffectPitchShift::SYNTHESIS_SIZE,
//                          FFT_INVERSE, FFT_DO_BIT_REVERSE);
    unsigned ret;
    ret = arm_cfft_radix4_init_f32(&cfftForwardInst, ANALYSIS_SIZE, FFT_FORWARD, FFT_DO_BIT_REVERSE); //init FFT
    if (!ret) { m_initFailed = true; };
    ret = arm_cfft_radix4_init_f32(&cfftInverseInst, SYNTHESIS_SIZE, FFT_INVERSE, FFT_DO_BIT_REVERSE); //init FFT
    if (!ret) { m_initFailed = true; };
}

AudioEffectPitchShift::~AudioEffectPitchShift()
{
}

void AudioEffectPitchShift::update(void)
{
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

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
	// Update the fifo
//	m_inputFifo.push_back(inputAudioBlock); // insert the new block
//	release(m_inputFifo.front()); //
//	m_inputFifo.pop_front();

	// Convert the contents of the audio blocks to the contiguous buffer
	// 1) Be aware the audio library stores audio samples in reverse temporal order.
	// This means the first sample (in time) is in the last location of the buffer.
	// 2) the oldest audio is at the front of the queue, the latest at the back
	float *analysisPtr      = &m_analysisBuffer[0];
	float *analysisFreqPtr  = &m_analysisFreqBuffer[0];
	float *synthesisFreqPtr = &m_synthesisFreqBuffer[0];
	float *synthesisPtr     = &m_synthesisBuffer[0];

	// first shift the contents of the float buffer up by AUDIO_BLOCK SAMPLES
	for (unsigned i=0; i<NUM_AUDIO_BLOCKS-1; i++) {
	    memcpy(&analysisPtr[i*AUDIO_BLOCK_SAMPLES], &analysisPtr[(i+1)*AUDIO_BLOCK_SAMPLES], AUDIO_BLOCK_SAMPLES*sizeof(float));
	}
	// Convert the newest incoming audio block to float
	arm_q15_to_float(inputAudioBlock->data, &analysisPtr[(NUM_AUDIO_BLOCKS-1)*AUDIO_BLOCK_SAMPLES], AUDIO_BLOCK_SAMPLES);
	release(inputAudioBlock); // were done with it now

	//if (m_initFailed) { Serial.println("FFT INIT FAILED"); }

	// Construct the interleaved FFT buffer
	unsigned idx = 0;
	for (unsigned i=0; i<ANALYSIS_SIZE; i++) {
	    m_analysisFreqBuffer[idx] = analysisPtr[i];
	    m_analysisFreqBuffer[idx+1] = 0;
	    idx += 2;
	}

	// Perform the FFT
	arm_cfft_radix4_f32(&cfftForwardInst, analysisFreqPtr);


	// perform the ocean pitch shift
	m_ocean(analysisFreqPtr, synthesisFreqPtr, (float)(m_frameIndex), m_pitchScale);
	//memcpy(synthesisFreqPtr, analysisFreqPtr, 2*ANALYSIS_SIZE*sizeof(float));

	// Perform the inverse FFT
	arm_cfft_radix4_f32(&cfftInverseInst, synthesisFreqPtr);

	// Deinterleave the synthesis buffer
	idx = 0;
    for (unsigned i=0; i<(2*SYNTHESIS_SIZE); i=i+2) {
        m_synthesisBuffer[idx] = synthesisFreqPtr[i];
        idx++;
    }

    // Convert the float buffer back to integer
    audio_block_t *outputBlock = allocate();
    arm_float_to_q15 (synthesisPtr, outputBlock->data, AUDIO_BLOCK_SAMPLES);

	transmit(outputBlock);
	release(outputBlock);
	m_frameIndex++;
}

void AudioEffectPitchShift::processMidi(int channel, int control, int value)
{

	float val = (float)value / 127.0f;

	if ((m_midiConfig[BYPASS][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[BYPASS][MIDI_CONTROL] == control)) {
		// Bypass
		if (value >= 65) { bypass(false); Serial.println(String("AudioEffectPitchShift::not bypassed -> ON") + value); }
		else { bypass(true); Serial.println(String("AudioEffectPitchShift::bypassed -> OFF") + value); }
		return;
	}

	if ((m_midiConfig[VOLUME][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[VOLUME][MIDI_CONTROL] == control)) {
		// Volume
		Serial.println(String("AudioEffectPitchShift::volume: ") + 100*val + String("%"));
		volume(val);
		return;
	}

}

void AudioEffectPitchShift::mapMidiControl(int parameter, int midiCC, int midiChannel)
{
	if (parameter >= NUM_CONTROLS) {
		return ; // Invalid midi parameter
	}
	m_midiConfig[parameter][MIDI_CHANNEL] = midiChannel;
	m_midiConfig[parameter][MIDI_CONTROL] = midiCC;
}

void AudioEffectPitchShift::m_ocean(float *inputFreq, float *outputFreq, float frameIndex, float pitchScale)
{
    // zero the output buffer
    for (unsigned i=0; i<(2*SYNTHESIS_SIZE); i++) {
        outputFreq[i] = 0.0f;
    }

    float phaseAdjustFactor = -((2.0f*((float)(M_PI))*frameIndex)
            / (OVERLAP_FACTOR_F * FFT_OVERSAMPLE_FACTOR_F * SYNTHESIS_SIZE_F));

    for (unsigned k=1; k < SYNTHESIS_SIZE/2; k++) {

        float a = (float)k;
        // b = mka + 0.5
        // where m is the FFT oversample factor, k is the pitch scaling, a
        // is the original bin number
        float b = std::roundf( (FFT_OVERSAMPLE_FACTOR_F * pitchScale * a));
        unsigned b_int = (unsigned)(b);

        if (b_int < SYNTHESIS_SIZE/2) {

            // phaseAdjust = (b-ma) * phaseAdjustFactor
            float phaseAdjust = (b - (FFT_OVERSAMPLE_FACTOR_F * a)) * phaseAdjustFactor;

            float a_real = inputFreq[2*k];
            float a_imag = inputFreq[2*k+1];

            outputFreq[2*b_int]   = (a_real * arm_cos_f32(phaseAdjust)) - (a_imag * arm_sin_f32(phaseAdjust));
            outputFreq[2*b_int+1] = (a_real * arm_sin_f32(phaseAdjust)) + (a_imag * arm_cos_f32(phaseAdjust));
        }

        // update the imag components
    }
}

}



