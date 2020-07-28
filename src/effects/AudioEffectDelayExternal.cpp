/*
 * BAAudioEffectDelayExternal.cpp
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
#include "BAHardware.h"
#include "BAAudioEffectDelayExternal.h"

using namespace BALibrary;

namespace BAEffects {

#define SPISETTING SPISettings(20000000, MSBFIRST, SPI_MODE0)

unsigned BAAudioEffectDelayExternal::m_usingSPICount[2] = {0,0};

BAAudioEffectDelayExternal::BAAudioEffectDelayExternal()
: AudioStream(1, m_inputQueueArray)
{
    m_mem = MemSelect::MEM0;
}

BAAudioEffectDelayExternal::BAAudioEffectDelayExternal(MemSelect mem)
: AudioStream(1, m_inputQueueArray)
{
    m_mem = mem;
}

BAAudioEffectDelayExternal::BAAudioEffectDelayExternal(BALibrary::MemSelect mem, float delayLengthMs)
: AudioStream(1, m_inputQueueArray)
{
	unsigned delayLengthInt = (delayLengthMs*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f;
	m_mem = mem;
	m_requestedDelayLength = delayLengthInt;
}

BAAudioEffectDelayExternal::~BAAudioEffectDelayExternal()
{
	if (m_spi) delete m_spi;
}

void BAAudioEffectDelayExternal::delay(uint8_t channel, float milliseconds) {

    if (!m_configured) { initialize(); }

	if (channel >= 8) return;
	if (milliseconds < 0.0) milliseconds = 0.0;
	uint32_t n = (milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f;
	n += AUDIO_BLOCK_SAMPLES;
	if (n > m_memoryLength - AUDIO_BLOCK_SAMPLES)
		n = m_memoryLength - AUDIO_BLOCK_SAMPLES;
	m_channelDelayLength[channel] = n;
	unsigned mask = m_activeMask;
	if (m_activeMask == 0) m_startUsingSPI(m_spiChannel);
	m_activeMask = mask | (1<<channel);
	Serial.print("DelayLengthInt: "); Serial.println(m_requestedDelayLength);
}

void BAAudioEffectDelayExternal::disable(uint8_t channel) {

    if (!m_configured) { initialize(); }

	if (channel >= 8) return;
	uint8_t mask = m_activeMask & ~(1<<channel);
	m_activeMask = mask;
	if (mask == 0) m_stopUsingSPI(m_spiChannel);
}

void BAAudioEffectDelayExternal::update(void)
{

	audio_block_t *block;
	uint32_t n, channel, read_offset;

	// grab incoming data and put it into the memory
	block = receiveReadOnly();

	if (!m_configured) {
	    if (block) {
	        transmit(block);
	        release(block);
	        return;
	    } else { return; }
	}

	if (block) {
		if (m_headOffset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
			// a single write is enough
			write(m_headOffset, AUDIO_BLOCK_SAMPLES, block->data);
			m_headOffset += AUDIO_BLOCK_SAMPLES;
		} else {
			// write wraps across end-of-memory
			n = m_memoryLength - m_headOffset;
			write(m_headOffset, n, block->data);
			m_headOffset = AUDIO_BLOCK_SAMPLES - n;
			write(0, m_headOffset, block->data + n);
		}
		release(block);
	} else {
		// if no input, store zeros, so later playback will
		// not be random garbage previously stored in memory
		if (m_headOffset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
			zero(m_headOffset, AUDIO_BLOCK_SAMPLES);
			m_headOffset += AUDIO_BLOCK_SAMPLES;
		} else {
			n = m_memoryLength - m_headOffset;
			zero(m_headOffset, n);
			m_headOffset = AUDIO_BLOCK_SAMPLES - n;
			zero(0, m_headOffset);
		}
	}

	// transmit the delayed outputs
	for (channel = 0; channel < 8; channel++) {
		if (!(m_activeMask & (1<<channel))) continue;
		block = allocate();
		if (!block) continue;
		// compute the delayed location where we read
		if (m_channelDelayLength[channel] <= m_headOffset) {
			read_offset = m_headOffset - m_channelDelayLength[channel];
		} else {
			read_offset = m_memoryLength + m_headOffset - m_channelDelayLength[channel];
		}
		if (read_offset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
			// a single read will do it
			read(read_offset, AUDIO_BLOCK_SAMPLES, block->data);
		} else {
			// read wraps across end-of-memory
			n = m_memoryLength - read_offset;
			read(read_offset, n, block->data);
			read(0, AUDIO_BLOCK_SAMPLES - n, block->data + n);
		}
		transmit(block, channel);
		release(block);
	}
}

unsigned BAAudioEffectDelayExternal::m_allocated[2] = {0, 0};

void BAAudioEffectDelayExternal::initialize(void)
{
	unsigned samples = 0;
	unsigned memsize = 0, avail = 0;

	m_activeMask = 0;
	m_headOffset = 0;
	//m_mem = mem;

	switch (m_mem) {
	case MemSelect::MEM0 :
	{
		memsize = BAHardwareConfig.getSpiMemSizeBytes(m_mem) / sizeof(int16_t);
		m_spi = &SPI;
		m_spiChannel = 0;
		m_misoPin = SPI0_MISO_PIN;
		m_mosiPin = SPI0_MOSI_PIN;
		m_sckPin =  SPI0_SCK_PIN;
		m_csPin = SPI0_CS_PIN;

		m_spi->setMOSI(m_mosiPin);
		m_spi->setMISO(m_misoPin);
		m_spi->setSCK(m_sckPin);
		m_spi->begin();
		break;
	}
	case MemSelect::MEM1 :
	{
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		memsize = BAHardwareConfig.getSpiMemSizeBytes(m_mem) / sizeof(int16_t);
		m_spi = &SPI1;
		m_spiChannel = 1;
		m_misoPin = SPI1_MISO_PIN;
		m_mosiPin = SPI1_MOSI_PIN;
		m_sckPin =  SPI1_SCK_PIN;
		m_csPin = SPI1_CS_PIN;

		m_spi->setMOSI(m_mosiPin);
		m_spi->setMISO(m_misoPin);
		m_spi->setSCK(m_sckPin);
		m_spi->begin();
#endif
		break;
	}

	}

	pinMode(m_csPin, OUTPUT);
	digitalWriteFast(m_csPin, HIGH);

	avail = memsize - m_allocated[m_mem];

	if (m_requestedDelayLength > avail) {
	    samples = avail;
	} else {
	    samples = m_requestedDelayLength;
	}

	m_memoryStart = m_allocated[m_mem];
	m_allocated[m_mem] += samples;
	m_memoryLength = samples;

	zero(0, m_memoryLength);
	m_configured = true;

}


void BAAudioEffectDelayExternal::read(uint32_t offset, uint32_t count, int16_t *data)
{
	uint32_t addr = m_memoryStart + offset;
	addr *= 2;

	m_spi->beginTransaction(SPISETTING);
	digitalWriteFast(m_csPin, LOW);
	m_spi->transfer16((0x03 << 8) | (addr >> 16));
	m_spi->transfer16(addr & 0xFFFF);

	while (count) {
		*data++ = (int16_t)(m_spi->transfer16(0));
		count--;
	}
	digitalWriteFast(m_csPin, HIGH);
	m_spi->endTransaction();

}

void BAAudioEffectDelayExternal::write(uint32_t offset, uint32_t count, const int16_t *data)
{
	uint32_t addr = m_memoryStart + offset;

	addr *= 2;
	m_spi->beginTransaction(SPISETTING);
	digitalWriteFast(m_csPin, LOW);
	m_spi->transfer16((0x02 << 8) | (addr >> 16));
	m_spi->transfer16(addr & 0xFFFF);
	while (count) {
		int16_t w = 0;
		if (data) w = *data++;
		m_spi->transfer16(w);
		count--;
	}
	digitalWriteFast(m_csPin, HIGH);
	m_spi->endTransaction();

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
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	}
	m_usingSPICount[spiBus]++;
}

inline void BAAudioEffectDelayExternal::m_stopUsingSPI(int spiBus) {
	if (m_usingSPICount[spiBus] == 0 || --m_usingSPICount[spiBus] == 0)
	{
		if (spiBus == 0) {
			m_spi->notUsingInterrupt(IRQ_SOFTWARE);
		} else if (spiBus == 1) {
			m_spi->notUsingInterrupt(IRQ_SOFTWARE);
		}

	}
}

#else
inline void BAAudioEffectDelayExternal::m_startUsingSPI(int spiBus) {
	if (spiBus == 0) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	}

}
inline void BAAudioEffectDelayExternal::m_stopUsingSPI(int spiBus) {
}

#endif


} /* namespace BAEffects */
