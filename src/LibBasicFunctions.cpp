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

audio_block_t alphaBlend(audio_block_t *out, audio_block_t *dry, audio_block_t* wet, float mix)
{
	for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
		out->data[i] = (dry->data[i] * (1 - mix)) + (wet->data[i] * mix);
	}
}

void clearAudioBlock(audio_block_t *block)
{
	memset(block->data, 0, sizeof(int16_t)*AUDIO_BLOCK_SAMPLES);
}


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
	if (!dest) return false;

	if (m_type == (MemType::MEM_INTERNAL)) {
		QueuePosition position = calcQueuePosition(offset);
		size_t index = position.index;

		if (position.offset == 0) {
			// single transfer
			audio_block_t *currentQueue = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index));
			memcpy(static_cast<void*>(dest->data), static_cast<void*>(currentQueue->data), numSamples * sizeof(int16_t));
			return true;
		}

		// Otherwise we need to break the transfer into two memcpy because it will go across two source queues.
		// Audio is stored in reverse order. That means the first sample (in time) goes in the last location in the audio block.
		int16_t *destStart = dest->data;
		audio_block_t *currentQueue;
		int16_t *srcStart;

		// Break the transfer into two. Copy the 'older' data first then the 'newer' data with respect to current time.
		currentQueue = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index+1)); // The latest buffer is at the back. We need index+1 counting from the back.
		srcStart  = (currentQueue->data + AUDIO_BLOCK_SAMPLES - position.offset);
		size_t numData = position.offset;
		memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));


		currentQueue = m_ringBuffer->at(m_ringBuffer->get_index_from_back(index)); // now grab the queue where the 'first' data sample was
		destStart += numData; // we already wrote numData so advance by this much.
		srcStart  = (currentQueue->data);
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

			m_slot->printStatus();

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

}

