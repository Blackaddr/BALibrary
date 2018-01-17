/*
 * LibBasicFunctions.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#include "LibBasicFunctions.h"

namespace BAGuitar {

AudioDelay::AudioDelay(MemType type, size_t maxSamples)
{
    m_type = type;
    if (type == MemType::INTERNAL) {
        // INTERNAL memory consisting of audio_block_t data structures.
        QueuePosition pos = calcQueuePosition(maxSamples);
        m_ringBuffer = new RingBuffer<audio_block_t *>(pos.index+2); // If the delay is in queue x, we need to overflow into x+1, thus x+2 total buffers.
    } else {
        // TODO EXTERNAL memory

    }
}

AudioDelay::AudioDelay(MemType type, float delayTimeMs)
: AudioDelay(type, calcAudioSamples(delayTimeMs))
{

}

AudioDelay::~AudioDelay()
{
    if (m_ringBuffer) delete m_ringBuffer;
}

bool AudioDelay::addBlock(audio_block_t *block)
{
    if (m_type != MemType::INTERNAL) return false; // ERROR

    // purposefully don't check if block is valid, the ringBuffer can support nullptrs
    if ( m_ringBuffer->size() < m_ringBuffer->max_size() ) {
        // pop before adding
        release(m_ringBuffer->front());
        m_ringBuffer->pop_front();
    }

    // add the new buffer
    m_ringBuffer->push_back(block);

}

void AudioDelay::getSamples(audio_block_t *dest, size_t offset, size_t numSamples = AUDIO_BLOCK_SAMPLES)
{
    QueuePosition pos = calcQueuePosition(offset);
    size_t readOffset = pos.offset;
    size_t index = pos.index;

    // Audio is stored in reverse order. That means the first sample (in time) goes in the last location in the audio block.

}

}

