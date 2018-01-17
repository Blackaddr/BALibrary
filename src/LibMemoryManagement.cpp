
#include <cstring>
#include <new>

#include "LibMemoryManagement.h"

namespace BAGuitar {

bool ExternalSramManager::m_configured = false;
MemConfig ExternalSramManager::m_memConfig[BAGuitar::NUM_MEM_SLOTS];

size_t calcAudioSamples(float milliseconds)
{
	return (size_t)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
}

QueuePosition calcQueuePosition(size_t numSamples)
{
	QueuePosition queuePosition;
	queuePosition.index = (int)(numSamples / AUDIO_BLOCK_SAMPLES);
	queuePosition.offset = numSamples % AUDIO_BLOCK_SAMPLES;
	return queuePosition;
}
QueuePosition calcQueuePosition(float milliseconds) {
	size_t numSamples = (int)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return calcQueuePosition(numSamples);
}

size_t calcOffset(QueuePosition position)
{
	return (position.index*AUDIO_BLOCK_SAMPLES) + position.offset;
}

/////////////////////////////////////////////////////////////////////////////
// MEM BUFFER IF
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// MEM VIRTUAL
/////////////////////////////////////////////////////////////////////////////
MemAudioBlock::MemAudioBlock(size_t numSamples)
: m_queues(((numSamples + AUDIO_BLOCK_SAMPLES - 1)/AUDIO_BLOCK_SAMPLES) +1)
{
//	// round up to an integer multiple of AUDIO_BLOCK_SAMPLES
//	int numQueues = (numSamples + AUDIO_BLOCK_SAMPLES - 1)/AUDIO_BLOCK_SAMPLES;
//
//	// Preload the queue with nullptrs to set the queue depth to the correct size
//	for (int i=0; i < numQueues; i++) {
//		m_queues.push_back(nullptr);
//	}
}

MemAudioBlock::MemAudioBlock(float milliseconds)
: MemAudioBlock(calcAudioSamples(milliseconds))
{
}

MemAudioBlock::~MemAudioBlock()
{

}

// the index is referenced from the head
audio_block_t *MemAudioBlock::getQueueBack(size_t offset)
{

//	for (int i=0; i<m_queues.getMaxSize(); i++) {
//		Serial.println(i + String(":") + (uint32_t)m_queues[i]->data);
//	}
//	Serial.println(String("Returning ") + (uint32_t)m_queues[m_queues.getBackIndex(offset)]);
	return m_queues[m_queues.getBackIndex(offset)];
}

bool MemAudioBlock::push(audio_block_t *block)
{
	//Serial.println("MemAudioBlock::push()");
	m_queues.push_back(block);
	//Serial.println("MemAudioBlock::push() done");
	return true;
}

audio_block_t* MemAudioBlock::pop()
{
	//Serial.println("MemAudioBlock::pop()");
	audio_block_t* block = m_queues.front();
	m_queues.pop_front();
	return block;
}

bool MemAudioBlock::clear()
{
	for (size_t i=0; i < m_queues.size(); i++) {
		if (m_queues[i]) {
			memset(m_queues[i]->data, 0, AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
		}
	}
	return true;
}

bool MemAudioBlock::write16(size_t offset, int16_t *srcDataPtr, size_t numData)
{
	// Calculate the queue position
	auto position = calcQueuePosition(offset);
	int writeOffset = position.offset;
	size_t index  = position.index;

	if ( (index+1) > m_queues.size()) return false; // out of range

	// loop over a series of memcpys until all data is transferred.
	size_t samplesRemaining = numData;

	int16_t *srcStart  = srcDataPtr; // this will increment during each loop iteration

	while (samplesRemaining > 0) {
		size_t numSamplesToWrite;

		void *destStart = static_cast<void*>(m_queues[index]->data + writeOffset);

		// determine if the transfer will complete or will hit the end of a block first
		if ( (writeOffset + samplesRemaining) > AUDIO_BLOCK_SAMPLES ) {
			// goes past end of the queue
			numSamplesToWrite = (AUDIO_BLOCK_SAMPLES - writeOffset);
			//writeOffset = 0;
			//index++;
		} else {
			// transfer ends in this audio block
			numSamplesToWrite = samplesRemaining;
			//writeOffset += numSamplesToWrite;
		}

		// perform the transfer
		if (!m_queues[index]) {
			// no allocated audio block, skip the copy
		} else {
			if (srcDataPtr) {
				memcpy(destStart, static_cast<const void*>(srcStart), numSamplesToWrite * sizeof(int16_t));
			} else {
				memset(destStart, 0, numSamplesToWrite * sizeof(int16_t));
			}
		}
		writeOffset = 0;
		index++;
		srcStart += numSamplesToWrite;
		samplesRemaining -= numSamplesToWrite;
	}
	m_currentPosition.offset = writeOffset;
	m_currentPosition.index  = index;
	return true;
}

inline bool MemAudioBlock::zero16(size_t offset, size_t numData)
{
	return write16(offset, nullptr, numData);
}

bool MemAudioBlock::read16(int16_t *dest, size_t destOffset, size_t srcOffset, size_t numSamples)
{
	if (!dest) return false; // destination is not valid
	(void)destOffset; // not supported with audio_block_t;
	//Serial.println("*************************************************************************");
	//Serial.println(String("read16():") + (uint32_t)dest + String(":") + destOffset + String(":") + srcOffset + String(":") + numSamples);

	// Calculate the queue position
	auto position = calcQueuePosition(srcOffset);
	size_t index  = position.index;

	// Break the transfer in two. Note that the audio is stored first sample (in time) last (in memory).

	int16_t *destStart = dest;
	audio_block_t *currentQueue;
	int16_t *srcStart;

	// Break the transfer into two. Note that the audio
	//Serial.println("Calling getQueue");
	currentQueue = getQueueBack(index+1); // buffer indexes go backwards from the back
	//Serial.println(String("Q. Address: ") + (uint32_t)currentQueue + String(" Data: ") + (uint32_t)currentQueue->data);
	srcStart  = (currentQueue->data + AUDIO_BLOCK_SAMPLES - position.offset);
	size_t numData = position.offset;
	//Serial.println(String("Source Start1: ") + (uint32_t)currentQueue->data + String(" Dest start1: ") + (uint32_t)dest);
	//Serial.println(String("copying to ") + (uint32_t)destStart + String(" from ") + (uint32_t)srcStart + String(" numData= ") + numData);
	memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));


	currentQueue = getQueueBack(index); // buffer indexes go backwards from the back
	//Serial.println(String("Q. Address: ") + (uint32_t)currentQueue + String(" Data: ") + (uint32_t)currentQueue->data);
	destStart += numData;
	srcStart  = (currentQueue->data);
	numData = AUDIO_BLOCK_SAMPLES - numData;
	//Serial.println(String("Source Start2: ") + (uint32_t)currentQueue->data + String(" Dest start2: ") + (uint32_t)dest);
	//Serial.println(String("copying to ") + (uint32_t)destStart + String(" from ") + (uint32_t)srcStart + String(" numData= ") + numData);
	memcpy(static_cast<void*>(destStart), static_cast<void*>(srcStart), numData * sizeof(int16_t));

	//m_queues.print();
	//Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

	return true;
}

// If this function hits the end of the queues it will wrap to the start
bool MemAudioBlock::writeAdvance16(int16_t *dataPtr, size_t numData)
{
	auto globalOffset = calcOffset(m_currentPosition);
	auto end = globalOffset + numData;

	if ( end >= (m_queues.size() * AUDIO_BLOCK_SAMPLES) ) {
		// transfer will wrap, so break into two
		auto samplesToWrite = end - globalOffset;
		// write the first chunk
		write16(globalOffset, dataPtr, samplesToWrite);
		// write the scond chunk
		int16_t *ptr;
		if (dataPtr) {
			// valid dataptr, advance the pointer
			ptr = dataPtr+samplesToWrite;
		} else {
			// dataPtr was nullptr
			ptr = nullptr;
		}
		write16(0, ptr, numData-samplesToWrite);
	} else {
		// no wrap
		write16(globalOffset, dataPtr, numData);
	}
	return true;
}

bool MemAudioBlock::zeroAdvance16(size_t numData)
{
	return writeAdvance16(nullptr, numData);
}

/////////////////////////////////////////////////////////////////////////////
// MEM SLOT
/////////////////////////////////////////////////////////////////////////////
bool MemSlot::clear()
{
	if (!m_valid) { return false; }
	m_spi->zero16(m_start, m_size);
	return true;
}

bool MemSlot::write16(size_t offset, int16_t *dataPtr, size_t dataSize)
{
	if (!m_valid) { return false; }
	if ((offset + dataSize-1) <= m_end) {
		m_spi->write16(offset, reinterpret_cast<uint16_t*>(dataPtr), dataSize); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool MemSlot::zero16(size_t offset, size_t dataSize)
{
	if (!m_valid) { return false; }
	if ((offset + dataSize-1) <= m_end) {
		m_spi->zero16(offset, dataSize); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool MemSlot::read16(int16_t *dest, size_t destOffset, size_t srcOffset, size_t numData)
{
	if (!dest) return false; // invalid destination
	if ((srcOffset + (numData*sizeof(int16_t))-1) <= m_end) {
		m_spi->read16(srcOffset, reinterpret_cast<uint16_t*>(dest), numData);
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool MemSlot::writeAdvance16(int16_t *dataPtr, size_t dataSize)
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

bool MemSlot::zeroAdvance16(size_t dataSize)
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

bool ExternalSramManager::requestMemory(MemSlot &slot, float delayMilliseconds, BAGuitar::MemSelect mem)
{
	// convert the time to numer of samples
	size_t delayLengthInt = (size_t)((delayMilliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return requestMemory(slot, delayLengthInt, mem);
}

bool ExternalSramManager::requestMemory(MemSlot &slot, size_t sizeBytes, BAGuitar::MemSelect mem)
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

