/*
 * ExtMemoryManagement.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#ifndef SRC_LIBMEMORYMANAGEMENT_H_
#define SRC_LIBMEMORYMANAGEMENT_H_

#include <cstddef>


//#include "Audio.h"

#include "BAHardware.h"
#include "BASpiMemory.h"
//#include "LibBasicFunctions.h"

namespace BAGuitar {

struct MemConfig {
	size_t size;
	size_t totalAvailable;
	size_t nextAvailable;
	BASpiMemory *m_spi = nullptr;
};

class ExternalSramManager; // forward declare so ExtMemSlot can setup friendship

class ExtMemSlot {
public:

	bool clear();
	bool setWritePosition(size_t offset) { m_currentWrPosition = m_start + offset; return true;} // TODO add range check
	size_t getWritePosition() const { return m_currentWrPosition-m_start; }

	bool setReadPosition(size_t offset) { m_currentRdPosition = m_start + offset; return true;} // TODO add range check
	size_t getReadPosition() const { return m_currentRdPosition-m_start; }

	bool write16(size_t offset, int16_t *dataPtr, size_t numData);
	bool zero16(size_t offset, size_t numData);
	bool read16(int16_t *dest, size_t srcOffset, size_t numData);

	uint16_t readAdvance16();
	bool writeAdvance16(int16_t *dataPtr, size_t numData);
	bool writeAdvance16(int16_t data); // write just one data
	bool zeroAdvance16(size_t numData);
	//void read16FromPast(int16_t *dest, size_t Offset, size_t numData);
	size_t size() const { return m_size; }
	bool enable() const;
	bool isEnabled() const;
	void printStatus(void) const;

private:
	friend ExternalSramManager;
	bool   m_valid = false;
	size_t m_start = 0;
	size_t m_end = 0;
	size_t m_currentWrPosition = 0;
	size_t m_currentRdPosition = 0;
	size_t m_size = 0;
	SpiDeviceId m_spiId;
	BASpiMemory *m_spi = nullptr;
};


class ExternalSramManager final {
public:
	ExternalSramManager() = delete;
	ExternalSramManager(unsigned numMemories);
	virtual ~ExternalSramManager();

	size_t availableMemory(BAGuitar::MemSelect mem);
	bool requestMemory(ExtMemSlot *slot, float delayMilliseconds, BAGuitar::MemSelect mem = BAGuitar::MemSelect::MEM0);
	bool requestMemory(ExtMemSlot *slot, size_t sizeBytes,        BAGuitar::MemSelect mem = BAGuitar::MemSelect::MEM0);

private:
	static bool m_configured;
	static MemConfig m_memConfig[BAGuitar::NUM_MEM_SLOTS];

};


}

#endif /* SRC_LIBMEMORYMANAGEMENT_H_ */
