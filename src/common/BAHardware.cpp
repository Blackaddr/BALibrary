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

////////////////////////////////////////////////////////////////////////////////
// DEFAULT SETTINGS - modified appropriate when user calls BAHardware:set()
// Default settings are currently for TGA PRO MKII & Teensy 4
////////////////////////////////////////////////////////////////////////////////
uint8_t USR_LED_ID = 6; ///< Teensy IO number for the user LED.

unsigned BA_EXPAND_NUM_POT  = 3;
unsigned BA_EXPAND_NUM_SW   = 2;
unsigned BA_EXPAND_NUM_LED  = 2;
unsigned BA_EXPAND_NUM_ENC  = 0;

uint8_t BA_EXPAND_POT1_PIN = A0; // 14_A0_TX3_SPDIFOUT
uint8_t BA_EXPAND_POT2_PIN = A1; // 15_A1_RX3_SPDIFIN
uint8_t BA_EXPAND_POT3_PIN = A2; // 16_A2_RX4_SCL1

uint8_t BA_EXPAND_SW1_PIN  = 2;  // 2_OUT2
uint8_t BA_EXPAND_SW2_PIN  = 3;  // 3_LRCLK2
uint8_t BA_EXPAND_LED1_PIN = 4;  // 4_BLCK2
uint8_t BA_EXPAND_LED2_PIN = 5;  // 5_IN2

uint8_t GPIO0 = 2;
uint8_t GPIO1 = 3;
uint8_t GPIO2 = 4;
uint8_t GPIO3 = 5;
uint8_t GPIO4 = 255; // Not available on MKII
uint8_t GPIO5 = 16;
uint8_t GPIO6 = 15;
uint8_t GPIO7 = 14;
uint8_t TP1 = 9;
uint8_t TP2 = 22;

// SPI0
uint8_t SPI0_SCK_PIN  = 13;
uint8_t SPI0_CS_PIN   = 10;
uint8_t SPI0_MISO_PIN = 12;
uint8_t SPI0_MOSI_PIN = 11;

// SPI1 is only available on user breakout board for MKII/T4
uint8_t SPI1_SCK_PIN  = 27;
uint8_t SPI1_CS_PIN   = 38;
uint8_t SPI1_MISO_PIN = 39;
uint8_t SPI1_MOSI_PIN = 26;

BAHardware::BAHardware()
{
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY_MICROMOD) // T4.X
    m_teensyProcessor = TeensyProcessor::TEENSY4;
#elif defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__)
    m_teensyProcessor = TeensyProcessor::TEENSY3;
#else
#error "Only Teensy 4.1, 4.0, 3.6, 3.5, 3.2 are supported"
#endif
}

void BAHardware::set(TgaBoard tgaBoard)
{
    m_tgaBoard        = tgaBoard;

    ////////////////////////////////////////////////////////////////////////////
    // MKII                                                                   //
    ////////////////////////////////////////////////////////////////////////////
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) // T4.X
    if (tgaBoard == TgaBoard::MKII_REV1) {
        // No change from defaults
    }
#endif

#if defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__) // T3.6 or T3.5 or T3.2
    if (tgaBoard == TgaBoard::MKII_REV1) {
        // Uses TEENSY_ADAPTER_T3

        USR_LED_ID = 16;

        BA_EXPAND_POT1_PIN = A16; // 14_A0_TX3_SPDIFOUT
        BA_EXPAND_POT2_PIN = A17; // 15_A1_RX3_SPDIFIN
        BA_EXPAND_POT3_PIN = A18; // 16_A2_RX4_SCL1

        BA_EXPAND_SW1_PIN  = 2;  // 2_OUT2
        BA_EXPAND_SW2_PIN  = 3;  // 3_LRCLK2
        BA_EXPAND_LED1_PIN = 4;  // 4_BLCK2
        BA_EXPAND_LED2_PIN = 6;  // 5_IN2

        GPIO0 = 2;
        GPIO1 = 3;
        GPIO2 = 4;
        GPIO3 = 6;
        GPIO4 = 255; // Not available on MKII
        GPIO5 = 37;
        GPIO6 = 36;
        GPIO7 = 35;
        TP1   = 34;
        TP2   = 33;

        SPI0_SCK_PIN  = 14;
        SPI0_CS_PIN   = 15;
        SPI0_MISO_PIN = 8;
        SPI0_MOSI_PIN = 7;

        SPI1_SCK_PIN  = 20;
        SPI1_CS_PIN   = 31;
        SPI1_MISO_PIN = 5;
        SPI1_MOSI_PIN = 21;
    }
#endif

    ////////////////////////////////////////////////////////////////////////////
    // REVB (Original TGA Pro)                                                //
    ////////////////////////////////////////////////////////////////////////////
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) // T4.X
    if (tgaBoard == TgaBoard::REV_B) {
        // Uses TGA_T4_ADAPTER board

        USR_LED_ID = 2;

        BA_EXPAND_POT1_PIN = A0; // 14_A0_TX3_SPDIFOUT
        BA_EXPAND_POT2_PIN = A1; // 15_A1_RX3_SPDIFIN
        BA_EXPAND_POT3_PIN = A2; // 16_A2_RX4_SCL1

        BA_EXPAND_SW1_PIN  = 3;  // 2_OUT2
        BA_EXPAND_SW2_PIN  = 4;  // 3_LRCLK2
        BA_EXPAND_LED1_PIN = 5;  // 4_BLCK2
        BA_EXPAND_LED2_PIN = 6;  // 5_IN2

        GPIO0 = 3;
        GPIO1 = 4;
        GPIO2 = 5;
        GPIO3 = 6;
        GPIO4 = 17;
        GPIO5 = 16;
        GPIO6 = 15;
        GPIO7 = 14;
        TP1   = 9;
        TP2   = 255; // Not available

        SPI0_SCK_PIN  = 13;
        SPI0_CS_PIN   = 10;
        SPI0_MISO_PIN = 12;
        SPI0_MOSI_PIN = 11;
    }
#endif

#if defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__) // T3.6 or T3.5 or T3.2
    if (tgaBoard == TgaBoard::REV_B) {

        USR_LED_ID = 16;

        BA_EXPAND_POT1_PIN = A16; // 14_A0_TX3_SPDIFOUT
        BA_EXPAND_POT2_PIN = A17; // 15_A1_RX3_SPDIFIN
        BA_EXPAND_POT3_PIN = A18; // 16_A2_RX4_SCL1

        BA_EXPAND_SW1_PIN  = 2;  // 2_OUT2
        BA_EXPAND_SW2_PIN  = 3;  // 3_LRCLK2
        BA_EXPAND_LED1_PIN = 4;  // 4_BLCK2
        BA_EXPAND_LED2_PIN = 6;  // 5_IN2

        GPIO0 = 2;
        GPIO1 = 3;
        GPIO2 = 4;
        GPIO3 = 6;
        GPIO4 = 38;
        GPIO5 = 37;
        GPIO6 = 36;
        GPIO7 = 35;
        TP1   = 34;
        TP2   = 33;

        SPI0_SCK_PIN  = 14;
        SPI0_CS_PIN   = 15;
        SPI0_MISO_PIN = 8;
        SPI0_MOSI_PIN = 7;

        SPI1_SCK_PIN  = 20;
        SPI1_CS_PIN   = 31;
        SPI1_MISO_PIN = 5;
        SPI1_MOSI_PIN = 21;
    }
#endif

    ////////////////////////////////////////////////////////////////////////////
    // REVA (Original TGA Pro)                                                //
    ////////////////////////////////////////////////////////////////////////////
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) // T4.X
    if (tgaBoard == TgaBoard::REV_A) {
        // Uses TGA_T4_ADAPTER board

        USR_LED_ID = 2;

        GPIO0 = 3;
        GPIO1 = 4;
        GPIO2 = 5;
        GPIO3 = 6;
        GPIO4 = 17;
        GPIO5 = 16;
        GPIO6 = 15;
        GPIO7 = 14;
        TP1   = 9;
        TP2   = 255; // Not available

        SPI0_SCK_PIN  = 13;
        SPI0_CS_PIN   = 10;
        SPI0_MISO_PIN = 12;
        SPI0_MOSI_PIN = 11;
    }
#endif

#if defined(__MK66FX1M0__) || defined(__MK64FX512__) || defined(__MK20DX256__) // T3.6 or T3.5 or T3.2
    if (tgaBoard == TgaBoard::REV_A) {
        // REVA did not support Expansion board

        USR_LED_ID = 16;

        GPIO0 = 2;
        GPIO1 = 3;
        GPIO2 = 4;
        GPIO3 = 6;
        GPIO4 = 12;
        GPIO5 = 32;
        GPIO6 = 27;
        GPIO7 = 29;
        TP1   = 34;
        TP2   = 33;

        SPI0_SCK_PIN  = 14;
        SPI0_CS_PIN   = 15;
        SPI0_MISO_PIN = 8;
        SPI0_MOSI_PIN = 7;

        SPI1_SCK_PIN  = 20;
        SPI1_CS_PIN   = 31;
        SPI1_MISO_PIN = 5;
        SPI1_MOSI_PIN = 21;
    }
#endif

    ////////////////////////////////////////////////////////////////////////////
    // Avalon                                                                 //
    ////////////////////////////////////////////////////////////////////////////
#if defined(ARDUINO_TEENSY41) // T4.X
    if (tgaBoard == TgaBoard::AVALON) {
        BA_EXPAND_NUM_POT  = 2;
        BA_EXPAND_NUM_SW   = 6;
        BA_EXPAND_NUM_LED  = 2;
        BA_EXPAND_NUM_ENC  = 4;

        BA_EXPAND_POT1_PIN = A0;
        BA_EXPAND_POT2_PIN = A1;
        BA_EXPAND_POT3_PIN = A13;

        BA_EXPAND_SW1_PIN  = 17;
        BA_EXPAND_SW2_PIN  = 16;
        BA_EXPAND_LED1_PIN = 22;
        BA_EXPAND_LED2_PIN = 32;

        SPI0_SCK_PIN  = 13;
        SPI0_CS_PIN   = 10;
        SPI0_MISO_PIN = 12;
        SPI0_MOSI_PIN = 11;
    }
#endif
}

TgaBoard BAHardware::getTgaBoard(void)
{
    return m_tgaBoard;
}

TeensyProcessor BAHardware::getTeensyProcessor(void)
{
    return m_teensyProcessor;
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


