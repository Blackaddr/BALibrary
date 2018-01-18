/*
 * AudioEffectAnalogDelay.cpp
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */
#include <new>
#include "AudioEffectAnalogDelay.h"

namespace BAGuitar {

AudioEffectAnalogDelay::AudioEffectAnalogDelay(float maxDelay)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(maxDelay);
	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
		m_channelOffsets[i] = 0;
	}
}

AudioEffectAnalogDelay::AudioEffectAnalogDelay(size_t numSamples)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(numSamples);
	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
		m_channelOffsets[i] = 0;
	}
}

// requires preallocated memory large enough
AudioEffectAnalogDelay::AudioEffectAnalogDelay(ExtMemSlot &slot)
: AudioStream(1, m_inputQueueArray)
{
//	m_memory = &slot;
//	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
//		m_channelOffsets[i] = 0;
//	}
//	m_memory->clear();
}

void AudioEffectAnalogDelay::update(void)
{
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

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

	audio_block_t *blockToRelease = m_memory->addBlock(inputAudioBlock);
	if (blockToRelease) release(blockToRelease);


	//Serial.print("Active channels: "); Serial.print(m_activeChannels, HEX); Serial.println("");


	// For each active channel, output the delayed audio
	audio_block_t *blockToOutput = nullptr;
	for (int channel=0; channel<MAX_DELAY_CHANNELS; channel++) {
		if (!(m_activeChannels & (1<<channel))) continue; // If this channel is disable, skip to the next;

		if (!m_externalMemory) {
			// internal memory
			QueuePosition queuePosition = calcQueuePosition(m_channelOffsets[channel]);
			Serial.println(String("Position info: ") + queuePosition.index + " : " + queuePosition.offset);
			if (queuePosition.offset == 0) {
				// only one queue needs to be read out
				//Serial.println(String("Directly sending queue offset ") + queuePosition.index);
				blockToOutput= m_memory->getBlock(queuePosition.index); // could be nullptr!
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
		m_memory->getSamples(blockToOutput, m_channelOffsets[channel]);
		//m_memory->read16(blockToOutput->data, 0, m_channelOffsets[channel], AUDIO_BLOCK_SAMPLES);
		transmit(blockToOutput);
		release(blockToOutput);
	}
}

bool AudioEffectAnalogDelay::delay(unsigned channel, float milliseconds)
{
	if (channel > MAX_DELAY_CHANNELS-1) // channel id too high
		return false;

	size_t delaySamples = calcAudioSamples(milliseconds);

//	if (!m_externalMemory) {
//		// Internal memory (AudioStream buffers)
//
//		QueuePosition queuePosition = calcQueuePosition(milliseconds);
//		Serial.println(String("CONFIG(") + m_memory->getMaxSize() + String("): delay: queue position ") + queuePosition.index + String(":") + queuePosition.offset);
//		// we have to take the queue position and add 1 to get the number of queues. But we need to add one more since this
//		// is the start of the audio buffer, and the AUDIO_BLOCK_SAMPLES will then flow into the next one, so add 2 overall.
//		size_t numQueues = queuePosition.index+2;
//		if (numQueues > m_numQueues)
//			m_numQueues = numQueues;
//	} else {
////		// External memory
////		if (delaySamples > m_memory->getSize()) {
////			// error, the external memory is not large enough
////			return false;
////		}
//	}

	QueuePosition queuePosition = calcQueuePosition(milliseconds);
	Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	m_channelOffsets[channel] = delaySamples;
	m_activeChannels |= 1<<channel;
	return true;
}

bool AudioEffectAnalogDelay::delay(unsigned channel,  size_t delaySamples)
{
	if (channel > MAX_DELAY_CHANNELS-1) // channel id too high
		return false;

//	if (!m_externalMemory) {
//		// Internal memory (AudioStream buffers)
//		MemAudioBlock *memory = reinterpret_cast<MemAudioBlock*>(m_memory);
//
//		//QueuePosition queuePosition = calcQueuePosition(milliseconds);
//		QueuePosition queuePosition = calcQueuePosition(delaySamples);
//		Serial.println(String("CONFIG(") + memory->getMaxSize() + String("): delay: queue position ") + queuePosition.index + String(":") + queuePosition.offset);
//		// we have to take the queue position and add 1 to get the number of queues. But we need to add one more since this
//		// is the start of the audio buffer, and the AUDIO_BLOCK_SAMPLES will then flow into the next one, so add 2 overall.
//		size_t numQueues = queuePosition.index+2;
//		if (numQueues > m_numQueues)
//			m_numQueues = numQueues;
//	} else {
////		// External memory
////		if (delaySamples > m_memory->getSize()) {
////			// error, the external memory is not large enough
////			return false;
////		}
//	}

	QueuePosition queuePosition = calcQueuePosition(delaySamples);
	Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	m_channelOffsets[channel] = delaySamples;
	m_activeChannels |= 1<<channel;
	return true;
}

void AudioEffectAnalogDelay::disable(unsigned channel)
{
	m_activeChannels &= ~(1<<channel);
}

}


