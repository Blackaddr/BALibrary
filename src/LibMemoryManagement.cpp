
#include <cstring>
#include <new>

#include "Audio.h"

#include "LibMemoryManagement.h"

namespace BAGuitar {


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

uint16_t ExtMemSlot::readAdvance16()
{
	uint16_t val = m_spi->read16(m_currentRdPosition);
	if (m_currentRdPosition < m_end) {
		m_currentRdPosition++;
	} else {
		m_currentRdPosition = m_start;
	}
	return val;
}


//void ExtMemSlot::read16FromPast(int16_t *dest, size_t currentOffset, size_t numData)
//{
//	size_t readStart;
//	if (m_currentPosition - currentOffset >= m_start) {
//		readStart = m_currentPosition - currentOffset;
//	} else {
//		// this offset will wrap the memory slot
//		size_t numBytesToStart = m_currentPosition;
//		readStart = m_end - (currentOffset - numBytesToStart);
//	}
//	m_spi->read16(readStart, reinterpret_cast<uint16_t*>(dest), numData);
//}

bool ExtMemSlot::writeAdvance16(int16_t *dataPtr, size_t dataSize)
{
	if (!m_valid) { return false; }
	if (m_currentWrPosition + dataSize-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->write16(m_currentWrPosition, reinterpret_cast<uint16_t*>(dataPtr), dataSize); // cast audio data to uint.
		m_currentWrPosition += dataSize;

	} else {
		// this write will wrap the memory slot
		size_t numBytes = m_end - m_currentWrPosition + 1;
		m_spi->write16(m_currentWrPosition, reinterpret_cast<uint16_t*>(dataPtr), numBytes);
		size_t remainingBytes = dataSize - numBytes; // calculate the remaining bytes
		m_spi->write16(m_start, reinterpret_cast<uint16_t*>(dataPtr + numBytes), remainingBytes); // write remaining bytes are start
		m_currentWrPosition = m_start + remainingBytes;
	}
	return true;
}

bool ExtMemSlot::writeAdvance16(int16_t data)
{
	if (!m_valid) { return false; }

	m_spi->write16(m_currentWrPosition, static_cast<uint16_t>(data));
	if (m_currentWrPosition < m_end) {
		m_currentWrPosition++;
	} else {
		m_currentWrPosition = m_start;
	}
	return true;
}

bool ExtMemSlot::zeroAdvance16(size_t dataSize)
{
	if (!m_valid) { return false; }
	if (m_currentWrPosition + dataSize-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->zero16(m_currentWrPosition, dataSize); // cast audio data to uint.
		m_currentWrPosition += dataSize;

	} else {
		// this write will wrap the memory slot
		size_t numBytes = m_end - m_currentWrPosition + 1;
		m_spi->zero16(m_currentWrPosition, numBytes);
		size_t remainingBytes = dataSize - numBytes; // calculate the remaining bytes
		m_spi->zero16(m_start, remainingBytes); // write remaining bytes are start
		m_currentWrPosition = m_start + remainingBytes;
	}
	return true;
}

bool ExtMemSlot::enable() const
{
	if (m_spi) {
		Serial.println("ExtMemSlot::enable()");
		m_spi->begin();
		return true;
	}
	else {
		Serial.println("ExtMemSlot m_spi is nullptr");
		return false;
	}
}

bool ExtMemSlot::isEnabled() const
{
	if (m_spi) { return m_spi->isStarted(); }
	else return false;
}

void ExtMemSlot::printStatus(void) const
{
	Serial.println(String("valid:") + m_valid + String(" m_start:") + m_start + \
			       String(" m_end:") + m_end + String(" m_currentWrPosition: ") + m_currentWrPosition + \
				   String(" m_size:") + m_size);
}


/////////////////////////////////////////////////////////////////////////////
// EXTERNAL SRAM MANAGER
/////////////////////////////////////////////////////////////////////////////
bool ExternalSramManager::m_configured = false;
MemConfig ExternalSramManager::m_memConfig[BAGuitar::NUM_MEM_SLOTS];

ExternalSramManager::ExternalSramManager(unsigned numMemories)
{
	// Initialize the static memory configuration structs
	if (!m_configured) {
		for (unsigned i=0; i < NUM_MEM_SLOTS; i++) {
			m_memConfig[i].size           = MEM_MAX_ADDR[i];
			m_memConfig[i].totalAvailable = MEM_MAX_ADDR[i];
			m_memConfig[i].nextAvailable  = 0;

			m_memConfig[i].m_spi = nullptr;
//			if (i < numMemories) {
//				m_memConfig[i].m_spi = new BAGuitar::BASpiMemory(static_cast<BAGuitar::SpiDeviceId>(i));
//			} else {
//				m_memConfig[i].m_spi = nullptr;
//			}
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

bool ExternalSramManager::requestMemory(ExtMemSlot *slot, float delayMilliseconds, BAGuitar::MemSelect mem)
{
	// convert the time to numer of samples
	size_t delayLengthInt = (size_t)((delayMilliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return requestMemory(slot, delayLengthInt, mem);
}

bool ExternalSramManager::requestMemory(ExtMemSlot *slot, size_t sizeBytes, BAGuitar::MemSelect mem)
{

	if (m_memConfig[mem].totalAvailable >= sizeBytes) {
		Serial.println(String("Configuring a slot for mem ") + mem);
		// there is enough available memory for this request
		slot->m_start = m_memConfig[mem].nextAvailable;
		slot->m_end   = slot->m_start + sizeBytes -1;
		slot->m_currentWrPosition = slot->m_start; // init to start of slot
		slot->m_currentRdPosition = slot->m_start; // init to start of slot
		slot->m_size = sizeBytes;

		if (!m_memConfig[mem].m_spi) {
			m_memConfig[mem].m_spi = new BAGuitar::BASpiMemory(static_cast<BAGuitar::SpiDeviceId>(mem));
			if (!m_memConfig[mem].m_spi) {
				Serial.println("requestMemory: new failed! m_spi is a nullptr");
			} else {
				m_memConfig[mem].m_spi->begin();
			}
		}
		slot->m_spi = m_memConfig[mem].m_spi;

		// Update the mem config
		m_memConfig[mem].nextAvailable   = slot->m_end+1;
		m_memConfig[mem].totalAvailable -= sizeBytes;
		slot->m_valid = true;
		if (!slot->isEnabled()) { slot->enable(); }
		slot->clear();
		return true;
	} else {
		// there is not enough memory available for the request

		return false;
	}
}

}

