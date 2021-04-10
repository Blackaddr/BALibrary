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
#include <cmath>

#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BALibrary {

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
	m_maxDelaySamples = maxSamples;
}

AudioDelay::AudioDelay(float maxDelayTimeMs)
: AudioDelay(calcAudioSamples(maxDelayTimeMs))
{

}

AudioDelay::AudioDelay(ExtMemSlot *slot)
{
	m_type = (MemType::MEM_EXTERNAL);
	m_slot = slot;
	m_maxDelaySamples = (slot->size() / sizeof(int16_t)) - AUDIO_BLOCK_SAMPLES;
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

            // KLUGE! The Teensy Audio Library doesn't support DMA buffers correctly on the T4.0. We need to
            // use an intermediate copy buffer kluge here.
#if defined(__IMXRT1062__)
            // This is a T4.0 build
            setSpiDmaCopyBuffer();
#endif

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

size_t AudioDelay::getMaxDelaySamples()
{
    if (m_type == MemType::MEM_EXTERNAL) {
        // update the max delay sample size
        m_maxDelaySamples = (m_slot->size() / sizeof(int16_t)) - AUDIO_BLOCK_SAMPLES;
    }
    return m_maxDelaySamples;
}

bool AudioDelay::getSamples(audio_block_t *dest, size_t offsetSamples, size_t numSamples)
{
    if (!dest) { return false; }
    else {
	    return m_getSamples(dest->data, offsetSamples, numSamples);
    }
}

bool AudioDelay::getSamples(int16_t *dest, size_t offsetSamples, size_t numSamples)
{
	return m_getSamples(dest, offsetSamples, numSamples);
}

bool AudioDelay::m_getSamples(int16_t *dest, size_t offsetSamples, size_t numSamples)
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
		if ( (!currentQueue0) || (!currentQueue1) ) {
			// a valid entry is not in all queue positions while it is filling, use zeros
			memset(static_cast<void*>(dest), 0, numSamples * sizeof(int16_t));
			return true;
		}

		if ( (position.offset == 0) && numSamples <= AUDIO_BLOCK_SAMPLES ) {
			// single transfer
			memcpy(static_cast<void*>(dest), static_cast<void*>(currentQueue0->data), numSamples * sizeof(int16_t));
			return true;
		}

		// Otherwise we need to break the transfer into two memcpy because it will go across two source queues.
		// Audio is stored in reverse order. That means the first sample (in time) goes in the last location in the audio block.
		int16_t *destStart = dest;
		int16_t *srcStart;

		// Break the transfer into two. Copy the 'older' data first then the 'newer' data with respect to current time.
		// TODO: should AUDIO_BLOCK_SAMPLES on the next line be numSamples?
		srcStart  = (currentQueue1->data + AUDIO_BLOCK_SAMPLES - position.offset);
		size_t numData = position.offset;
		memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));

		destStart += numData; // we already wrote numData so advance by this much.
		srcStart  = (currentQueue0->data);
		numData = numSamples - numData;
		memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));

		return true;

	} else {
		// EXTERNAL Memory
		if (numSamples*sizeof(int16_t) <= m_slot->size() ) { // check for overflow

			// current position is considered the write position subtracted by the number of samples we're going
			// to read since this is the smallest delay we can get without reading past the write position into
			// the "future".
			int currentPositionBytes = (int)m_slot->getWritePosition() - (int)(numSamples*sizeof(int16_t));
			size_t offsetBytes = offsetSamples * sizeof(int16_t);

			if ((int)offsetBytes <= currentPositionBytes) {
				// when we back up to read, we won't wrap over the beginning of the slot
				m_slot->setReadPosition(currentPositionBytes - offsetBytes);
			} else {
				// It's going to wrap around to the from the beginning to the end of the slot.
				int readPosition = (int)m_slot->size() + currentPositionBytes - offsetBytes;
				m_slot->setReadPosition((size_t)readPosition);
			}

			// Read the number of samples
			m_slot->readAdvance16(dest, numSamples);

			return true;
		} else {
			// numSamples is > than total slot size
			Serial.println("getSamples(): ERROR numSamples > total slot size");
			Serial.println(numSamples + String(" > ") + m_slot->size());
			return false;
		}
	}

}

bool AudioDelay::interpolateDelay(int16_t *extendedSourceBuffer, int16_t *destBuffer, float fraction, size_t numSamples)
{
	int16_t frac1 = static_cast<int16_t>(32767.0f * fraction);
	int16_t frac2 = 32767 - frac1;

	if ((fraction < 0.0f) || (fraction > 1.0f) ) {
	    return false;
	}

	/// @todo optimize this later
	for (unsigned i=0; i<numSamples; i++) {
		destBuffer[i] =  ((frac1*extendedSourceBuffer[i]) >> 16) +  ((frac2*extendedSourceBuffer[i+1]) >> 16);
	}
	return true;
}

bool AudioDelay::setSpiDmaCopyBuffer(void)
{
    bool returnValue = false;

    if (m_slot->isUseDma()) {
        // For DMA use on T4.0 we need this kluge
        BASpiMemoryDMA * spiDma = static_cast<BASpiMemoryDMA*>(m_slot->getSpiMemoryHandle());
        if (spiDma) {
            // Check if the size is already set
            if (spiDma->getDmaCopyBufferSize() == 0) {
              spiDma->setDmaCopyBufferSize(sizeof(int16_t) * AUDIO_BLOCK_SAMPLES);
              returnValue = true;
            }
        }
    }
    return returnValue;
}


}

