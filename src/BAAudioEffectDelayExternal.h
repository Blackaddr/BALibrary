/*
 * BAAudioEffectDelayExternal.h
 *
 *  Created on: Nov 5, 2017
 *      Author: slascos
 */

#ifndef __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H
#define __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H

#include <Audio.h>

namespace BAGuitar {

#include "Arduino.h"
#include "AudioStream.h"
#include "spi_interrupt.h"

enum MemSelect {
	MEM0,
	MEM1,
};

class BAAudioEffectDelayExternal : public AudioStream
{
public:

	BAAudioEffectDelayExternal();
	BAAudioEffectDelayExternal(MemSelect type, float milliseconds=1e6);

	void delay(uint8_t channel, float milliseconds);
	void disable(uint8_t channel);
	virtual void update(void);

private:
	void initialize(MemSelect mem, uint32_t samples);
	void read(uint32_t address, uint32_t count, int16_t *data);
	void write(uint32_t address, uint32_t count, const int16_t *data);
	void zero(uint32_t address, uint32_t count);
	uint32_t memory_begin;    // the first address in the memory we're using
	uint32_t memory_length;   // the amount of memory we're using
	uint32_t head_offset;     // head index (incoming) data into external memory
	uint32_t delay_length[8]; // # of sample delay for each channel (128 = no delay)
	uint8_t  activemask;      // which output channels are active
	//uint8_t  memory_type;     // 0=23LC1024, 1=Frank's Memoryboard
	static uint32_t allocated[2];
	audio_block_t *inputQueueArray[1];

	MemSelect m_mem;
	int m_misoPin;
	int m_mosiPin;
	int m_sckPin;
	int m_csPin;
};


} /* namespace BAGuitar */

#endif /* __BAGUITAR_BAAUDIOEFFECTDELAYEXTERNAL_H */
