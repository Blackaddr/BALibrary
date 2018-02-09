/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BASpiMemory is convenience class for accessing the optional SPI RAMs on
 *  the GTA Series boards.
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
#ifndef __SRC_BASPIMEMORY_H
#define __SRC_BASPIMEMORY_H

#include <SPI.h>
#include <DmaSpi.h>

#include "BATypes.h"
#include "BAHardware.h"

namespace BAGuitar {

/**************************************************************************//**
 *  This wrapper class uses the Arduino SPI (Wire) library to access the SPI ram.
 *  @details The purpose of this class is primilary for functional testing since
 *  it currently support single-word access. High performance access should be
 *  done using DMA techniques in the Teensy library.
 *****************************************************************************/
class BASpiMemory {
public:
	BASpiMemory() = delete;
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2).
	/// @details default is 20 Mhz
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	BASpiMemory(SpiDeviceId memDeviceId);
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2)
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	/// @param speedHz specify the desired speed in Hz.
	BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz);
	virtual ~BASpiMemory();

	virtual void begin();

	/// write a single data word to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param data the value to write
	void write(size_t address, uint8_t data);
	virtual void write(size_t address, uint8_t *data, size_t numBytes);
	virtual void zero(size_t address, size_t numBytes);

	void write16(size_t address, uint16_t data);
	virtual void write16(size_t address, uint16_t *data, size_t numWords);

	virtual void zero16(size_t address, size_t numWords);

	/// read a single 8-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	uint8_t read(size_t address);
	virtual void read(size_t address, uint8_t *data, size_t numBytes);

	/// read a single 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	uint16_t read16(size_t address);

	/// read a block 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @param dest the pointer to the destination
	/// @param numWords the number of 16-bit words to transfer
	virtual void read16(size_t address, uint16_t *dest, size_t numWords);

    bool isStarted() const { return m_started; }

protected:
	SPIClass *m_spi = nullptr;
	SpiDeviceId m_memDeviceId; // the MEM device being control with this instance
	uint8_t m_csPin; // the IO pin number for the CS on the controlled SPI device
	SPISettings m_settings; // the Wire settings for this SPI port
	bool m_started = false;

};

//constexpr int MAX_DMA_XFERS = 4;

class BASpiMemoryDMA : public BASpiMemory {
	BASpiMemoryDMA() = delete;
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2).
	/// @details default is 20 Mhz
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	/// @param bufferSize size of buffer to store DMA transfers
	BASpiMemoryDMA(SpiDeviceId memDeviceId, size_t bufferSizeBytes);
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2)
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	/// @param speedHz specify the desired speed in Hz.
	/// @param bufferSize size of buffer to store DMA transfers
	BASpiMemoryDMA(SpiDeviceId memDeviceId, uint32_t speedHz, size_t bufferSizeBytes);
	virtual ~BASpiMemoryDMA();

	void write(size_t address, uint8_t *data, size_t numBytes) override;
	void zero(size_t address, size_t numBytes) override;
	void write16(size_t address, uint16_t *data, size_t numWords) override;
	void zero16(size_t address, size_t numWords) override;

	/// read a single 8-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	void read(size_t address, uint8_t *dest, size_t numBytes) override;

	/// read a block 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @param dest the pointer to the destination
	/// @param numWords the number of 16-bit words to transfer
	void read16(size_t address, uint16_t *dest, size_t numWords) override;


	void begin() override;
	void readBufferContents(uint8_t *dest, size_t numBytes, size_t bufferOffset = 0);

private:
	AbstractDmaSpi<DmaSpi0, SPIClass, SPI> *m_spiDma = nullptr;
	ActiveLowChipSelect *m_cs = nullptr;
	size_t m_bufferSize;
	uint8_t *m_txBuffer = nullptr;
	DmaSpi::Transfer *m_txTransfer;
	uint8_t *m_rxBuffer = nullptr;
	DmaSpi::Transfer *m_rxTransfer;

	void m_setSpiCmdAddr(int command, size_t address, uint8_t *dest);
};


} /* namespace BAGuitar */

#endif /* __SRC_BASPIMEMORY_H */
