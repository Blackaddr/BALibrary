/*
 * AudioEffectAnalogDelay.h
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */

#ifndef SRC_AUDIOEFFECTANALOGDELAY_H_
#define SRC_AUDIOEFFECTANALOGDELAY_H_

#include <vector>

#include <Audio.h>
#include "LibMemoryManagement.h"

namespace BAGuitar {



class AudioEffectAnalogDelay : public AudioStream {
public:
	static constexpr int MAX_DELAY_CHANNELS = 8;

	AudioEffectAnalogDelay() = delete;
	AudioEffectAnalogDelay(INTERNAL_MEMORY, float maxDelay);
	AudioEffectAnalogDelay(EXTERNAL_MEMORY, MemSlot &slot); // requires sufficiently sized pre-allocated memory
	virtual ~AudioEffectAnalogDelay() {}

	virtual void update(void);
	bool delay(unsigned channel, float milliseconds);
	void disable(unsigned channel);

private:
	audio_block_t *m_inputQueueArray[1];
	unsigned m_activeChannels = 0;
	bool m_externalMemory = false;
	MemBufferIF *m_memory = nullptr;

	size_t m_numQueues = 0;
	size_t m_channelOffsets[MAX_DELAY_CHANNELS];
};

}

#endif /* SRC_AUDIOEFFECTANALOGDELAY_H_ */
