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
AudioEffectAnalogDelay::AudioEffectAnalogDelay(ExtMemSlot *slot)
: AudioStream(1, m_inputQueueArray)
{
	m_memory = new AudioDelay(slot);
	m_externalMemory = true;
	for (int i=0; i<MAX_DELAY_CHANNELS; i++) {
		m_channelOffsets[i] = 0;
	}
}

AudioEffectAnalogDelay::~AudioEffectAnalogDelay()
{
	if (m_memory) delete m_memory;
}

void AudioEffectAnalogDelay::update(void)
{
	audio_block_t *inputAudioBlock = receiveReadOnly(); // get the next block of input samples

	if (m_callCount < 1024) {
		if (inputAudioBlock) {
			transmit(inputAudioBlock, 0);
			release(inputAudioBlock);
		}
		m_callCount++; return;
	}


	m_callCount++;
	Serial.println(String("AudioEffectAnalgDelay::update: ") + m_callCount);

	audio_block_t *blockToRelease = m_memory->addBlock(inputAudioBlock);
	if (blockToRelease) release(blockToRelease);

	Serial.print("Active channels: "); Serial.print(m_activeChannels, HEX); Serial.println("");

//	if (inputAudioBlock) {
//		transmit(inputAudioBlock, 0);
//		release(inputAudioBlock);
//	}
//	return;

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
			// external memory
			blockToOutput = allocate(); // allocate always for external memory
		}

		// copy the output data
		if (!blockToOutput) continue; // skip this channel due to failure
		// copy over data
		m_memory->getSamples(blockToOutput, m_channelOffsets[channel]);

		transmit(blockToOutput);
		release(blockToOutput);
	}
}

bool AudioEffectAnalogDelay::delay(unsigned channel, float milliseconds)
{
	if (channel > MAX_DELAY_CHANNELS-1) // channel id too high
		return false;

	size_t delaySamples = calcAudioSamples(milliseconds);

	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
		QueuePosition queuePosition = calcQueuePosition(milliseconds);
		Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();

		if (!slot) { Serial.println("ERROR: slot ptr is not valid"); }
		if (!slot->isEnabled()) {
			slot->enable();
			Serial.println("WEIRD: slot was not enabled");
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

	if (!m_memory) { Serial.println("delay(): m_memory is not valid"); }

	if (!m_externalMemory) {
		// internal memory
		QueuePosition queuePosition = calcQueuePosition(delaySamples);
		Serial.println(String("CONFIG: delay:") + delaySamples + String(" queue position ") + queuePosition.index + String(":") + queuePosition.offset);
	} else {
		// external memory
		Serial.println(String("CONFIG: delay:") + delaySamples);
		ExtMemSlot *slot = m_memory->getSlot();
		if (!slot->isEnabled()) {
			slot->enable();
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


