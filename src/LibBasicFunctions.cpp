/*
 * LibBasicFunctions.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BAGuitar {

size_t calcAudioSamples(float milliseconds)
{
	return (size_t)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
}

QueuePosition calcQueuePosition(size_t numSamples)
{
	QueuePosition queuePosition;
	queuePosition.index = (int)(numSamples / AUDIO_BLOCK_SAMPLES);
	queuePosition.offset = numSamples % AUDIO_BLOCK_SAMPLES;
	return queuePosition;
}
QueuePosition calcQueuePosition(float milliseconds) {
	size_t numSamples = (int)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return calcQueuePosition(numSamples);
}

size_t calcOffset(QueuePosition position)
{
	return (position.index*AUDIO_BLOCK_SAMPLES) + position.offset;
}

void alphaBlend(audio_block_t *out, audio_block_t *dry, audio_block_t* wet, float mix)
{
	 //Non-optimized version for illustrative purposes
//		for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
//			out->data[i] = (dry->data[i] * (1 - mix)) + (wet->data[i] * mix);
//		}
//		return;

	// ARM DSP optimized
	int16_t wetBuffer[AUDIO_BLOCK_SAMPLES];
	int16_t dryBuffer[AUDIO_BLOCK_SAMPLES];
	int16_t scaleFractWet = (int16_t)(mix * 32767.0f);
	int16_t scaleFractDry = 32767-scaleFractWet;

	arm_scale_q15(dry->data, scaleFractDry, 0, dryBuffer, AUDIO_BLOCK_SAMPLES);
	arm_scale_q15(wet->data, scaleFractWet, 0, wetBuffer, AUDIO_BLOCK_SAMPLES);
	arm_add_q15(wetBuffer, dryBuffer, out->data, AUDIO_BLOCK_SAMPLES);
}

void clearAudioBlock(audio_block_t *block)
{
	memset(block->data, 0, sizeof(int16_t)*AUDIO_BLOCK_SAMPLES);
}


////////////////////////////////////////////////////
// AudioDelay
////////////////////////////////////////////////////
AudioDelay::AudioDelay(size_t maxSamples)
: m_slot(nullptr)
{
    m_type = (MemType::MEM_INTERNAL);

	// INTERNAL memory consisting of audio_block_t data structures.
	QueuePosition pos = calcQueuePosition(maxSamples);
	m_ringBuffer = new RingBuffer<audio_block_t *>(pos.index+2); // If the delay is in queue x, we need to overflow into x+1, thus x+2 total buffers.
}

AudioDelay::AudioDelay(float maxDelayTimeMs)
: AudioDelay(calcAudioSamples(maxDelayTimeMs))
{

}

AudioDelay::AudioDelay(ExtMemSlot *slot)
{
	m_type = (MemType::MEM_EXTERNAL);
	m_slot = slot;
}

AudioDelay::~AudioDelay()
{
    if (m_ringBuffer) delete m_ringBuffer;
}

audio_block_t* AudioDelay::addBlock(audio_block_t *block)
{
	audio_block_t *blockToRelease = nullptr;

	if (m_type == (MemType::MEM_INTERNAL)) {
		// INTERNAL memory

		// purposefully don't check if block is valid, the ringBuffer can support nullptrs
		if ( m_ringBuffer->size() >= m_ringBuffer->max_size() ) {
			// pop before adding
			blockToRelease = m_ringBuffer->front();
			m_ringBuffer->pop_front();
		}

		// add the new buffer
		m_ringBuffer->push_back(block);
		return blockToRelease;

	} else {
		// EXTERNAL memory
		if (!m_slot) { Serial.println("addBlock(): m_slot is not valid"); }

		if (block) {

			// Audio is stored in reverse in block so we need to write it backwards to external memory
			// to maintain temporal coherency.
//			int16_t *srcPtr = block->data + AUDIO_BLOCK_SAMPLES - 1;
//			for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
//				m_slot->writeAdvance16(*srcPtr);
//				srcPtr--;
//			}

			int16_t *srcPtr = block->data;
			for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
				m_slot->writeAdvance16(*srcPtr);
				srcPtr++;
			}

		}
		blockToRelease =  block;
	}
	return blockToRelease;
}

audio_block_t* AudioDelay::getBlock(size_t index)
{
	audio_block_t *ret = nullptr;
	if (m_type == (MemType::MEM_INTERNAL)) {
		ret =  m_ringBuffer->at(m_ringBuffer->get_index_from_back(index));
	}
	return ret;
}

bool AudioDelay::getSamples(audio_block_t *dest, size_t offset, size_t numSamples)
{
	if (!dest) {
		Serial.println("getSamples(): dest is invalid");
		return false;
	}

	if (m_type == (MemType::MEM_INTERNAL)) {
		QueuePosition position = calcQueuePosition(offset);
		size_t index = position.index;

		audio_block_t *currentQueue0 = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index));
		// The latest buffer is at the back. We need index+1 counting from the back.
		audio_block_t *currentQueue1 = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index+1));

		// check if either queue is invalid, if so just zero the destination buffer
		if ( (!currentQueue0) || (!currentQueue0) ) {
			// a valid entry is not in all queue positions while it is filling, use zeros
			memset(static_cast<void*>(dest->data), 0, numSamples * sizeof(int16_t));
			return true;
		}

		if (position.offset == 0) {
			// single transfer
			memcpy(static_cast<void*>(dest->data), static_cast<void*>(currentQueue0->data), numSamples * sizeof(int16_t));
			return true;
		}

		// Otherwise we need to break the transfer into two memcpy because it will go across two source queues.
		// Audio is stored in reverse order. That means the first sample (in time) goes in the last location in the audio block.
		int16_t *destStart = dest->data;
		int16_t *srcStart;

		// Break the transfer into two. Copy the 'older' data first then the 'newer' data with respect to current time.
		//currentQueue = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index+1)); // The latest buffer is at the back. We need index+1 counting from the back.
		srcStart  = (currentQueue1->data + AUDIO_BLOCK_SAMPLES - position.offset);
		size_t numData = position.offset;
		memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));

		//currentQueue = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index)); // now grab the queue where the 'first' data sample was
		destStart += numData; // we already wrote numData so advance by this much.
		srcStart  = (currentQueue0->data);
		numData = AUDIO_BLOCK_SAMPLES - numData;
		memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));

		return true;

	} else {
		// EXTERNAL Memory
		if (numSamples*sizeof(int16_t) <= m_slot->size() ) {
			int currentPositionBytes = (int)m_slot->getWritePosition() - (int)(AUDIO_BLOCK_SAMPLES*sizeof(int16_t));
			size_t offsetBytes = offset * sizeof(int16_t);

			if ((int)offsetBytes <= currentPositionBytes) {
				m_slot->setReadPosition(currentPositionBytes - offsetBytes);
			} else {
				// It's going to wrap around to the end of the slot
				int readPosition = (int)m_slot->size() + currentPositionBytes - offsetBytes;
				m_slot->setReadPosition((size_t)readPosition);
			}

			//m_slot->printStatus();

			// write the data to the destination block in reverse
//			int16_t *destPtr = dest->data + AUDIO_BLOCK_SAMPLES-1;
//			for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
//				*destPtr = m_slot->readAdvance16();
//				destPtr--;
//			}

			int16_t *destPtr = dest->data;
			for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
				*destPtr = m_slot->readAdvance16();
				destPtr++;
			}
			return true;
		} else {
			// numSampmles is > than total slot size
			Serial.println("getSamples(): ERROR numSamples > total slot size");
			return false;
		}
	}

}

////////////////////////////////////////////////////
// IirBiQuadFilter
////////////////////////////////////////////////////
IirBiQuadFilter::IirBiQuadFilter(unsigned numStages, const int32_t *coeffs, int coeffShift)
: NUM_STAGES(numStages)
{
	m_coeffs = new int32_t[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(int32_t));

	m_state  = new int32_t[4*numStages];
	arm_biquad_cascade_df1_init_q31(&m_iirCfg, numStages, m_coeffs, m_state, coeffShift);
}

IirBiQuadFilter::~IirBiQuadFilter()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilter::process(int16_t *output, int16_t *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(int16_t));
	} else {

		// create convertion buffers on teh stack
		int32_t input32[numSamples];
		int32_t output32[numSamples];
		for (int i=0; i<numSamples; i++) {
			input32[i] = (int32_t)(input[i]);
		}

		arm_biquad_cascade_df1_fast_q31(&m_iirCfg, input32, output32, numSamples);

		for (int i=0; i<numSamples; i++) {
			output[i] = (int16_t)(output32[i] & 0xffff);
		}
	}
	return true;
}

// HIGH QUALITY
IirBiQuadFilterHQ::IirBiQuadFilterHQ(unsigned numStages, const int32_t *coeffs, int coeffShift)
: NUM_STAGES(numStages)
{
	m_coeffs = new int32_t[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(int32_t));

	m_state = new int64_t[4*numStages];;
	arm_biquad_cas_df1_32x64_init_q31(&m_iirCfg, numStages, m_coeffs, m_state, coeffShift);
}

IirBiQuadFilterHQ::~IirBiQuadFilterHQ()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilterHQ::process(int16_t *output, int16_t *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(int16_t));
	} else {

		// create convertion buffers on teh stack
		int32_t input32[numSamples];
		int32_t output32[numSamples];
		for (int i=0; i<numSamples; i++) {
			input32[i] = (int32_t)(input[i]);
		}

		arm_biquad_cas_df1_32x64_q31(&m_iirCfg, input32, output32, numSamples);

		for (int i=0; i<numSamples; i++) {
			output[i] = (int16_t)(output32[i] & 0xffff);
		}
	}
	return true;
}

// FLOAT
IirBiQuadFilterFloat::IirBiQuadFilterFloat(unsigned numStages, const float *coeffs)
: NUM_STAGES(numStages)
{
	m_coeffs = new float[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(float));

	m_state = new float[4*numStages];;
	arm_biquad_cascade_df2T_init_f32(&m_iirCfg, numStages, m_coeffs, m_state);
}

IirBiQuadFilterFloat::~IirBiQuadFilterFloat()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilterFloat::process(float *output, float *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(float));
	} else {

		arm_biquad_cascade_df2T_f32(&m_iirCfg, input, output, numSamples);

	}
	return true;
}

}

