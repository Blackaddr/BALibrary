
#include <cstring>
#include <new>

#include "LibMemoryManagement.h"

namespace BAGuitar {

bool ExternalSramManager::m_configured = false;
MemConfig ExternalSramManager::m_memConfig[BAGuitar::NUM_MEM_SLOTS];


/////////////////////////////////////////////////////////////////////////////
// MEM SLOT
/////////////////////////////////////////////////////////////////////////////
bool ExtMemSlot::clear()
{
	if (!m_valid) { return false; }
	m_spi->zero16(m_start, m_size);
	return true;
}

bool ExtMemSlot::write16(size_t offset, int16_t *dataPtr, size_t dataSize)
{
	if (!m_valid) { return false; }
	size_t writeStart = m_start + offset;
	if ((writeStart + dataSize-1) <= m_end) {
		m_spi->write16(writeStart, reinterpret_cast<uint16_t*>(dataPtr), dataSize); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool ExtMemSlot::zero16(size_t offset, size_t dataSize)
{
	if (!m_valid) { return false; }
	size_t writeStart = m_start + offset;
	if ((writeStart + dataSize-1) <= m_end) {
		m_spi->zero16(writeStart, dataSize); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool ExtMemSlot::read16(int16_t *dest, size_t srcOffset, size_t numData)
{
	if (!dest) return false; // invalid destination
	size_t readOffset = m_start + srcOffset;

	if ((readOffset + (numData*sizeof(int16_t))-1) <= m_end) {
		m_spi->read16(readOffset, reinterpret_cast<uint16_t*>(dest), numData);
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

size_t ExtMemSlot::read16FromCurrent(int16_t *dest, size_t currentOffset, size_t numData)
{
	size_t readStart;
	if (m_currentPosition + currentOffset <= m_end) {
		readStart = m_currentPosition + currentOffset;
	} else {
		// this offset will wrap the memory slot
		size_t numBytesToEnd = m_end - m_currentPosition + 1;
		readStart = m_start + (currentOffset - numBytesToEnd);
	}
	m_spi->read16(dest, readStart, numData);
}

bool ExtMemSlot::writeAdvance16(int16_t *dataPtr, size_t dataSize)
{
	if (!m_valid) { return false; }
	if (m_currentPosition + dataSize-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->write16(m_currentPosition, reinterpret_cast<uint16_t*>(dataPtr), dataSize); // cast audio data to uint.
		m_currentPosition += dataSize;

	} else {
		// this write will wrap the memory slot
		size_t numBytes = m_end - m_currentPosition + 1;
		m_spi->write16(m_currentPosition, reinterpret_cast<uint16_t*>(dataPtr), numBytes);
		size_t remainingBytes = dataSize - numBytes; // calculate the remaining bytes
		m_spi->write16(m_start, reinterpret_cast<uint16_t*>(dataPtr + numBytes), remainingBytes); // write remaining bytes are start
		m_currentPosition = m_start + remainingBytes;
	}
	return true;
}

bool ExtMemSlot::zeroAdvance16(size_t dataSize)
{
	if (!m_valid) { return false; }
	if (m_currentPosition + dataSize-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->zero16(m_currentPosition, dataSize); // cast audio data to uint.
		m_currentPosition += dataSize;

	} else {
		// this write will wrap the memory slot
		size_t numBytes = m_end - m_currentPosition + 1;
		m_spi->zero16(m_currentPosition, numBytes);
		size_t remainingBytes = dataSize - numBytes; // calculate the remaining bytes
		m_spi->zero16(m_start, remainingBytes); // write remaining bytes are start
		m_currentPosition = m_start + remainingBytes;
	}
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// EXTERNAL SRAM MANAGER
/////////////////////////////////////////////////////////////////////////////
ExternalSramManager::ExternalSramManager(unsigned numMemories)
{
	// Initialize the static memory configuration structs
	if (!m_configured) {
		for (unsigned i=0; i < NUM_MEM_SLOTS; i++) {
			m_memConfig[i].size           = MEM_MAX_ADDR[i];
			m_memConfig[i].totalAvailable = MEM_MAX_ADDR[i];
			m_memConfig[i].nextAvailable  = 0;
			m_memConfig[i].m_spi          = new BAGuitar::BASpiMemory(static_cast<BAGuitar::SpiDeviceId>(i));
		}
		m_configured = true;
	}
}

ExternalSramManager::~ExternalSramManager()
{
	for (unsigned i=0; i < NUM_MEM_SLOTS; i++) {
		if (m_memConfig[i].m_spi) { delete m_memConfig[i].m_spi; }
	}
}

size_t ExternalSramManager::availableMemory(BAGuitar::MemSelect mem)
{
	return m_memConfig[mem].totalAvailable;
}

bool ExternalSramManager::requestMemory(ExtMemSlot &slot, float delayMilliseconds, BAGuitar::MemSelect mem)
{
	// convert the time to numer of samples
	size_t delayLengthInt = (size_t)((delayMilliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return requestMemory(slot, delayLengthInt, mem);
}

bool ExternalSramManager::requestMemory(ExtMemSlot &slot, size_t sizeBytes, BAGuitar::MemSelect mem)
{
	if (m_memConfig[mem].totalAvailable >= sizeBytes) {
		// there is enough available memory for this request
		slot.m_start = m_memConfig[mem].nextAvailable;
		slot.m_end   = slot.m_start + sizeBytes -1;
		slot.m_currentPosition = slot.m_start; // init to start of slot
		slot.m_size = sizeBytes;
		slot.m_spi = m_memConfig[mem].m_spi;

		// Update the mem config
		m_memConfig[mem].nextAvailable   = slot.m_end+1;
		m_memConfig[mem].totalAvailable -= sizeBytes;
		slot.m_valid = true;
		return true;
	} else {
		// there is not enough memory available for the request
		return false;
	}
}

}

