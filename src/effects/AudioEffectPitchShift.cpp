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
constexpr float WINDOW_GAIN = 0.5;

AudioEffectPitchShift::AudioEffectPitchShift()
: AudioStream(1, m_inputQueueArray)
{
    m_analysisBuffer = (fftType_t *)malloc(ANALYSIS_SIZE*sizeof(fftType_t));
    m_windowFunction = (fftType_t *)malloc(ANALYSIS_SIZE*sizeof(fftType_t));
    m_windowBuffer = (fftType_t *)malloc(SYNTHESIS_SIZE*sizeof(fftType_t));
    m_outputBuffer = (fftType_t *)malloc(ANALYSIS_SIZE*sizeof(fftType_t));
    m_synthesisBuffer = (fftType_t *)malloc(SYNTHESIS_SIZE*sizeof(fftType_t));

    m_analysisFreqBuffer  = (fftType_t *)malloc(2*SYNTHESIS_SIZE*sizeof(fftType_t));
    m_synthesisFreqBuffer = (fftType_t *)malloc(2*SYNTHESIS_SIZE*sizeof(fftType_t));

    // clear the audio buffer to avoid pops and configure the Hann window
    for (unsigned i=0; i<AudioEffectPitchShift::ANALYSIS_SIZE; i++) {
        m_analysisBuffer[i] = 0.0f;
        m_windowFunction[i] = 0.5f * (1.0f - cos(2.0f * M_PI *  (float)i / ANALYSIS_SIZE_F)) * WINDOW_GAIN;
    }

    for (unsigned i=0; i<SYNTHESIS_SIZE; i++) {
        m_windowBuffer[i] = 0.0f;
    }

    // Configure the FFT
    unsigned ret;
    ret = arm_rfft_init_f32(&fftFwdReal, &fftFwdComplex, SYNTHESIS_SIZE, FFT_FORWARD, FFT_DO_BIT_REVERSE); //init FFT
    if (ret != ARM_MATH_SUCCESS) { m_initFailed = true; };
    ret = arm_rfft_init_f32(&fftInvReal, &fftInvComplex, SYNTHESIS_SIZE, FFT_INVERSE, FFT_DO_BIT_REVERSE); //init FFT
    if (ret != ARM_MATH_SUCCESS) { m_initFailed = true; };
}

AudioEffectPitchShift::~AudioEffectPitchShift()
{
    free(m_analysisBuffer);
    m_analysisBuffer = nullptr;
    free(m_windowFunction);
    m_windowFunction = nullptr;
    free(m_windowBuffer);
    m_windowBuffer = nullptr;
    free(m_outputBuffer);
    m_outputBuffer = nullptr;
    free(m_synthesisBuffer);
    m_synthesisBuffer = nullptr;
    free(m_analysisFreqBuffer);
    m_analysisFreqBuffer = nullptr;
    free(m_synthesisFreqBuffer);
    m_synthesisFreqBuffer = nullptr;
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

	// Convert the contents of the audio blocks to the contiguous buffer
	// 1) Be aware the audio library stores audio samples in reverse temporal order.
	// This means the first sample (in time) is in the last location of the buffer.
	// 2) the oldest audio is at the front of the queue, the most recent at the back
	fftType_t *analysisPtr      = &m_analysisBuffer[0];
	fftType_t *analysisFreqPtr  = &m_analysisFreqBuffer[0];
	fftType_t *synthesisFreqPtr = &m_synthesisFreqBuffer[0];
	fftType_t *synthesisPtr     = &m_synthesisBuffer[0];

	// first shift the contents of the fftType_t buffer up by AUDIO_BLOCK SAMPLES
	for (unsigned i=0; i<(NUM_AUDIO_BLOCKS-1); i++) {
	    memcpy(&analysisPtr[i*AUDIO_BLOCK_SAMPLES],    &analysisPtr[(i+1)*AUDIO_BLOCK_SAMPLES],    AUDIO_BLOCK_SAMPLES*sizeof(fftType_t));
	    memcpy(&m_outputBuffer[i*AUDIO_BLOCK_SAMPLES], &m_outputBuffer[(i+1)*AUDIO_BLOCK_SAMPLES], AUDIO_BLOCK_SAMPLES*sizeof(fftType_t));
	}
	// Convert the newest incoming audio block to fftType_t
	int16ToFft(inputAudioBlock->data, &analysisPtr[(NUM_AUDIO_BLOCKS-1)*AUDIO_BLOCK_SAMPLES], AUDIO_BLOCK_SAMPLES);
	memset(&m_outputBuffer[(NUM_AUDIO_BLOCKS-1)*AUDIO_BLOCK_SAMPLES], 0, AUDIO_BLOCK_SAMPLES * sizeof(fftType_t));
	release(inputAudioBlock); // were done with it now

	if (m_initFailed) { Serial.println("FFT INIT FAILED"); }

	// Window the contents of the analysis buffer to a temp buffer
	memset(m_windowBuffer, 0, sizeof(float) * SYNTHESIS_SIZE);
	arm_mult_f32(analysisPtr, &m_windowFunction[0], m_windowBuffer, ANALYSIS_SIZE);

	// Perform the FFT
	arm_rfft_f32(&fftFwdReal, m_windowBuffer, analysisFreqPtr);


	//memset(analysisPtr, 0, sizeof(float)*ANALYSIS_SIZE);

	// perform the ocean pitch shift
	m_ocean(analysisFreqPtr, synthesisFreqPtr, (float)(m_frameIndex), m_pitchScale);
	//memcpy(synthesisFreqPtr, analysisFreqPtr, 2*ANALYSIS_SIZE*sizeof(fftType_t));


	arm_rfft_f32(&fftInvReal, synthesisFreqPtr, synthesisPtr);

	// Window the output before adding, only use first part of the synthesized waveform
	arm_mult_f32(synthesisPtr, &m_windowFunction[0], synthesisPtr, ANALYSIS_SIZE);

	// Add the synthesis to the output buffer
    arm_add_f32(m_outputBuffer, synthesisPtr, m_outputBuffer, ANALYSIS_SIZE);
	//memcpy(m_outputBuffer, synthesisPtr, sizeof(fftType_t)*AUDIO_BLOCK_SAMPLES);


    // Convert the fftType_t buffer back to integer
    audio_block_t *outputBlock = allocate();
    //fftToInt16 (analysisPtr, outputBlock->data, AUDIO_BLOCK_SAMPLES);
    fftToInt16 (m_outputBuffer, outputBlock->data, AUDIO_BLOCK_SAMPLES);

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

	if ((m_midiConfig[PITCH][MIDI_CHANNEL] == channel) &&
        (m_midiConfig[PITCH][MIDI_CONTROL] == control)) {
        // Volume
	    int pitchCents = roundf((val - 1.0f) * 1200.0f);
        Serial.println(String("AudioEffectPitchShift::pitch: ") + pitchCents + String(" cents"));
        setPitchShiftCents(pitchCents);
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


float AudioEffectPitchShift::setPitchKnob(float val)
{
    int pitchCents = roundf((val - 0.5f)*2.0f * 1200.0f);
    float pitchScale = setPitchShiftCents(pitchCents);
    return pitchScale;
}

float AudioEffectPitchShift::setPitchShiftCents(int shiftCents)
{
    constexpr float ROOT_12TH_OF_2 = 1.0594630944;
    // alpha = nthroot(2,12)^(pitchShiftCents/100);
    m_pitchScale = powf(ROOT_12TH_OF_2,((float)(shiftCents) / 100.0f));
    return m_pitchScale;
}

void AudioEffectPitchShift::m_ocean(fftType_t *inputFreq, fftType_t *outputFreq, float frameIndex, float pitchScale)
{
    // zero the output buffer
    for (unsigned i=0; i<(2*SYNTHESIS_SIZE); i++) {
        outputFreq[i] = 0.0f;
    }

    //pitchScale = 2.0f;

//    float phaseAdjustFactor = -((2.0f*((float)(M_PI))*frameIndex)
//            / (OVERLAP_FACTOR_F * FFT_OVERSAMPLE_FACTOR_F * SYNTHESIS_SIZE_F));
    float phaseAdjustFactor = -((2.0f*((float)(M_PI))*frameIndex)
                / (OVERLAP_FACTOR_F * FFT_OVERSAMPLE_FACTOR_F));

    //outputFreq[0] = inputFreq[0];
    //outputFreq[1] = inputFreq[1];
    for (unsigned k=1; k < SYNTHESIS_SIZE/2; k++) {

        float a = (float)k;
        // b = mka + 0.5
        // where m is the FFT oversample factor, k is the pitch scaling, a
        // is the original bin number
        float b = std::roundf( (FFT_OVERSAMPLE_FACTOR_F * pitchScale * a));
        unsigned b_int = (unsigned)(b);

        //if (b_int <256) {
        if ((b_int < (SYNTHESIS_SIZE/2/2))) {

            // phaseAdjust = (b-ma) * phaseAdjustFactor
            float phaseAdjust = (b - (FFT_OVERSAMPLE_FACTOR_F * a)) * phaseAdjustFactor;


            float a_real = inputFreq[2*k];
            float a_imag = inputFreq[2*k+1];

            // Note the real and imag are interleaved
            unsigned idx = 2*b_int;
            outputFreq[idx]      = (a_real * fastCos(phaseAdjust)) - (a_imag * fastSin(phaseAdjust));
            outputFreq[idx+1]  = (a_real * fastSin(phaseAdjust)) + (a_imag * fastCos(phaseAdjust));

            //if ((int)frameIndex % 512 == 0) {
            //Serial.println(String("b:") + b_int + String(" idx:") + idx + String(" coeff:") + outputFreq[idx] + String(":") + outputFreq[idx+1]); }

            // Negative Frequencies
            //unsigned negB = SYNTHESIS_SIZE-b_int;
            //outputFreq[2*negB]    = outputFreq[idx];
            //outputFreq[2*negB+1] = -outputFreq[idx+1];
        }
        // update the imag components
    }
}

}



