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
	/// @param memDeviceId specify which MEM to controlw with SpiDeviceId.
	BASpiMemory(SpiDeviceId memDeviceId);
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2)
	/// @param memDeviceId specify which MEM to controlw with SpiDeviceId.
	/// @param speedHz specify the desired speed in Hz.
	BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz);
	virtual ~BASpiMemory();

	void begin(void);

	/// write a single data word to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param data the value to write
	void write(int address, int data);

	void write16(int address, uint16_t data);

	/// read a single 8-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	int  read(int address);

	/// read a single 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	uint16_t  read16(int address);

private:
	SPIClass *m_spi = nullptr;
	SpiDeviceId m_memDeviceId; // the MEM device being control with this instance
	uint8_t m_csPin; // the IO pin number for the CS on the controlled SPI device
	SPISettings m_settings; // the Wire settings for this SPI port

};

class BASpiMemoryException {};

} /* namespace BAGuitar */

#endif /* __SRC_BASPIMEMORY_H */
