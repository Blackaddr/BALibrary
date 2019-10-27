/*
 * ParameterAutomation.cpp
 *
 *  Created on: October 23, 2019
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

namespace BALibrary {

BAHardware BAHardwareConfig; // create the global configuration struct

void BAHardware::set(TgaBoard tgaBoard)
{
    m_tgaBoard = tgaBoard;
}
TgaBoard BAHardware::getTgaBoard(void)
{
    return m_tgaBoard;
}

void BAHardware::set(ExpansionBoard expansionBoard)
{
    m_expansionBoard = expansionBoard;
}

ExpansionBoard BAHardware::getExpansionBoard(void)
{
    return m_expansionBoard;
}

void BAHardware::set(MemSelect memSelect, SpiMemoryDefinition spiMem)
{
    switch(memSelect) {
    case MemSelect::MEM0 : m_spiMem0 = spiMem; break;
    case MemSelect::MEM1 : m_spiMem1 = spiMem; break;
    default  :
        break;
    }
}
SpiMemoryDefinition BAHardware::getSpiMemoryDefinition(MemSelect memSelect)
{
    switch(memSelect) {
    case MemSelect::MEM0 : return m_spiMem0; break;
    case MemSelect::MEM1 : return m_spiMem1; break;
    default :
        return m_spiMem0;
    }
}

size_t BAHardware::getSpiMemSizeBytes(unsigned memIndex)
{
    size_t sizeBytes = 0;
    switch(memIndex) {
    case 0  : sizeBytes = m_spiMem0.MEM_SIZE_BYTES; break;
    case 1  : sizeBytes = m_spiMem1.MEM_SIZE_BYTES; break;
    default : break;
    }
    return sizeBytes;
}

size_t BAHardware::getSpiMemSizeBytes(MemSelect memSelect)
{
    size_t sizeBytes = 0;
    unsigned memIndex = static_cast<unsigned>(memSelect);
    switch(memIndex) {
    case 0  : sizeBytes = m_spiMem0.MEM_SIZE_BYTES; break;
    case 1  : sizeBytes = m_spiMem1.MEM_SIZE_BYTES; break;
    default : break;
    }
    return sizeBytes;
}

size_t BAHardware::getSpiMemMaxAddr(unsigned memIndex)
{
    return getSpiMemSizeBytes(memIndex)-1;
}

size_t BAHardware::getSpiMemMaxAddr(MemSelect memSelect)
{
    return getSpiMemSizeBytes(memSelect)-1;
}

}


