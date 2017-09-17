/*
 * BASpiMemory.h
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
#ifndef SRC_BASPIMEMORY_H_
#define SRC_BASPIMEMORY_H_

#include <SPI.h>

namespace BAGuitar {

enum class SpiDeviceId {
	SPI_DEVICE0,
	SPI_DEVICE1
};

class BASpiMemory {
public:
	BASpiMemory() = delete;
	BASpiMemory(SpiDeviceId memDeviceId);
	BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz);
	virtual ~BASpiMemory();

	void write(int address, int data);
	int  read(int address);

private:
	SpiDeviceId m_memDeviceId;
	uint8_t m_csPin;
	SPISettings m_settings;
	void m_Init();

};

class BASpiMemoryException {};

} /* namespace BAGuitar */

#endif /* SRC_BASPIMEMORY_H_ */
