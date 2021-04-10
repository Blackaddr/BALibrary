/*
 * BASpiMemory.cpp
 *
 *  Created on: May 22, 2017
 *      Author: slascos
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.*
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stdlib.h"
#include "assert.h"
#include "Arduino.h"
#include "BASpiMemory.h"

namespace BALibrary {

//// MEM0 Settings
//int SPI_CS_MEM0   = SPI0_CS_PIN;
//int SPI_MOSI_MEM0 = SPI0_MOSI_PIN;
//int SPI_MISO_MEM0 = SPI0_MISO_PIN;
//int SPI_SCK_MEM0  = SPI0_SCK_PIN;
//
//#if defined(SPI1_AVAILABLE)
//// MEM1 Settings
//int SPI_CS_MEM1   = SPI1_CS_PIN;
//int SPI_MOSI_MEM1 = SPI1_MOSI_PIN;
//int SPI_MISO_MEM1 = SPI1_MISO_PIN;
//int SPI_SCK_MEM1  = SPI1_SCK_PIN;
//#endif

// SPI Constants
constexpr int SPI_WRITE_MODE_REG = 0x1;
constexpr int SPI_WRITE_CMD = 0x2;
constexpr int SPI_READ_CMD = 0x3;
constexpr int SPI_ADDR_2_MASK = 0xFF0000;
constexpr int SPI_ADDR_2_SHIFT = 16;
constexpr int SPI_ADDR_1_MASK = 0x00FF00;
constexpr int SPI_ADDR_1_SHIFT = 8;
constexpr int SPI_ADDR_0_MASK = 0x0000FF;

constexpr int CMD_ADDRESS_SIZE = 4;
constexpr int MAX_DMA_XFER_SIZE = 0x4000;

constexpr size_t MEM_ALIGNED_ALLOC = 32; // number of bytes to align DMA buffer to

void * dma_aligned_malloc(size_t align, size_t size);
void dma_aligned_free(void * ptr);

BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId)
{
	m_memDeviceId = memDeviceId;
	m_settings = {20000000, MSBFIRST, SPI_MODE0};
}

BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz)
{
	m_memDeviceId = memDeviceId;
	m_settings = {speedHz, MSBFIRST, SPI_MODE0};
}

// Intitialize the correct Arduino SPI interface
void BASpiMemory::begin()
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		m_csPin = SPI0_CS_PIN;
		m_spi = &SPI;
		m_spi->setMOSI(SPI0_MOSI_PIN);
		m_spi->setMISO(SPI0_MISO_PIN);
		m_spi->setSCK(SPI0_SCK_PIN);
		m_spi->begin();
		m_dieBoundary = BAHardwareConfig.getSpiMemoryDefinition(MemSelect::MEM0).DIE_BOUNDARY;
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		m_csPin = SPI1_CS_PIN;
		m_spi = &SPI1;
		m_spi->setMOSI(SPI1_MOSI_PIN);
		m_spi->setMISO(SPI1_MISO_PIN);
		m_spi->setSCK(SPI1_SCK_PIN);
		m_spi->begin();
		m_dieBoundary = BAHardwareConfig.getSpiMemoryDefinition(MemSelect::MEM1).DIE_BOUNDARY;
		break;
#endif

	default :
		// unreachable since memDeviceId is an enumerated class
		return;
	}

	pinMode(m_csPin, OUTPUT);
	digitalWrite(m_csPin, HIGH);
	m_started = true;

}

BASpiMemory::~BASpiMemory() {
}

// Single address write
void BASpiMemory::write(size_t address, uint8_t data)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_WRITE_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));
	m_spi->transfer(data);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

// Sequential write
void BASpiMemory::write(size_t address, uint8_t *src, size_t numBytes)
{
    // Check if this burst will cross the die boundary
    while (numBytes > 0) {
        size_t bytesToWrite = m_bytesToXfer(address, numBytes);
        m_rawWrite(address, src, bytesToWrite);
        address += bytesToWrite;
        numBytes -= bytesToWrite;
        src += bytesToWrite;
    }
}

void BASpiMemory::zero(size_t address, size_t numBytes)
{
    // Check if this burst will cross the die boundary
    while (numBytes > 0) {
        size_t bytesToWrite = m_bytesToXfer(address, numBytes);
        m_rawZero(address, bytesToWrite);
        address += bytesToWrite;
        numBytes -= bytesToWrite;
    }
}

void BASpiMemory::write16(size_t address, uint16_t data)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);
	m_spi->transfer16(data);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::write16(size_t address, uint16_t *src, size_t numWords)
{
    // Check if this burst will cross the die boundary
    size_t numBytes = numWords * sizeof(uint16_t);
    while (numBytes > 0) {
        size_t bytesToWrite = m_bytesToXfer(address, numBytes);
        size_t wordsToWrite = bytesToWrite / sizeof(uint16_t);
        m_rawWrite16(address, src, wordsToWrite);
        address += bytesToWrite;
        numBytes -= bytesToWrite;
        src += wordsToWrite;
    }
}

void BASpiMemory::zero16(size_t address, size_t numWords)
{
    // Check if this burst will cross the die boundary
    size_t numBytes = numWords * sizeof(uint16_t);
    while (numBytes > 0) {
        size_t bytesToWrite = m_bytesToXfer(address, numBytes);
        size_t wordsToWrite = bytesToWrite / sizeof(uint16_t);
        m_rawZero16(address, wordsToWrite);
        address += bytesToWrite;
        numBytes -= bytesToWrite;
    }
}

// single address read
uint8_t BASpiMemory::read(size_t address)
{
	int data;

	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_READ_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));
	data = m_spi->transfer(0);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
	return data;
}

void BASpiMemory::read(size_t address, uint8_t *dest, size_t numBytes)
{
    // Check if this burst will cross the die boundary
    while (numBytes > 0) {
        size_t bytesToRead = m_bytesToXfer(address, numBytes);
        m_rawRead(address, dest, bytesToRead);
        address += bytesToRead;
        numBytes -= bytesToRead;
        dest += bytesToRead;
    }
}

uint16_t BASpiMemory::read16(size_t address)
{

	uint16_t data;
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_READ_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);
	data = m_spi->transfer16(0);
	m_spi->endTransaction();

	digitalWrite(m_csPin, HIGH);
	return data;
}

void BASpiMemory::read16(size_t address, uint16_t *dest, size_t numWords)
{
    // Check if this burst will cross the die boundary
    size_t numBytes = numWords * sizeof(uint16_t);
    while (numBytes > 0) {
        size_t bytesToRead = m_bytesToXfer(address, numBytes);
        size_t wordsToRead = bytesToRead / sizeof(uint16_t);
        m_rawRead16(address, dest, wordsToRead);
        address += bytesToRead;
        numBytes -= bytesToRead;
        dest += wordsToRead;
    }
}

// PRIVATE FUNCTIONS
size_t BASpiMemory::m_bytesToXfer(size_t address, size_t numBytes)
{
    // Check if this burst will cross the die boundary
    size_t bytesToXfer = numBytes;
    if (m_dieBoundary) {
        if ((address < m_dieBoundary) && (address+numBytes > m_dieBoundary)) {
            // split into two xfers
            bytesToXfer = m_dieBoundary-address;
        }
    }
    return bytesToXfer;
}

void BASpiMemory::m_rawWrite(size_t address, uint8_t *src, size_t numBytes)
{
    uint8_t *dataPtr = src;

    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer(SPI_WRITE_CMD);
    m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
    m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
    m_spi->transfer((address & SPI_ADDR_0_MASK));

    for (size_t i=0; i < numBytes; i++) {
        m_spi->transfer(*dataPtr++);
    }
    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::m_rawWrite16(size_t address, uint16_t *src, size_t numWords)
{
    uint16_t *dataPtr = src;

    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
    m_spi->transfer16(address & 0xFFFF);

    for (size_t i=0; i<numWords; i++) {
        m_spi->transfer16(*dataPtr++);
    }

    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::m_rawZero(size_t address, size_t numBytes)
{
    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer(SPI_WRITE_CMD);
    m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
    m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
    m_spi->transfer((address & SPI_ADDR_0_MASK));

    for (size_t i=0; i < numBytes; i++) {
        m_spi->transfer(0);
    }
    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::m_rawZero16(size_t address, size_t numWords)
{
    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
    m_spi->transfer16(address & 0xFFFF);

    for (size_t i=0; i<numWords; i++) {
        m_spi->transfer16(0);
    }

    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::m_rawRead(size_t address, uint8_t *dest, size_t numBytes)
{
    uint8_t *dataPtr = dest;

    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer(SPI_READ_CMD);
    m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
    m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
    m_spi->transfer((address & SPI_ADDR_0_MASK));

    for (size_t i=0; i<numBytes; i++) {
        *dataPtr++ = m_spi->transfer(0);
    }

    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::m_rawRead16(size_t address, uint16_t *dest, size_t numWords)
{
    uint16_t *dataPtr = dest;
    m_spi->beginTransaction(m_settings);
    digitalWrite(m_csPin, LOW);
    m_spi->transfer16((SPI_READ_CMD << 8) | (address >> 16) );
    m_spi->transfer16(address & 0xFFFF);

    for (size_t i=0; i<numWords; i++) {
        *dataPtr++ = m_spi->transfer16(0);
    }

    m_spi->endTransaction();
    digitalWrite(m_csPin, HIGH);
}


/////////////////////////////////////////////////////////////////////////////
// BASpiMemoryDMA
/////////////////////////////////////////////////////////////////////////////
BASpiMemoryDMA::BASpiMemoryDMA(SpiDeviceId memDeviceId)
: BASpiMemory(memDeviceId)
{
	int cs;
	switch (memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		cs = SPI0_CS_PIN;
		m_cs = new ActiveLowChipSelect(cs, m_settings);
		break;
#if defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		cs = SPI1_CS_PIN;
		m_cs = new ActiveLowChipSelect1(cs, m_settings);
		break;
#endif
	default :
		cs = SPI0_CS_PIN;
	}

	// add 4 bytes to buffer for SPI CMD and 3 bytes of address
	m_txCommandBuffer = new uint8_t[CMD_ADDRESS_SIZE];
	m_rxCommandBuffer = new uint8_t[CMD_ADDRESS_SIZE];
	m_txTransfer = new DmaSpi::Transfer[2];
	m_rxTransfer = new DmaSpi::Transfer[2];
}

BASpiMemoryDMA::BASpiMemoryDMA(SpiDeviceId memDeviceId, uint32_t speedHz)
: BASpiMemory(memDeviceId, speedHz)
{
	int cs;
	switch (memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		cs = SPI0_CS_PIN;
		m_cs = new ActiveLowChipSelect(cs, m_settings);
		break;
#if defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		cs = SPI1_CS_PIN;
		m_cs = new ActiveLowChipSelect1(cs, m_settings);
		break;
#endif
	default :
		cs = SPI0_CS_PIN;
	}

	m_txCommandBuffer = new uint8_t[CMD_ADDRESS_SIZE];
	m_rxCommandBuffer = new uint8_t[CMD_ADDRESS_SIZE];
	m_txTransfer = new DmaSpi::Transfer[2];
	m_rxTransfer = new DmaSpi::Transfer[2];
}

BASpiMemoryDMA::~BASpiMemoryDMA()
{
	delete m_cs;
	if (m_txTransfer) delete [] m_txTransfer;
	if (m_rxTransfer) delete [] m_rxTransfer;
	if (m_txCommandBuffer) delete [] m_txCommandBuffer;
	if (m_rxCommandBuffer) delete [] m_txCommandBuffer;
}

void BASpiMemoryDMA::m_setSpiCmdAddr(int command, size_t address, uint8_t *dest)
{
	dest[0] = command;
	dest[1] = ((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	dest[2] = ((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	dest[3] = ((address & SPI_ADDR_0_MASK));
}

void BASpiMemoryDMA::begin(void)
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		m_csPin = SPI0_CS_PIN;
		m_spi = &SPI;
		m_spi->setMOSI(SPI0_MOSI_PIN);
		m_spi->setMISO(SPI0_MISO_PIN);
		m_spi->setSCK(SPI0_SCK_PIN);
		m_spi->begin();
		m_spiDma = new DmaSpiGeneric();
		m_dieBoundary = BAHardwareConfig.getSpiMemoryDefinition(MemSelect::MEM0).DIE_BOUNDARY;
		break;

#if defined(__MK66FX1M0__) // DMA on SPI1 is only supported on T3.6
	case SpiDeviceId::SPI_DEVICE1 :
		m_csPin = SPI1_CS_PIN;
		m_spi = &SPI1;
		m_spi->setMOSI(SPI1_MOSI_PIN);
		m_spi->setMISO(SPI1_MISO_PIN);
		m_spi->setSCK(SPI1_SCK_PIN);
		m_spi->begin();
		m_spiDma = new DmaSpiGeneric(1);
		m_dieBoundary = BAHardwareConfig.getSpiMemoryDefinition(MemSelect::MEM1).DIE_BOUNDARY;
		break;
#endif

	default :
		// unreachable since memDeviceId is an enumerated class
		return;
	}

    m_spiDma->begin();
    m_spiDma->start();
    m_started = true;
}



// SPI must build up a payload that starts the teh CMD/Address first. It will cycle
// through the payloads in a circular buffer and use the transfer objects to check if they
// are done before continuing.
void BASpiMemoryDMA::write(size_t address, uint8_t *src, size_t numBytes)
{
	size_t bytesRemaining = numBytes;
	uint8_t *srcPtr = src;
	size_t nextAddress = address;
    uint8_t *intermediateBuffer = nullptr;

    while ( m_txTransfer[1].busy() || m_txTransfer[0].busy()) { yield(); } // wait until not busy

    // Check for intermediate buffer use
    if (m_dmaCopyBufferSize) {
        // copy to the intermediate buffer;
        intermediateBuffer = m_dmaWriteCopyBuffer;
        memcpy(intermediateBuffer, src, numBytes);
    }

	while (bytesRemaining > 0) {
	    m_txXferCount = m_bytesToXfer(nextAddress, min(bytesRemaining, static_cast<size_t>(MAX_DMA_XFER_SIZE))); // check for die boundary
		m_setSpiCmdAddr(SPI_WRITE_CMD, nextAddress, m_txCommandBuffer);
		m_txTransfer[1] = DmaSpi::Transfer(m_txCommandBuffer, CMD_ADDRESS_SIZE, nullptr, 0, m_cs, TransferType::NO_END_CS);
		m_spiDma->registerTransfer(m_txTransfer[1]);

		while ( m_txTransfer[0].busy() || m_txTransfer[1].busy()) { yield(); } // wait until not busy
		m_txTransfer[0] = DmaSpi::Transfer(srcPtr, m_txXferCount, nullptr, 0, m_cs, TransferType::NO_START_CS, intermediateBuffer, nullptr);
		m_spiDma->registerTransfer(m_txTransfer[0]);
		bytesRemaining -= m_txXferCount;
		srcPtr += m_txXferCount;
		nextAddress += m_txXferCount;
	}
}


void BASpiMemoryDMA::zero(size_t address, size_t numBytes)
{
	size_t bytesRemaining = numBytes;
	size_t nextAddress = address;

	/// TODO: Why can't the T4 zero the memory when a NULLPTR is passed? It seems to write a constant random value.
	/// Perhaps there is somewhere we can set a fill value?
#if defined(__IMXRT1062__)
	static uint8_t zeroBuffer[MAX_DMA_XFER_SIZE];
	memset(zeroBuffer, 0, MAX_DMA_XFER_SIZE);
#else
	uint8_t *zeroBuffer = nullptr;
#endif

	while (bytesRemaining > 0) {
	    m_txXferCount = m_bytesToXfer(nextAddress, min(bytesRemaining, static_cast<size_t>(MAX_DMA_XFER_SIZE))); // check for die boundary

		while ( m_txTransfer[1].busy()) { yield(); } // wait until not busy
		m_setSpiCmdAddr(SPI_WRITE_CMD, nextAddress, m_txCommandBuffer);
		m_txTransfer[1] = DmaSpi::Transfer(m_txCommandBuffer, CMD_ADDRESS_SIZE, nullptr, 0, m_cs, TransferType::NO_END_CS);
		m_spiDma->registerTransfer(m_txTransfer[1]);

		while ( m_txTransfer[0].busy()) { yield(); } // wait until not busy
		//m_txTransfer[0] = DmaSpi::Transfer(nullptr, m_txXferCount, nullptr, 0, m_cs, TransferType::NO_START_CS);
		m_txTransfer[0] = DmaSpi::Transfer(zeroBuffer, m_txXferCount, nullptr, 0, m_cs, TransferType::NO_START_CS);
		m_spiDma->registerTransfer(m_txTransfer[0]);
		bytesRemaining -= m_txXferCount;
		nextAddress += m_txXferCount;
	}
}


void BASpiMemoryDMA::write16(size_t address, uint16_t *src, size_t numWords)
{
	write(address, reinterpret_cast<uint8_t*>(src), sizeof(uint16_t)*numWords);
}

void BASpiMemoryDMA::zero16(size_t address, size_t numWords)
{
	zero(address, sizeof(uint16_t)*numWords);
}


void BASpiMemoryDMA::read(size_t address, uint8_t *dest, size_t numBytes)
{
	size_t bytesRemaining = numBytes;
	uint8_t *destPtr = dest;
	size_t nextAddress = address;
	volatile uint8_t *intermediateBuffer = nullptr;

	// Check for intermediate buffer use
	if (m_dmaCopyBufferSize) {
	    intermediateBuffer = m_dmaReadCopyBuffer;
	}

	while (bytesRemaining > 0) {

	    while ( m_rxTransfer[1].busy() || m_rxTransfer[0].busy()) { yield(); }
	    m_rxXferCount = m_bytesToXfer(nextAddress, min(bytesRemaining, static_cast<size_t>(MAX_DMA_XFER_SIZE))); // check for die boundary
		m_setSpiCmdAddr(SPI_READ_CMD, nextAddress, m_rxCommandBuffer);
		m_rxTransfer[1] = DmaSpi::Transfer(m_rxCommandBuffer, CMD_ADDRESS_SIZE, nullptr, 0, m_cs, TransferType::NO_END_CS);
		m_spiDma->registerTransfer(m_rxTransfer[1]);

		while ( m_rxTransfer[0].busy() || m_rxTransfer[1].busy()) { yield(); }
		m_rxTransfer[0] = DmaSpi::Transfer(nullptr, m_rxXferCount, destPtr, 0, m_cs, TransferType::NO_START_CS, nullptr, intermediateBuffer);
		m_spiDma->registerTransfer(m_rxTransfer[0]);

		bytesRemaining -= m_rxXferCount;
		destPtr += m_rxXferCount;
		nextAddress += m_rxXferCount;
	}
}


void BASpiMemoryDMA::read16(size_t address, uint16_t *dest, size_t numWords)
{
	read(address, reinterpret_cast<uint8_t*>(dest), sizeof(uint16_t)*numWords);
}


bool BASpiMemoryDMA::isWriteBusy(void) const
{
	return (m_txTransfer[0].busy() or m_txTransfer[1].busy());
}

bool BASpiMemoryDMA::isReadBusy(void) const
{
	return (m_rxTransfer[0].busy() or m_rxTransfer[1].busy());
}

bool BASpiMemoryDMA::setDmaCopyBufferSize(size_t numBytes)
{
    if (m_dmaWriteCopyBuffer) {
        dma_aligned_free((void *)m_dmaWriteCopyBuffer);
        m_dmaCopyBufferSize = 0;
    }
    if (m_dmaReadCopyBuffer) {
        dma_aligned_free((void *)m_dmaReadCopyBuffer);
        m_dmaCopyBufferSize = 0;
    }

    if (numBytes > 0) {
        m_dmaWriteCopyBuffer = (uint8_t*)dma_aligned_malloc(MEM_ALIGNED_ALLOC, numBytes);
        if (!m_dmaWriteCopyBuffer) {
            // allocate failed
            m_dmaCopyBufferSize = 0;
            return false;
        }

        m_dmaReadCopyBuffer = (volatile uint8_t*)dma_aligned_malloc(MEM_ALIGNED_ALLOC, numBytes);
        if (!m_dmaReadCopyBuffer) {
            m_dmaCopyBufferSize = 0;
            return false;
        }
        m_dmaCopyBufferSize = numBytes;
    }

    return true;
}

// Some convenience functions to malloc and free aligned memory
// Number of bytes we're using for storing
// the aligned pointer offset
typedef uint16_t offset_t;
#define PTR_OFFSET_SZ sizeof(offset_t)

#ifndef align_up
#define align_up(num, align) \
    (((num) + ((align) - 1)) & ~((align) - 1))
#endif

void * dma_aligned_malloc(size_t align, size_t size)
{
    void * ptr = NULL;

    // We want it to be a power of two since
    // align_up operates on powers of two
    assert((align & (align - 1)) == 0);

    if(align && size)
    {
        /*
         * We know we have to fit an offset value
         * We also allocate extra bytes to ensure we
         * can meet the alignment
         */
        uint32_t hdr_size = PTR_OFFSET_SZ + (align - 1);
        void * p = malloc(size + hdr_size);

        if(p)
        {
            /*
             * Add the offset size to malloc's pointer
             * (we will always store that)
             * Then align the resulting value to the
             * target alignment
             */
            ptr = (void *) align_up(((uintptr_t)p + PTR_OFFSET_SZ), align);

            // Calculate the offset and store it
            // behind our aligned pointer
            *((offset_t *)ptr - 1) =
                (offset_t)((uintptr_t)ptr - (uintptr_t)p);

        } // else NULL, could not malloc
    } //else NULL, invalid arguments

    return ptr;
}

void dma_aligned_free(void * ptr)
{
    assert(ptr);

    /*
    * Walk backwards from the passed-in pointer
    * to get the pointer offset. We convert to an offset_t
    * pointer and rely on pointer math to get the data
    */
    offset_t offset = *((offset_t *)ptr - 1);

    /*
    * Once we have the offset, we can get our
    * original pointer and call free
    */
    void * p = (void *)((uint8_t *)ptr - offset);
    free(p);
}

} /* namespace BALibrary */
