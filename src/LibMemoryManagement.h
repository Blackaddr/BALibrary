/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  LibMemoryManagment is a class for providing access to external SPI based
 *  SRAM with the optional convience of breaking it up into 'slots' which are smaller
 *  memory entities.
 *  @details This class treats an external memory as a pool from which the user requests
 *  a block. When using that block, the user need not be concerned with where the pool
 *  it came from, they only deal with offsets from the start of their memory block. Ie.
 *  Your particular slot of memory appears to you to start at 0 and end at whatever size
 *  was requested.
 *
 *  @copyright This program is free software: you can redistribute it and/or modify
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
 *****************************************************************************/

#ifndef __BALIBRARY_LIBMEMORYMANAGEMENT_H
#define __BALIBRARY_LIBMEMORYMANAGEMENT_H

#include <cstddef>

#include "BAHardware.h"
#include "BASpiMemory.h"

namespace BALibrary {

/**************************************************************************//**
 * MemConfig contains the configuration information associated with a particular
 * SPI interface.
 *****************************************************************************/
struct MemConfig {
	size_t size;                  ///< the total size of the external SPI memory
	size_t totalAvailable;        ///< the number of bytes available (remaining)
	size_t nextAvailable;         ///< the starting point for the next available slot
	BASpiMemory *m_spi = nullptr; ///< handle to the SPI interface
};

class ExternalSramManager; // forward declare so ExtMemSlot can declared friendship with it

/**************************************************************************//**
 * ExtMemSlot provides a convenient interface to a particular slot of an
 * external memory.
 * @details the memory can be access randomly, as a single word, as a block of
 * data, or as circular queue.
 *****************************************************************************/
class ExtMemSlot {
public:

	/// clear the entire contents of the slot by writing zeros
	/// @returns true on success
	bool clear();

	/// set a new write position (in bytes) for circular operation
	/// @param offsetBytes moves the write pointer to the specified offset from the slot start
	/// @returns true on success, else false if offset is beyond slot boundaries.
	bool setWritePosition(size_t offsetBytes);

	/// returns the currently set write pointer pointer
	/// @returns the write position value
	size_t getWritePosition() const { return m_currentWrPosition-m_start; }

	/// set a new read position (in bytes) for circular operation
	/// @param offsetBytes moves the read pointer to the specified offset from the slot start
	/// @returns true on success, else false if offset is beyond slot boundaries.
	bool setReadPosition(size_t offsetBytes);

	/// returns the currently set read pointer pointer
	/// @returns the read position value
	size_t getReadPosition() const { return m_currentRdPosition-m_start; }

	/// Write a block of 16-bit data to the memory at the specified offset
	/// @param offsetWords offset in 16-bit words from start of slot
	/// @param src pointer to start of block of 16-bit data
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool write16(size_t offsetWords, int16_t *src, size_t numWords);

	/// Write a block of zeros (16-bit) to the memory at the specified offset
	/// @param offsetWords offset in 16-bit words from start of slot
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool zero16(size_t offsetWords, size_t numWords);

	/// Read a block of 16-bit data from the memory at the specified location
	/// @param offsetWords offset in 16-bit words from start of slot
	/// @param dest pointer to destination for the read data
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool read16(size_t offsetWords, int16_t *dest, size_t numWords);

	/// Read the next in memory during circular operation
	/// @returns the next 16-bit data word in memory
	uint16_t readAdvance16();


	/// Read the next block of numWords during circular operation
	/// @details, dest is ignored when using DMA
	/// @param dest pointer to the destination of the read.
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool readAdvance16(int16_t *dest, size_t numWords);

	/// Write a block of 16-bit data from the specified location in circular operation
	/// @param src pointer to the start of the block of data to write to memory
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool writeAdvance16(int16_t *src, size_t numWords);

	/// Write a single 16-bit data to the next location in circular operation
	/// @param data the 16-bit word to transfer
	/// @returns true on success, else false on error
	bool writeAdvance16(int16_t data); // write just one data

	/// Write a block of 16-bit data zeros in circular operation
	/// @param numWords number of 16-bit words to transfer
	/// @returns true on success, else false on error
	bool zeroAdvance16(size_t numWords);

	/// Get the size of the memory slot
	/// @returns size of the slot in bytes
	size_t size() const { return m_size; }

	/// Ensures the underlying SPI interface is enabled
	/// @returns true on success, false on error
	bool enable() const;

	/// Checks whether underlying SPI interface is enabled
	/// @returns true if enabled, false if not enabled
	bool isEnabled() const;

	bool isUseDma() const { return m_useDma; }

	bool isWriteBusy() const;

	bool isReadBusy() const;

	BASpiMemory *getSpiMemoryHandle() { return m_spi; }

	/// DEBUG USE: prints out the slot member variables
	void printStatus(void) const;

private:
	friend ExternalSramManager;     ///< gives the manager access to the private variables
	bool   m_valid = false;         ///< After a slot is successfully configured by the manager it becomes valid
	size_t m_start = 0;             ///< the external memory address in bytes where this slot starts
	size_t m_end = 0;               ///< the external memory address in bytes where this slot ends (inclusive)
	size_t m_currentWrPosition = 0; ///< current write pointer for circular operation
	size_t m_currentRdPosition = 0; ///< current read pointer for circular operation
	size_t m_size = 0;              ///< size of this slot in bytes
	bool   m_useDma = false;        ///< when TRUE, BASpiMemoryDMA will be used.
	SpiDeviceId m_spiId;            ///< the SPI Device ID
	BASpiMemory *m_spi = nullptr;   ///< pointer to an instance of the BASpiMemory interface class
};


/**************************************************************************//**
 * ExternalSramManager provides a class to handle dividing an external SPI RAM
 * into independent slots for general use.
 * @details the class does not support deallocated memory because this would cause
 * fragmentation.
 *****************************************************************************/
class ExternalSramManager final {
public:
	ExternalSramManager();

	/// The manager is constructed by specifying how many external memories to handle allocations for
	/// @param numMemories the number of external memories
	ExternalSramManager(unsigned numMemories);
	virtual ~ExternalSramManager();

	/// Query the amount of available (unallocated) memory
	/// @details note that currently, memory cannot be allocated.
	/// @param mem specifies which memory to query, default is memory 0
	/// @returns the available memory in bytes
	size_t availableMemory(BALibrary::MemSelect mem = BALibrary::MemSelect::MEM0);

	/// Request memory be allocated for the provided slot
	/// @param slot a pointer to the global slot object to which memory will be allocated
	/// @param delayMilliseconds request the amount of memory based on required time for audio samples, rather than number of bytes.
	/// @param mem specify which external memory to allocate from
	/// @param useDma when true, DMA is used for SPI port, else transfers block until complete
	/// @returns true on success, otherwise false on error
	bool requestMemory(ExtMemSlot *slot, float delayMilliseconds, BALibrary::MemSelect mem = BALibrary::MemSelect::MEM0, bool useDma = false);

	/// Request memory be allocated for the provided slot
	/// @param slot a pointer to the global slot object to which memory will be allocated
	/// @param sizeBytes request the amount of memory in bytes to request
	/// @param mem specify which external memory to allocate from
    /// @param useDma when true, DMA is used for SPI port, else transfers block until complete
	/// @returns true on success, otherwise false on error
	bool requestMemory(ExtMemSlot *slot, size_t sizeBytes, BALibrary::MemSelect mem = BALibrary::MemSelect::MEM0, bool useDma = false);

private:
	static bool m_configured; ///< there should only be one instance of ExternalSramManager in the whole project
	static MemConfig m_memConfig[BALibrary::NUM_MEM_SLOTS]; ///< store the configuration information for each external memory
	void m_configure(void); ///< configure the memory manager

};


} // BALibrary

#endif /* __BALIBRARY_LIBMEMORYMANAGEMENT_H */
