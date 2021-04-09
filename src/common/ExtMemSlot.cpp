/*
 * ExtMemSlot.cpp
 *
 *  Created on: Jan 19, 2018
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
#include <cstring>
#include <new>

#include "Audio.h"
#include "LibMemoryManagement.h"

namespace BALibrary {

/////////////////////////////////////////////////////////////////////////////
// MEM SLOT
/////////////////////////////////////////////////////////////////////////////
bool ExtMemSlot::clear()
{
	if (!m_valid) { return false; }
	m_spi->zero16(m_start, m_size);
	return true;
}

bool ExtMemSlot::setWritePosition(size_t offsetBytes)
{
	if (m_start + offsetBytes <= m_end) {
		m_currentWrPosition = m_start + offsetBytes;
		return true;
	} else { return false; }
}

bool ExtMemSlot::write16(size_t offsetWords, int16_t *src, size_t numWords)
{
	if (!m_valid) { return false; }
	size_t writeStart = m_start + sizeof(int16_t)*offsetWords; // 2x because int16 is two bytes per data
	size_t numBytes = sizeof(int16_t)*numWords;
	if ((writeStart + numBytes-1) <= m_end) {
		m_spi->write16(writeStart, reinterpret_cast<uint16_t*>(src), numWords); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool ExtMemSlot::setReadPosition(size_t offsetBytes)
{
	if (m_start + offsetBytes <= m_end) {
		m_currentRdPosition = m_start + offsetBytes;
		return true;
	} else {
		return false;
	}
}

bool ExtMemSlot::zero16(size_t offsetWords, size_t numWords)
{
	if (!m_valid) { return false; }
	size_t writeStart = m_start + sizeof(int16_t)*offsetWords;
	size_t numBytes = sizeof(int16_t)*numWords;
	if ((writeStart + numBytes-1) <= m_end) {
		m_spi->zero16(writeStart, numWords); // cast audio data to uint
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the write
		return false;
	}
}

bool ExtMemSlot::read16(size_t offsetWords, int16_t *dest, size_t numWords)
{
	if (!dest) return false; // invalid destination
	size_t readOffset = m_start + sizeof(int16_t)*offsetWords;
	size_t numBytes = sizeof(int16_t)*numWords;

	if ((readOffset + numBytes-1) <= m_end) {
		m_spi->read16(readOffset, reinterpret_cast<uint16_t*>(dest), numWords);
		return true;
	} else {
		// this would go past the end of the memory slot, do not perform the read
		return false;
	}
}

uint16_t ExtMemSlot::readAdvance16()
{
	uint16_t val = m_spi->read16(m_currentRdPosition);
	if (m_currentRdPosition < m_end-1) {
		m_currentRdPosition +=2; // position is in bytes and we read two
	} else {
		m_currentRdPosition = m_start;
	}
	return val;
}

bool ExtMemSlot::readAdvance16(int16_t *dest, size_t numWords)
{
    if (!m_valid) { return false; }
    size_t numBytes = sizeof(int16_t)*numWords;

    if (m_currentRdPosition + numBytes-1 <= m_end) {
        // entire block fits in memory slot without wrapping
        m_spi->read16(m_currentRdPosition, reinterpret_cast<uint16_t*>(dest), numWords); // cast audio data to uint.
        m_currentRdPosition += numBytes;

    } else {
        // this read will wrap the memory slot
        size_t rdBytes = m_end - m_currentRdPosition + 1;
        size_t rdDataNum = rdBytes >> 1; // divide by two to get the number of data
        m_spi->read16(m_currentRdPosition, reinterpret_cast<uint16_t*>(dest), rdDataNum);
        size_t remainingData = numWords - rdDataNum;
        m_spi->read16(m_start, reinterpret_cast<uint16_t*>(dest + rdDataNum), remainingData); // write remaining bytes are start
        m_currentRdPosition = m_start + (remainingData*sizeof(int16_t));
    }
    return true;
}


bool ExtMemSlot::writeAdvance16(int16_t *src, size_t numWords)
{
	if (!m_valid) { return false; }
	size_t numBytes = sizeof(int16_t)*numWords;

	if (m_currentWrPosition + numBytes-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->write16(m_currentWrPosition, reinterpret_cast<uint16_t*>(src), numWords); // cast audio data to uint.
		m_currentWrPosition += numBytes;

	} else {
		// this write will wrap the memory slot
		size_t wrBytes = m_end - m_currentWrPosition + 1;
		size_t wrDataNum = wrBytes >> 1; // divide by two to get the number of data

		m_spi->write16(m_currentWrPosition, reinterpret_cast<uint16_t*>(src), wrDataNum);
		size_t remainingData = numWords - wrDataNum;

		m_spi->write16(m_start, reinterpret_cast<uint16_t*>(src + wrDataNum), remainingData); // write remaining bytes are start
		m_currentWrPosition = m_start + (remainingData*sizeof(int16_t));
	}

	// If a write transaction landed exactly on the end of the memory, the next position must be
	// manually put back to the start
	if (m_currentWrPosition > m_end) { m_currentWrPosition = m_start; }
	return true;
}


bool ExtMemSlot::zeroAdvance16(size_t numWords)
{
	if (!m_valid) { return false; }
	size_t numBytes = 2*numWords;
	if (m_currentWrPosition + numBytes-1 <= m_end) {
		// entire block fits in memory slot without wrapping
		m_spi->zero16(m_currentWrPosition, numWords); // cast audio data to uint.
		m_currentWrPosition += numBytes;

	} else {
		// this write will wrap the memory slot
		size_t wrBytes = m_end - m_currentWrPosition + 1;
		size_t wrDataNum = wrBytes >> 1;
		m_spi->zero16(m_currentWrPosition, wrDataNum);
		size_t remainingWords = numWords - wrDataNum; // calculate the remaining bytes
		m_spi->zero16(m_start, remainingWords); // write remaining bytes are start
		m_currentWrPosition = m_start + remainingWords*sizeof(int16_t);
	}
	return true;
}


bool ExtMemSlot::writeAdvance16(int16_t data)
{
	if (!m_valid) { return false; }

	m_spi->write16(m_currentWrPosition, static_cast<uint16_t>(data));
	if (m_currentWrPosition < m_end-1) {
		m_currentWrPosition+=2; // wrote two bytes
	} else {
		m_currentWrPosition = m_start;
	}
	return true;
}


bool ExtMemSlot::enable() const
{
	if (m_spi) {
		Serial.println("ExtMemSlot::enable()");
		m_spi->begin();
		return true;
	}
	else {
		Serial.println("ExtMemSlot m_spi is nullptr");
		return false;
	}
}

bool ExtMemSlot::isEnabled() const
{
	if (m_spi) { return m_spi->isStarted(); }
	else return false;
}

bool ExtMemSlot::isWriteBusy() const
{
	if (m_useDma) {
		return (static_cast<BASpiMemoryDMA*>(m_spi))->isWriteBusy();
	} else { return false; }
}

bool ExtMemSlot::isReadBusy() const
{
	if (m_useDma) {
		return (static_cast<BASpiMemoryDMA*>(m_spi))->isReadBusy();
	} else { return false; }
}


void ExtMemSlot::printStatus(void) const
{
	Serial.println(String("valid:") + m_valid + String(" m_start:") + m_start + \
			       String(" m_end:") + m_end + String(" m_currentWrPosition: ") + m_currentWrPosition + \
				   String(" m_currentRdPosition: ") + m_currentRdPosition + \
				   String(" m_size:") + m_size);
}

}

