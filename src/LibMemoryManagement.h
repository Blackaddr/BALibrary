/*
 * ExtMemoryManagement.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#ifndef SRC_LIBMEMORYMANAGEMENT_H_
#define SRC_LIBMEMORYMANAGEMENT_H_

#include <cstddef>


#include "Audio.h"

#include "BAHardware.h"
#include "BASpiMemory.h"
//#include "LibBasicFunctions.h"

namespace BAGuitar {

struct QueuePosition {
	int offset;
	int index;
};
QueuePosition calcQueuePosition(float milliseconds);
QueuePosition calcQueuePosition(size_t numSamples);
size_t        calcAudioSamples(float milliseconds);

struct MemConfig {
	size_t size;
	size_t totalAvailable;
	size_t nextAvailable;
	BASpiMemory *m_spi = nullptr;
};

class ExternalSramManager; // forward declare so MemSlot can setup friendship

class MemBufferIF {
public:
	size_t getSize() const { return m_size; }
	virtual bool clear() = 0;
	virtual bool write16(size_t offset, int16_t *dataPtr, size_t numData) = 0;
	virtual bool zero16(size_t offset, size_t numData) = 0;
	virtual bool read16(int16_t *dest, size_t destOffset, size_t srcOffset, size_t numData) = 0;
	virtual bool writeAdvance16(int16_t *dataPtr, size_t numData) = 0;
	virtual bool zeroAdvance16(size_t numData) = 0;
	virtual ~MemBufferIF() {}
protected:
	bool m_valid = false;
	size_t m_size = 0;
};

class MemAudioBlock : public MemBufferIF {
public:
	//MemAudioBlock();
	MemAudioBlock() = delete;
	MemAudioBlock(size_t numSamples);
	MemAudioBlock(float milliseconds);

	virtual ~MemAudioBlock();

	bool push(audio_block_t *block);
	audio_block_t *pop();
	audio_block_t *getQueue(size_t index) { return m_queues[index]; }
	size_t getNumQueues() const { return m_queues.size(); }
	bool clear() override;
	bool write16(size_t offset, int16_t *dataPtr, size_t numData) override;
	bool zero16(size_t offset, size_t numSamples) override;
	bool read16(int16_t *dest, size_t destOffset, size_t srcOffset, size_t numSamples);
	bool writeAdvance16(int16_t *dataPtr, size_t numData) override;
	bool zeroAdvance16(size_t numData) override;
private:
	//size_t m_numQueues;
	BAGuitar::RingBuffer <audio_block_t*> m_queues;
	QueuePosition m_currentPosition = {0,0};

};


class MemSlot : public MemBufferIF {
public:

	bool clear() override;
	bool write16(size_t offset, int16_t *dataPtr, size_t numData) override;
	bool zero16(size_t offset, size_t numData) override;
	bool read16(int16_t *dest, size_t destOffset, size_t srcOffset, size_t numData);
	bool writeAdvance16(int16_t *dataPtr, size_t numData) override;
	bool zeroAdvance16(size_t numData) override;

private:
	friend ExternalSramManager;
	size_t m_start;
	size_t m_end;
	size_t m_currentPosition;
	BASpiMemory *m_spi = nullptr;
};


class ExternalSramManager final {
public:
	ExternalSramManager() = delete;
	ExternalSramManager(unsigned numMemories);
	virtual ~ExternalSramManager();

	size_t availableMemory(BAGuitar::MemSelect mem);
	bool requestMemory(MemSlot &slot, float delayMilliseconds, BAGuitar::MemSelect mem = BAGuitar::MemSelect::MEM0);
	bool requestMemory(MemSlot &slot, size_t sizeBytes,        BAGuitar::MemSelect mem = BAGuitar::MemSelect::MEM0);

private:
	static bool m_configured;
	static MemConfig m_memConfig[BAGuitar::NUM_MEM_SLOTS];

};


}

#endif /* SRC_LIBMEMORYMANAGEMENT_H_ */
