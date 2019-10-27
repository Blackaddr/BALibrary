/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BAAudioEffectDelayExternal is a class for using an external SPI SRAM chip
 *  as an audio delay line. The external memory can be shared among several
 *  different instances of BAAudioEffectDelayExternal by specifying the max delay
 *  length during construction.
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

#ifndef __BAEFFECTS_BAAUDIOEFFECTDELAYEXTERNAL_H
#define __BAEFFECTS_BAAUDIOEFFECTDELAYEXTERNAL_H

#include <Audio.h>
#include "AudioStream.h"

#include "BAHardware.h"

namespace BAEffects {

/**************************************************************************//**
 * BAAudioEffectDelayExternal can use external SPI RAM for delay rather than
 * the limited RAM available on the Teensy itself.
 *****************************************************************************/
class BAAudioEffectDelayExternal : public AudioStream
{
public:

	/// Default constructor will use all memory available in MEM0
	BAAudioEffectDelayExternal();

	/// Specifiy which external memory to use
	/// @param type specify which memory to use
	BAAudioEffectDelayExternal(BALibrary::MemSelect mem);

	/// Specify external memory, and how much of the memory to use
	/// @param type specify which memory to use
	/// @param delayLengthMs maximum delay length in milliseconds
	BAAudioEffectDelayExternal(BALibrary::MemSelect mem, float delayLengthMs);
	virtual ~BAAudioEffectDelayExternal();

	/// set the actual amount of delay on a given delay tap
	/// @param channel specify channel tap 1-8
	/// @param milliseconds specify how much delay for the specified tap
	void delay(uint8_t channel, float milliseconds);

	/// Disable a delay channel tap
	/// @param channel specify channel tap 1-8
	void disable(uint8_t channel);

	virtual void update(void);

	static unsigned m_usingSPICount[2]; // internal use for all instances

private:
	bool m_configured = false;
	unsigned m_requestedDelayLength = 1e6;
	BALibrary::MemSelect m_mem = BALibrary::MemSelect::MEM0;
	void initialize(void);
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

	SPIClass *m_spi = nullptr;
	int m_spiChannel = 0;
	int m_misoPin = 0;
	int m_mosiPin = 0;
	int m_sckPin = 0;
	int m_csPin = 0;

	void m_startUsingSPI(int spiBus);
	void m_stopUsingSPI(int spiBus);
};


} /* namespace BAEffects */

#endif /* __BAEFFECTS_BAAUDIOEFFECTDELAYEXTERNAL_H */
