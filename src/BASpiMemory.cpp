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

#include "Arduino.h"
#include "BASpiMemory.h"

namespace BAGuitar {

// MEM0 Settings
constexpr int SPI_CS_MEM0 = 15;
constexpr int SPI_MOSI_MEM0 = 7;
constexpr int SPI_MISO_MEM0 = 8;
constexpr int SPI_SCK_MEM0 = 14;

// MEM1 Settings
constexpr int SPI_CS_MEM1 = 31;
constexpr int SPI_MOSI_MEM1 = 21;
constexpr int SPI_MISO_MEM1 = 5;
constexpr int SPI_SCK_MEM1 = 20;

// SPI Constants
constexpr int SPI_WRITE_CMD = 0x2;
constexpr int SPI_READ_CMD = 0x3;
constexpr int SPI_ADDR_2_MASK = 0xFF0000;
constexpr int SPI_ADDR_2_SHIFT = 16;
constexpr int SPI_ADDR_1_MASK = 0x00FF00;
constexpr int SPI_ADDR_1_SHIFT = 8;
constexpr int SPI_ADDR_0_MASK = 0x0000FF;




BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId)
{
	m_memDeviceId = memDeviceId;
	m_settings = {20000000, MSBFIRST, SPI_MODE0};
	m_Init();
}

BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz)
{
	m_memDeviceId = memDeviceId;
	m_settings = {speedHz, MSBFIRST, SPI_MODE0};
	m_Init();
}

// Intitialize the correct Arduino SPI interface
void BASpiMemory::m_Init()
{

	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		m_csPin = SPI_CS_MEM0;
		SPI.setMOSI(SPI_MOSI_MEM0);
		SPI.setMISO(SPI_MISO_MEM0);
		SPI.setSCK(SPI_SCK_MEM0);
		SPI.begin();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		m_csPin = SPI_CS_MEM1;
		SPI1.setMOSI(SPI_MOSI_MEM1);
		SPI1.setMISO(SPI_MISO_MEM1);
		SPI1.setSCK(SPI_SCK_MEM1);
		SPI1.begin();
		break;
#endif

	default :
		// unreachable since memDeviceId is an enumerated class
		return;
	}

	pinMode(m_csPin, OUTPUT);
	digitalWrite(m_csPin, HIGH);

}

BASpiMemory::~BASpiMemory() {
}

// Single address write
void BASpiMemory::write(int address, int data)
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		SPI.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI.transfer(SPI_WRITE_CMD);
		SPI.transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
		SPI.transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
		SPI.transfer((address & SPI_ADDR_0_MASK));
		SPI.transfer(data);
		SPI.endTransaction();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		SPI1.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI1.transfer(SPI_WRITE_CMD);
		SPI1.transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
		SPI1.transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
		SPI1.transfer((address & SPI_ADDR_0_MASK));
		SPI1.transfer(data);
		SPI1.endTransaction();
		break;
#endif

	default :
		break;
		// unreachable
	}
	digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::write16(int address, uint16_t data)
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		SPI.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI.transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
		SPI.transfer16(address & 0xFFFF);
		SPI.transfer16(data);
		SPI.endTransaction();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		SPI1.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI1.transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
		SPI1.transfer16(address & 0xFFFF);
		SPI1.transfer16(data);
		SPI1.endTransaction();
		break;
#endif

	default :
		break;
		// unreachable
	}
	digitalWrite(m_csPin, HIGH);
}

// single address read
int BASpiMemory::read(int address)
{

	int data = -1;

	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		SPI.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI.transfer(SPI_READ_CMD);
		SPI.transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
		SPI.transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
		SPI.transfer((address & SPI_ADDR_0_MASK));
		data = SPI.transfer(0);
		SPI.endTransaction();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		SPI1.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI1.transfer(SPI_READ_CMD);
		SPI1.transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
		SPI1.transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
		SPI1.transfer((address & SPI_ADDR_0_MASK));
		data = SPI1.transfer(0);
		SPI1.endTransaction();
		break;
#endif

	default:
		break;
		// unreachable
	}

	digitalWrite(m_csPin, HIGH);
	return data;
}

uint16_t BASpiMemory::read16(int address)
{

	uint16_t data=0;

	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		SPI.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI.transfer16((SPI_READ_CMD << 8) | (address >> 16) );
		SPI.transfer16(address & 0xFFFF);
		data = SPI.transfer16(0);
		SPI.endTransaction();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		SPI1.beginTransaction(m_settings);
		digitalWrite(m_csPin, LOW);
		SPI1.transfer16((SPI_READ_CMD << 8) | (address >> 16) );
		SPI1.transfer16(address & 0xFFFF);
		data = SPI1.transfer16(0);
		SPI1.endTransaction();
		break;
#endif

	default:
		break;
		// unreachable
	}

	digitalWrite(m_csPin, HIGH);
	return data;
}

} /* namespace BAGuitar */
