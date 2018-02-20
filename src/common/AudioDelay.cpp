/*
 * AudioDelay.cpp
 *
 *  Created on: January 1, 2018
 *      Author: slascos
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.*
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BAGuitar {

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
			// this causes pops
		    m_slot->writeAdvance16(block->data, AUDIO_BLOCK_SAMPLES);
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

bool AudioDelay::getSamples(audio_block_t *dest, size_t offsetSamples, size_t numSamples)
{
	if (!dest) {
		Serial.println("getSamples(): dest is invalid");
		return false;
	}

	if (m_type == (MemType::MEM_INTERNAL)) {
		QueuePosition position = calcQueuePosition(offsetSamples);
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
			size_t offsetBytes = offsetSamples * sizeof(int16_t);

			if ((int)offsetBytes <= currentPositionBytes) {
				m_slot->setReadPosition(currentPositionBytes - offsetBytes);
			} else {
				// It's going to wrap around to the end of the slot
				int readPosition = (int)m_slot->size() + currentPositionBytes - offsetBytes;
				m_slot->setReadPosition((size_t)readPosition);
			}

			// This causes pops
			m_slot->readAdvance16(dest->data, AUDIO_BLOCK_SAMPLES);

			return true;
		} else {
			// numSampmles is > than total slot size
			Serial.println("getSamples(): ERROR numSamples > total slot size");
			return false;
		}
	}

}

}

