/*
 * BAAudioEffectDelayExternal.h
 *
 *  Created on: Nov 5, 2017
 *      Author: slascos
 */

#ifndef __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H
#define __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H

#include <Audio.h>
#include "Arduino.h"
#include "AudioStream.h"
#include "spi_interrupt.h"

#include "BACommon.h"

namespace BAGuitar {

enum MemSelect : unsigned {
	MEM0 = 0,
	MEM1 = 1
};

class BAAudioEffectDelayExternal : public AudioStream
{
public:

	BAAudioEffectDelayExternal();
	BAAudioEffectDelayExternal(BAGuitar::MemSelect type);
	BAAudioEffectDelayExternal(BAGuitar::MemSelect type, float delayLengthMs);
	virtual ~BAAudioEffectDelayExternal();

	void delay(uint8_t channel, float milliseconds);
	void disable(uint8_t channel);
	virtual void update(void);

	static unsigned m_usingSPICount[2];

private:
	void initialize(BAGuitar::MemSelect mem, unsigned delayLength = 1e6);
	void read(uint32_t address, uint32_t count, int16_t *data);
	void write(uint32_t address, uint32_t count, const int16_t *data);
	void zero(uint32_t address, uint32_t count);
	unsigned m_memoryStart;    // the first address in the memory we're using
	unsigned m_memoryLength;   // the amount of memory we're using
	unsigned m_headOffset;     // head index (incoming) data into external memory
	unsigned m_channelDelayLength[8]; // # of sample delay for each channel (128 = no delay)
	unsigned  m_activeMask;      // which output channels are active
	static unsigned m_allocated[2];
	audio_block_t *m_inputQueueArray[1];



	BAGuitar::MemSelect m_mem;
	SPIClass *m_spi = nullptr;
	int m_spiChannel = 0;
	int m_misoPin = 0;
	int m_mosiPin = 0;
	int m_sckPin = 0;
	int m_csPin = 0;

	void m_startUsingSPI(int spiBus);
	void m_stopUsingSPI(int spiBus);
};


} /* namespace BAGuitar */

#endif /* __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H */
