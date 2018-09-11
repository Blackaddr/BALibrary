/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  This file contains specific definitions for each Blackaddr Audio hardware
 *  board.
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

#ifndef __BALIBRARY_BAHARDWARE_H
#define __BALIBRARY_BAHARDWARE_H

#include <Arduino.h>
#include <cstdint>
#include <cstddef>

/**************************************************************************//**
 * BALibrary is a namespace/Library for Guitar processing from Blackaddr Audio.
 *****************************************************************************/
namespace BALibrary {

// uncomment the line that corresponds to your hardware
//#define TGA_PRO_REVA
#if (!defined(TGA_PRO_REVA) && !defined(TGA_PRO_REVB))
#define TGA_PRO_REVA
#endif

#if defined(TGA_PRO_REVA) || defined(TGA_PRO_REVB)

constexpr uint8_t USR_LED_ID = 16; ///< Teensy IO number for the user LED.

/**************************************************************************//**
 * GPIOs and Testpoints are accessed via enumerated class constants.
 *****************************************************************************/
enum class GPIO : uint8_t {
	GPIO0 = 2,
	GPIO1 = 3,
	GPIO2 = 4,
	GPIO3 = 6,

	GPIO4 = 12,
	GPIO5 = 32,
	GPIO6 = 27,
	GPIO7 = 28,

	TP1 = 34,
	TP2 = 33
};

/**************************************************************************//**
 * Optionally installed SPI RAM
 *****************************************************************************/
constexpr unsigned NUM_MEM_SLOTS = 2;
enum MemSelect : unsigned {
	MEM0 = 0, ///< SPI RAM MEM0
	MEM1 = 1  ///< SPI RAM MEM1
};

/**************************************************************************//**
 * Set the maximum address (byte-based) in the external SPI memories
 *****************************************************************************/
constexpr size_t MEM_MAX_ADDR[NUM_MEM_SLOTS] = { 131071, 131071 };


/**************************************************************************//**
 * General Purpose SPI Interfaces
 *****************************************************************************/
enum class SpiDeviceId : unsigned {
	SPI_DEVICE0 = 0, ///< Arduino SPI device
	SPI_DEVICE1 = 1  ///< Arduino SPI1 device
};
constexpr int SPI_MAX_ADDR = 131071; ///< Max address size per chip
constexpr size_t SPI_MEM0_SIZE_BYTES = 131072;
constexpr size_t SPI_MEM0_MAX_AUDIO_SAMPLES = SPI_MEM0_SIZE_BYTES/sizeof(int16_t);

constexpr size_t SPI_MEM1_SIZE_BYTES = 131072;
constexpr size_t SPI_MEM1_MAX_AUDIO_SAMPLES = SPI_MEM1_SIZE_BYTES/sizeof(int16_t);



#else

#error "No hardware declared"

#endif

#if defined (TGA_PRO_EXPAND_REV2)
/**************************************************************************//**
 * Blackaddr Audio Expansion Board
 *****************************************************************************/
constexpr unsigned BA_EXPAND_NUM_POT  = 3;
constexpr unsigned BA_EXPAND_NUM_SW   = 2;
constexpr unsigned BA_EXPAND_NUM_LED  = 2;
constexpr unsigned BA_EXPAND_NUM_ENC  = 0;

constexpr uint8_t BA_EXPAND_POT1_PIN = A16; // 35_A16_PWM
constexpr uint8_t BA_EXPAND_POT2_PIN = A17; // 36_A17_PWM
constexpr uint8_t BA_EXPAND_POT3_PIN = A18; // 37_SCL1_A18_PWM
constexpr uint8_t BA_EXPAND_SW1_PIN  = 2;  // 2)PWM
constexpr uint8_t BA_EXPAND_SW2_PIN  = 3;  // 3_SCL2_PWM
constexpr uint8_t BA_EXPAND_LED1_PIN = 4;  // 4_SDA2_PWM
constexpr uint8_t BA_EXPAND_LED2_PIN = 6;  // 6_PWM
#endif

} // namespace BALibrary


#endif /* __BALIBRARY_BAHARDWARE_H */
