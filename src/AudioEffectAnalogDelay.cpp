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

AudioEffectAnalogDelay::AudioEffectAnalogDelay(INTERNAL_MEMORY, size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new MemAudioBlock(numSamples);
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
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samplestransmit(inputAudioBlock, 0);

	if (!inputAudioBlock) {
		return;
	}
//	else {
//		transmit(inputAudioBlock, 0);
//		release(inputAudioBlock);
//		return;
//	}

	if (m_callCount < 1024) {
		if (inputAudioBlock) {
			transmit(inputAudioBlock, 0);
			release(inputAudioBlock);
		}
		m_callCount++; return;
	}

//	else if (m_callCount > 1400) {
//		if (inputAudioBlock) release(inputAudioBlock);
//		return;
//	}
	m_callCount++;
	Serial.println(String("AudioEffectAnalgDelay::update: ") + m_callCount);


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

		// check to see if the queue has reached full size yet
		//Serial.println(String("memory queues:") + memory->getNumQueues() + String(" m_numQueues:") + m_numQueues);
		if (memory->getNumQueues() >= m_numQueues) {
			release(memory->pop());
		}

		// now push in the newest audio block
		//Serial.println(String("push ") + (uint32_t)inputAudioBlock);
		memory->push(inputAudioBlock); // MemAudioBlock supports nullptrs, no need to check
		//Serial.println("done memory->push()");

//		audio_block_t *output = memory->getQueueBack();
//		Serial.println("got the output");
//		if (output) {
//			transmit(output, 0);
//		}
//		Serial.println("Done transmit");
	}
//	return;


	//Serial.print("Active channels: "); Serial.print(m_activeChannels, HEX); Serial.println("");


	// For each active channel, output the delayed audio
	audio_block_t *blockToOutput = nullptr;
	for (int channel=0; channel<MAX_DELAY_CHANNELS; channel++) {
		if (!(m_activeChannels & (1<<channel))) continue; // If this channel is disable, skip to the next;

		if (!m_externalMemory) {
			// internal memory
			MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);
			QueuePosition queuePosition = calcQueuePosition(m_channelOffsets[channel]);
			Serial.println(String("Position info: ") + queuePosition.index + " : " + queuePosition.offset);
			if (queuePosition.offset == 0) {
				// only one queue needs to be read out
				//Serial.println(String("Directly sending queue offset ") + queuePosition.index);
				blockToOutput= memory->getQueueBack(queuePosition.index); // could be nullptr!
				if (blockToOutput) transmit(blockToOutput);
				continue;
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
		MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);

		QueuePosition queuePosition = calcQueuePosition(milliseconds);
		Serial.println(String("CONFIG(") + memory->getMaxSize() + String("): delay: queue position ") + queuePosition.index + String(":") + queuePosition.offset);
		// we have to take the queue position and add 1 to get the number of queues. But we need to add one more since this
		// is the start of the audio buffer, and the AUDIO_BLOCK_SAMPLES will then flow into the next one, so add 2 overall.
		size_t numQueues = queuePosition.index+2;
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

bool AudioEffectAnalogDelay::delay(unsigned channel,  size_t delaySamples)
{
	if (channel > MAX_DELAY_CHANNELS-1) // channel id too high
		return false;

	if (!m_externalMemory) {
		// Internal memory (AudioStream buffers)
		MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);

		//QueuePosition queuePosition = calcQueuePosition(milliseconds);
		QueuePosition queuePosition = calcQueuePosition(delaySamples);
		Serial.println(String("CONFIG(") + memory->getMaxSize() + String("): delay: queue position ") + queuePosition.index + String(":") + queuePosition.offset);
		// we have to take the queue position and add 1 to get the number of queues. But we need to add one more since this
		// is the start of the audio buffer, and the AUDIO_BLOCK_SAMPLES will then flow into the next one, so add 2 overall.
		size_t numQueues = queuePosition.index+2;
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


