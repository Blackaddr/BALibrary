/*
 * LibMemoryManagement.cpp
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
// EXTERNAL SRAM MANAGER
/////////////////////////////////////////////////////////////////////////////
bool ExternalSramManager::m_configured = false;
MemConfig ExternalSramManager::m_memConfig[BALibrary::NUM_MEM_SLOTS];


ExternalSramManager::ExternalSramManager(unsigned numMemories)
{
}

ExternalSramManager::ExternalSramManager()
{

}

ExternalSramManager::~ExternalSramManager()
{
	for (unsigned i=0; i < NUM_MEM_SLOTS; i++) {
		if (m_memConfig[i].m_spi) { delete m_memConfig[i].m_spi; }
	}
}

size_t ExternalSramManager::availableMemory(BALibrary::MemSelect mem)
{
    if (!m_configured) { m_configure(); }
	return m_memConfig[mem].totalAvailable;
}

bool ExternalSramManager::requestMemory(ExtMemSlot *slot, float delayMilliseconds, BALibrary::MemSelect mem, bool useDma)
{
    if (!m_configured) { m_configure(); }
	// convert the time to numer of samples
	size_t delayLengthInt = (size_t)((delayMilliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return requestMemory(slot, delayLengthInt * sizeof(int16_t), mem, useDma);
}

bool ExternalSramManager::requestMemory(ExtMemSlot *slot, size_t sizeBytes, BALibrary::MemSelect mem, bool useDma)
{

    if (!m_configured) { m_configure(); }

	if (m_memConfig[mem].totalAvailable >= sizeBytes) {
		Serial.println(String("Configuring a slot for mem ") + mem);
		// there is enough available memory for this request
		slot->m_start = m_memConfig[mem].nextAvailable;
		slot->m_end   = slot->m_start + sizeBytes -1;
		slot->m_currentWrPosition = slot->m_start; // init to start of slot
		slot->m_currentRdPosition = slot->m_start; // init to start of slot
		slot->m_size = sizeBytes;

		if (!m_memConfig[mem].m_spi) {
		    if (useDma) {
		        m_memConfig[mem].m_spi = new BALibrary::BASpiMemoryDMA(static_cast<BALibrary::SpiDeviceId>(mem));
		        slot->m_useDma = true;
		    } else {
		        m_memConfig[mem].m_spi = new BALibrary::BASpiMemory(static_cast<BALibrary::SpiDeviceId>(mem));
		        slot->m_useDma = false;
		    }
			if (!m_memConfig[mem].m_spi) {
			} else {
				Serial.println("Calling spi begin()");
				m_memConfig[mem].m_spi->begin();
			}
		}
		slot->m_spi = m_memConfig[mem].m_spi;

		// Update the mem config
		m_memConfig[mem].nextAvailable   = slot->m_end+1;
		m_memConfig[mem].totalAvailable -= sizeBytes;
		slot->m_valid = true;
		if (!slot->isEnabled()) { slot->enable(); }
		Serial.println("Clear the memory\n"); Serial.flush();
		slot->clear();
		Serial.println("Done Request memory\n"); Serial.flush();
		return true;
	} else {
		// there is not enough memory available for the request
	    Serial.println(String("ExternalSramManager::requestMemory(): Insufficient memory in slot, request/available: ")
	            + sizeBytes + String(" : ")
	            + m_memConfig[mem].totalAvailable);
		return false;
	}
}

void ExternalSramManager::m_configure(void)
{
    // Initialize the static memory configuration structs
    if (!m_configured) {
        for (unsigned i=0; i < NUM_MEM_SLOTS; i++) {
            m_memConfig[i].size           = BAHardwareConfig.getSpiMemSizeBytes(i);
            m_memConfig[i].totalAvailable = BAHardwareConfig.getSpiMemSizeBytes(i);
            m_memConfig[i].nextAvailable  = 0;

            m_memConfig[i].m_spi = nullptr;
        }
        m_configured = true;
    }
}

}

