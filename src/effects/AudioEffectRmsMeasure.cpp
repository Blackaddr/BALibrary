/*
 * AudioEffectRmsMeasure.cpp
 *
 *  Created on: March 7, 2020
 *      Author: slascos
 */
#include <arm_math.h>
#include "BALibrary.h"
#include "AudioEffectRmsMeasure.h"

using namespace BALibrary;

namespace BAEffects {

AudioEffectRmsMeasure::AudioEffectRmsMeasure(unsigned numBlockMeasurements)
:   AudioStream(1, m_inputQueueArray),
  m_numBlockMeasurements(numBlockMeasurements),
  m_accumulatorCount(0),
  m_sum(0),
  m_rms(0.0f),
  m_dbfs(0.0f)
{
}

AudioEffectRmsMeasure::~AudioEffectRmsMeasure()
{
}

void AudioEffectRmsMeasure::update(void)
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

	// Calculate the RMS noise over the specified number of audio blocks.
	// Once the necessary blocks have been accumulated, calculate and print the
	// RMS noise figure.
	// RMS noise = sqrt((1/N) * (x1*x1 + x2*x2 + ...) )

	// First create the square sum of the input audio block and add it to the accumulator
	int64_t dotProduct = 0;
	//arm_dot_prod_q15(inputAudioBlock->data, inputAudioBlock->data, AUDIO_BLOCK_SAMPLES, &dotProduct);
	for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
	    dotProduct += ((int64_t)inputAudioBlock->data[i] * (int64_t)inputAudioBlock->data[i]);
	    if (dotProduct < 0) {
	        Serial.println("DOT ERROR!");
	        Serial.println(inputAudioBlock->data[i], HEX);
	    }
	}
	m_sum += (int64_t)(dotProduct);

	m_accumulatorCount++;

	// Check if we have enough samples accumulated.
	if (m_accumulatorCount == m_numBlockMeasurements) {
	    // Calculate and print the RMS figure
	    float div = (float)m_sum / (float)(m_accumulatorCount * AUDIO_BLOCK_SAMPLES);
	    arm_sqrt_f32(div, &m_rms);
	    // dbfs = 20*log10(abs(rmsFigure)/32768.0f);
	    m_dbfs = 20.0f * log10(m_rms/32768.0f);

	    Serial.print("Accumulator: "); Serial.println((int)(m_sum >> 32), HEX);
	    Serial.print("RAW RMS: "); Serial.println(m_rms);

	    Serial.print("AudioEffectRmsMeasure: the RMS figure is "); Serial.print(m_dbfs);
	    Serial.print(" dBFS over "); Serial.print(m_accumulatorCount); Serial.println(" audio blocks");

	    m_sum = 0;
	    m_accumulatorCount = 0;

	}

	transmit(inputAudioBlock);
	release(inputAudioBlock);
}


}



