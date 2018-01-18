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


AudioDelay::AudioDelay(size_t maxSamples)
: m_slot(nullptr)
{
    m_type = MemType::MEM_INTERNAL;

	// INTERNAL memory consisting of audio_block_t data structures.
	QueuePosition pos = calcQueuePosition(maxSamples);
	m_ringBuffer = new RingBuffer<audio_block_t *>(pos.index+2); // If the delay is in queue x, we need to overflow into x+1, thus x+2 total buffers.
}

AudioDelay::AudioDelay(float maxDelayTimeMs)
: AudioDelay(calcAudioSamples(maxDelayTimeMs))
{

}

AudioDelay::AudioDelay(ExtMemSlot &slot)
: m_slot(slot)
{
	m_type = MemType::MEM_EXTERNAL;
}

AudioDelay::~AudioDelay()
{
    if (m_ringBuffer) delete m_ringBuffer;
}

audio_block_t* AudioDelay::addBlock(audio_block_t *block)
{
	if (m_type == MemType::MEM_INTERNAL) {
		// INTERNAL memory
		audio_block_t *blockToRelease = nullptr;
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
		m_slot.writeAdvance16(block->data, AUDIO_BLOCK_SAMPLES);
		return nullptr;

	}

}

audio_block_t* AudioDelay::getBlock(size_t index)
{
	if (m_type == MemType::MEM_INTERNAL) {
		return m_ringBuffer->at(m_ringBuffer->get_index_from_back(index));
	} else {
		return nullptr;
	}
}

bool AudioDelay::getSamples(audio_block_t *dest, size_t offset, size_t numSamples)
{
	if (!dest) return false;

	if (m_type == MemType::MEM_INTERNAL) {
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
		if (numSamples < m_slot.size() ) {
			m_slot.read16FromCurrent(dest->data, offset, numSamples);
			return true;
		} else {
			// numSampmles is > than total slot size
			return false;
		}
	}

}

}

