/*
 * BAAudioControlWM8731.h
 *
 *  Created on: November 1, 2017
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

#include "BAAudioEffectDelayExternal.h"

namespace BAGuitar {


#define SPISETTING SPISettings(20000000, MSBFIRST, SPI_MODE0)

struct MemSpiConfig {
	unsigned mosiPin;
	unsigned misoPin;
	unsigned sckPin;
	unsigned csPin;
	unsigned memSize;
};

constexpr MemSpiConfig Mem0Config = {7,  8, 14, 15, 65536 };
constexpr MemSpiConfig Mem1Config = {21, 5, 20, 31, 65536 };

unsigned BAAudioEffectDelayExternal::m_usingSPICount[2] = {0,0};

BAAudioEffectDelayExternal::BAAudioEffectDelayExternal()
: AudioStream(1, inputQueueArray)
{
	initialize(MemSelect::MEM0);
}

BAAudioEffectDelayExternal::BAAudioEffectDelayExternal(MemSelect mem)
: AudioStream(1, inputQueueArray)
{
	initialize(mem);
}

void BAAudioEffectDelayExternal::delay(uint8_t channel, float milliseconds) {

	if (channel >= 8) return;
	if (milliseconds < 0.0) milliseconds = 0.0;
	uint32_t n = (milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f;
	n += AUDIO_BLOCK_SAMPLES;
	if (n > memory_length - AUDIO_BLOCK_SAMPLES)
		n = memory_length - AUDIO_BLOCK_SAMPLES;
	delay_length[channel] = n;
	uint8_t mask = activemask;
	if (activemask == 0) m_startUsingSPI(m_spiChannel);
	activemask = mask | (1<<channel);
}

void BAAudioEffectDelayExternal::disable(uint8_t channel) {
	if (channel >= 8) return;
	uint8_t mask = activemask & ~(1<<channel);
	activemask = mask;
	if (mask == 0) m_stopUsingSPI(m_spiChannel);
}

void BAAudioEffectDelayExternal::update(void)
{
	audio_block_t *block;
	uint32_t n, channel, read_offset;

	// grab incoming data and put it into the memory
	block = receiveReadOnly();

	if (block) {
		if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			// a single write is enough
			write(head_offset, AUDIO_BLOCK_SAMPLES, block->data);
			head_offset += AUDIO_BLOCK_SAMPLES;
		} else {
			// write wraps across end-of-memory
			n = memory_length - head_offset;
			write(head_offset, n, block->data);
			head_offset = AUDIO_BLOCK_SAMPLES - n;
			write(0, head_offset, block->data + n);
		}
		release(block);
	} else {
		// if no input, store zeros, so later playback will
		// not be random garbage previously stored in memory
		if (head_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			zero(head_offset, AUDIO_BLOCK_SAMPLES);
			head_offset += AUDIO_BLOCK_SAMPLES;
		} else {
			n = memory_length - head_offset;
			zero(head_offset, n);
			head_offset = AUDIO_BLOCK_SAMPLES - n;
			zero(0, head_offset);
		}
	}

	// transmit the delayed outputs
	for (channel = 0; channel < 8; channel++) {
		if (!(activemask & (1<<channel))) continue;
		block = allocate();
		if (!block) continue;
		// compute the delayed location where we read
		if (delay_length[channel] <= head_offset) {
			read_offset = head_offset - delay_length[channel];
		} else {
			read_offset = memory_length + head_offset - delay_length[channel];
		}
		if (read_offset + AUDIO_BLOCK_SAMPLES <= memory_length) {
			// a single read will do it
			read(read_offset, AUDIO_BLOCK_SAMPLES, block->data);
		} else {
			// read wraps across end-of-memory
			n = memory_length - read_offset;
			read(read_offset, n, block->data);
			read(0, AUDIO_BLOCK_SAMPLES - n, block->data + n);
		}
		transmit(block, channel);
		release(block);
	}
}

uint32_t BAAudioEffectDelayExternal::allocated[2] = {0, 0};

void BAAudioEffectDelayExternal::initialize(MemSelect mem)
{
	uint32_t samples = 0;
	uint32_t memsize, avail;

	activemask = 0;
	head_offset = 0;
	memsize = 65536;
	m_mem = mem;

	switch (mem) {
	case MemSelect::MEM0 :
	{
		samples = Mem0Config.memSize;
		m_spiChannel = 0;
		m_misoPin = Mem0Config.misoPin;
		m_mosiPin = Mem0Config.mosiPin;
		m_sckPin =  Mem0Config.sckPin;
		m_csPin = Mem0Config.csPin;

		SPI.setMOSI(m_mosiPin);
		SPI.setMISO(m_misoPin);
		SPI.setSCK(m_sckPin);
		SPI.begin();
		break;
	}
	case MemSelect::MEM1 :
	{
		m_spiChannel = 1;
		samples = Mem1Config.memSize;
		m_misoPin = Mem1Config.misoPin;
		m_mosiPin = Mem1Config.mosiPin;
		m_sckPin =  Mem1Config.sckPin;
		m_csPin = Mem1Config.csPin;

		SPI1.setMOSI(m_mosiPin);
		SPI1.setMISO(m_misoPin);
		SPI1.setSCK(m_sckPin);
		SPI1.begin();
		break;
	}
	}

	pinMode(m_csPin, OUTPUT);
	digitalWriteFast(m_csPin, HIGH);

	avail = memsize - allocated[mem];

	if (samples > avail) samples = avail;
	memory_begin = allocated[mem];
	allocated[mem] += samples;
	memory_length = samples;

	zero(0, memory_length);
}


void BAAudioEffectDelayExternal::read(uint32_t offset, uint32_t count, int16_t *data)
{
	uint32_t addr = memory_begin + offset;
	addr *= 2;

	switch(m_mem) {
	case MemSelect::MEM0 :
		SPI.beginTransaction(SPISETTING);
		digitalWriteFast(m_csPin, LOW);
		SPI.transfer16((0x03 << 8) | (addr >> 16));
		SPI.transfer16(addr & 0xFFFF);

		while (count) {
			*data++ = (int16_t)(SPI.transfer16(0));
			count--;
		}
		digitalWriteFast(m_csPin, HIGH);
		SPI.endTransaction();
		break;
	case MemSelect::MEM1 :
		SPI1.beginTransaction(SPISETTING);
		digitalWriteFast(m_csPin, LOW);
		SPI1.transfer16((0x03 << 8) | (addr >> 16));
		SPI1.transfer16(addr & 0xFFFF);

		while (count) {
			*data++ = (int16_t)(SPI1.transfer16(0));
			count--;
		}
		digitalWriteFast(m_csPin, HIGH);
		SPI1.endTransaction();
		break;
	}

}

void BAAudioEffectDelayExternal::write(uint32_t offset, uint32_t count, const int16_t *data)
{
	uint32_t addr = memory_begin + offset;

	switch(m_mem) {
	case MemSelect::MEM0 :
		addr *= 2;
		SPI.beginTransaction(SPISETTING);
		digitalWriteFast(m_csPin, LOW);
		SPI.transfer16((0x02 << 8) | (addr >> 16));
		SPI.transfer16(addr & 0xFFFF);
		while (count) {
			int16_t w = 0;
			if (data) w = *data++;
			SPI.transfer16(w);
			count--;
		}
		digitalWriteFast(m_csPin, HIGH);
		SPI.endTransaction();
		break;
	case MemSelect::MEM1 :
		addr *= 2;
		SPI1.beginTransaction(SPISETTING);
		digitalWriteFast(m_csPin, LOW);
		SPI1.transfer16((0x02 << 8) | (addr >> 16));
		SPI1.transfer16(addr & 0xFFFF);
		while (count) {
			int16_t w = 0;
			if (data) w = *data++;
			SPI1.transfer16(w);
			count--;
		}
		digitalWriteFast(m_csPin, HIGH);
		SPI1.endTransaction();
		break;
	}


}

///////////////////////////////////////////////////////////////////
// PRIVATE METHODS
///////////////////////////////////////////////////////////////////
void BAAudioEffectDelayExternal::zero(uint32_t address, uint32_t count) {
	write(address, count, NULL);
}

#ifdef SPI_HAS_NOTUSINGINTERRUPT
inline void BAAudioEffectDelayExternal::m_startUsingSPI(int spiBus) {
	if (spiBus == 0) {
		SPI.usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		SPI1.usingInterrupt(IRQ_SOFTWARE);
	}
	m_usingSPICount[spiBus]++;
}

inline void BAAudioEffectDelayExternal::m_stopUsingSPI(int spiBus) {
	if (m_usingSPICount[spiBus] == 0 || --m_usingSPICount[spiBus] == 0)
	{
		if (spiBus == 0) {
			SPI.notUsingInterrupt(IRQ_SOFTWARE);
		} else if (spiBus == 1) {
			SPI1.notUsingInterrupt(IRQ_SOFTWARE);
		}

	}
}

#else
inline void BAAudioEffectDelayExternal::m_startUsingSPI(int spiBus) {
	if (spiBus == 0) {
		SPI.usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		SPI1.usingInterrupt(IRQ_SOFTWARE);
	}

}
inline void BAAudioEffectDelayExternal::m_stopUsingSPI(int spiBus) {
}

#endif


} /* namespace BAGuitar */
