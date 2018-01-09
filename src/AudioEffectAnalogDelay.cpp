/*
 * AudioEffectAnalogDelay.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include "AudioEffectAnalogDelay.h"

namespace BAGuitar {

AudioEffectAnalogDelay::AudioEffectAnalogDelay(INTERNAL_MEMORY, float maxDelay)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new MemAudioBlock(maxDelay);
	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
		m_channelOffsets[i] = 0;
	}
}

// requires preallocated memory large enough
AudioEffectAnalogDelay::AudioEffectAnalogDelay(EXTERNAL_MEMORY, MemSlot &slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = &slot;
	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
		m_channelOffsets[i] = 0;
	}
	m_memory->clear();
}

void AudioEffectAnalogDelay::update(void)
{
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

	if (m_externalMemory) {
		// external mem requires an actual write to memory with the new data block
		if (inputAudioBlock) {
			// valid block
			m_memory->writeAdvance16(inputAudioBlock->data, AUDIO_BLOCK_SAMPLES);
		} else {
			// write zeros instead
			m_memory->zeroAdvance16(AUDIO_BLOCK_SAMPLES);
		}
	} else {
		// internal memory only requires updating the queue of audio block pointers.
		MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);
		memory->push(inputAudioBlock); // MemAudioBlock supports nullptrs, no need to check

		// check to see if the queue has reached full size yet
		if (memory->getNumQueues() > m_numQueues) {
			release(memory->pop());
		}
	}

	// For each active channel, output the delayed audio
	audio_block_t *blockToOutput = nullptr;
	for (int channel=0; channel<MAX_DELAY_CHANNELS; channel++) {
		if (!(m_activeChannels & (1<<channel))) continue; // If this channel is disable, skip to the next;

		if (!m_externalMemory) {
			// internal memory
			MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);
			QueuePosition queuePosition = calcQueuePosition(m_channelOffsets[channel]);
			if (queuePosition.offset == 0) {
				// only one queue needs to be read out
				blockToOutput= memory->getQueue(queuePosition.index); // could be nullptr!
				transmit(blockToOutput);
			} else {
				blockToOutput = allocate(); // allocate if spanning 2 queues
			}
		} else {
			blockToOutput = allocate(); // allocate always for external memory
		}

		// copy the output data
		if (!blockToOutput) continue; // skip this channel due to failure
		// copy over data
		m_memory->read16(blockToOutput->data, 0, m_channelOffsets[channel], AUDIO_BLOCK_SAMPLES);
		transmit(blockToOutput);
		release(blockToOutput);
	}
}

bool AudioEffectAnalogDelay::delay(unsigned channel, float milliseconds)
{
	if (channel > MAX_DELAY_CHANNELS-1) // channel id too high
		return false;

	size_t delaySamples = calcAudioSamples(milliseconds);

	if (!m_externalMemory) {
		// Internal memory (AudioStream buffers)
		QueuePosition queuePosition = calcQueuePosition(milliseconds);
		size_t numQueues = queuePosition.index+1;
		if (numQueues > m_numQueues)
			m_numQueues = numQueues;
	} else {
		// External memory
		if (delaySamples > m_memory->getSize()) {
			// error, the external memory is not large enough
			return false;
		}
	}

	m_channelOffsets[channel] = delaySamples;
	m_activeChannels |= 1<<channel;
	return true;
}

void AudioEffectAnalogDelay::disable(unsigned channel)
{
	m_activeChannels &= ~(1<<channel);
}

}


