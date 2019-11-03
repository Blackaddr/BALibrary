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

// In your Arudino .ino file, use #defines for your TGA Pro revision and options
// to correctly configure your hardware
#define TGA_PRO_REVA(x)        BALibrary::BAHardwareConfig.m_tgaBoard = TgaBoard::REV_A ///< Macro for specifying REV A of the TGA Pro
#define TGA_PRO_REVB(x)        BALibrary::BAHardwareConfig.m_tgaBoard = TgaBoard::REV_B ///< Macro for specifying REV B of the TGA Pro
#define TGA_PRO_EXPAND_REV2(x) BALibrary::BAHardwareConfig.m_expansionBoard = ExpansionBoard::REV_2 ///< Macro for specifying REV 2 of the Expansion Board
#define SPI_MEM0_1M(x)         BALibrary::BAHardwareConfig.m_spiMem0 = SPI_MEMORY_1M ///< Macro for specifying MEM0 is 1Mbit
#define SPI_MEM0_4M(x)         BALibrary::BAHardwareConfig.m_spiMem0 = SPI_MEMORY_4M ///< Macro for specifying MEM1 is 4Mbit
#define SPI_MEM1_1M(x)         BALibrary::BAHardwareConfig.m_spiMem1 = SPI_MEMORY_1M ///< Macro for specifying MEM0 is 1Mbit
#define SPI_MEM1_4M(x)         BALibrary::BAHardwareConfig.m_spiMem1 = SPI_MEMORY_4M ///< Macro for specifying MEM1 is 1Mbit

/******************************************************************************
 * Hardware Configuration
 *****************************************************************************/
/// enum to specify the TGA Board revision
enum class TgaBoard : unsigned {
    REV_A = 0, ///< indicates using REV A of the TGA Pro
    REV_B      ///< indicates using REV B of the TGA Pro
};

/// enum to specify the TGA Pro Expansion Board revision
enum class ExpansionBoard : unsigned {
    NO_EXPANSION = 0, ///< default, indicates no expansion board is present
    REV_1,            ///< indicates using REV 1 of the Expansion Board
    REV_2             ///< indicates using REV 2 of the Expansion Board
};

/// enum to specify SPI memory dize
enum class SpiMemorySize : unsigned {
    NO_MEMORY = 0, ///< default, indicates no SPI memory installed
    MEM_1M,        ///< indicates 1Mbit memory is installed
    MEM_4M         ///< indicates 4Mbit memory is installed
};

constexpr unsigned NUM_MEM_SLOTS = 2; ///< The TGA Pro has two SPI ports for memory
/// enum to specify MEM0 or MEM1
enum MemSelect : unsigned {
    MEM0 = 0, ///< SPI RAM MEM0
    MEM1 = 1  ///< SPI RAM MEM1
};

/******************************************************************************
 * SPI Memory Definitions
 *****************************************************************************/
/// stores Spi memory size information
struct SpiMemoryDefinition {
    size_t MEM_SIZE_BYTES;
    size_t DIE_BOUNDARY;
};

/// Settings for 4Mbit SPI MEM
constexpr SpiMemoryDefinition SPI_MEMORY_4M = {
    .MEM_SIZE_BYTES = 524288,
    .DIE_BOUNDARY   = 262144
};

/// Settings for 1Mbit SPI MEM
constexpr SpiMemoryDefinition SPI_MEMORY_1M = {
    .MEM_SIZE_BYTES = 131072,
    .DIE_BOUNDARY   = 0
};

/// Settings for No memory
constexpr SpiMemoryDefinition SPI_MEMORY_NONE = {
    .MEM_SIZE_BYTES = 0,
    .DIE_BOUNDARY   = 0
};

/******************************************************************************
 * General Purpose SPI Interfaces
 *****************************************************************************/
/// enum to specify which SPI port is  being used
enum class SpiDeviceId : unsigned {
    SPI_DEVICE0 = 0, ///< Arduino SPI device
    SPI_DEVICE1 = 1  ///< Arduino SPI1 device
};

/**************************************************************************//**
 * BAHardware is a global object that holds hardware configuration options for
 * board revisions and ordering options. It is created automatically, and only
 * one is present. For configuration, the MACROS specified at the top of
 * BAHardware.h should be used.
 *****************************************************************************/
class BAHardware {
public:
    BAHardware() = default; ///< default constructor

    /// sets the TGA Pro board revision
    /// @param tgaBoard enum to specify board revision
    void set(TgaBoard tgaBoard);

    /// get the configured TGA Pro board revision
    /// @returns enum for the board revision
    TgaBoard getTgaBoard(void);

    /// sets the Expansion board revision
    /// @param expansionBoard enum to specify the expansion board revision
    void set(ExpansionBoard expansionBoard);

    /// get the configured Expansion Board revision
    /// @returns enum for the board revision
    ExpansionBoard getExpansionBoard(void);

    /// sets the configured size of a SPI memory
    /// @param memSelect specifies which memory device you are configuring
    /// @param spiMem specifies the memory definition provide (Size, etc.)
    void set(MemSelect memSelect, SpiMemoryDefinition spiMem);

    /// gets the memory definition for a given memory device
    /// @param mem enum to specify memory device to query
    SpiMemoryDefinition getSpiMemoryDefinition(MemSelect mem);

    /// get the size of the given memory in bytes, defaults to MEM0
    /// @param memSelect enum specifies the memory to query
    /// @returns size in bytes
    size_t getSpiMemSizeBytes(MemSelect memSelect = MemSelect::MEM0);

    /// get the size of the given memory in bytes, defaults to MEM0
    /// @param memIndex unsigned specifies the memory to query
    /// @returns size in bytes
    size_t getSpiMemSizeBytes(unsigned memIndex);

    /// get the maximum address in a given memory, defaults to MEM0
    /// @param memSelect enum specifies the memory to query
    /// @returns the last valid address location in the memory
    size_t getSpiMemMaxAddr  (MemSelect memSelect = MemSelect::MEM0);

    /// get the maximum address in a given memory, defaults to MEM0
    /// @param memIndex unsigned specifies the memory to query
    /// @returns the last valid address location in the memory
    size_t getSpiMemMaxAddr  (unsigned memIndex);

    TgaBoard       m_tgaBoard       = TgaBoard::REV_B; ///< stores the configured TGA Pro revision
    ExpansionBoard m_expansionBoard = ExpansionBoard::NO_EXPANSION; ///< stores the configured Expansion Board revision
    SpiMemoryDefinition m_spiMem0   = SPI_MEMORY_NONE; ///< stores the definition for MEM0
    SpiMemoryDefinition m_spiMem1   = SPI_MEMORY_NONE; ///< stores the definition for MEM1
};

extern BAHardware BAHardwareConfig; ///< external definition of global configuration class object

/**************************************************************************//**
 * Teensy 3.6/3.5 Hardware Pinout
 *****************************************************************************/
#if defined(__MK66FX1M0__) || defined(__MK64FX512__) // T3.6 or T3.5
constexpr uint8_t USR_LED_ID = 16; ///< Teensy IO number for the user LED.

// SPI0 and SPI1 pinouts
constexpr uint8_t SPI0_SCK_PIN = 14;
constexpr uint8_t SPI0_CS_PIN = 15;
constexpr uint8_t SPI0_MISO_PIN = 8;
constexpr uint8_t SPI0_MOSI_PIN = 7;

#define SPI1_AVAILABLE
constexpr uint8_t SPI1_SCK_PIN = 20;
constexpr uint8_t SPI1_CS_PIN = 31;
constexpr uint8_t SPI1_MISO_PIN = 5;
constexpr uint8_t SPI1_MOSI_PIN = 21;

// GPIOs and Testpoints are accessed via enumerated class constants.
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
 * Teensy 4.0 Hardware Settings
 *****************************************************************************/
#elif defined(__IMXRT1062__) // T4.0
constexpr uint8_t USR_LED_ID = 2; ///< Teensy IO number for the user LED.

// SPI0 pinouts
constexpr uint8_t SPI0_SCK_PIN = 13;
constexpr uint8_t SPI0_CS_PIN = 10;
constexpr uint8_t SPI0_MISO_PIN = 12;
constexpr uint8_t SPI0_MOSI_PIN = 11;

// GPIOs and Testpoints are accessed via enumerated class constants.
enum class GPIO : uint8_t {
    GPIO0 = 3,
    GPIO1 = 4,
    GPIO2 = 5,
    GPIO3 = 6,

    GPIO4 = 17,
    GPIO5 = 16,
    GPIO6 = 15,
    GPIO7 = 14,

    TP1 = 9,
    TP2 = 22
};

#define SCL_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00
#define SDA_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01
constexpr uint32_t SCL_SDA_PAD_CFG = 0xF808;

#define MCLK_PAD_CTRL  IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_09
#define BCLK_PAD_CTRL  IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10
#define LRCLK_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11
#define DAC_PAD_CTRL   IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01
constexpr uint32_t I2S_PAD_CFG = 0x0008;

/**************************************************************************//**
 * DEFAULT Teensy 3.2 Hardware Settings
 *****************************************************************************/
#else
constexpr uint8_t USR_LED_ID = 16; ///< Teensy IO number for the user LED.

// SPI0 and SPI1 pinouts
constexpr uint8_t SPI0_SCK_PIN = 14;
constexpr uint8_t SPI0_CS_PIN = 15;
constexpr uint8_t SPI0_MISO_PIN = 8;
constexpr uint8_t SPI0_MOSI_PIN = 7;

// GPIOs and Testpoints are accessed via enumerated class constants.
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
#endif

/**************************************************************************//**
 * Blackaddr Audio Expansion Board Pin Configuration
 *****************************************************************************/
#if defined(__MK66FX1M0__) || defined(__MK64FX512__) // T3.6 or T3.5
// Teensy 3.6 Pinout
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

#elif defined(__IMXRT1062__)
// Teensy 4.0 pinout
constexpr unsigned BA_EXPAND_NUM_POT  = 3;
constexpr unsigned BA_EXPAND_NUM_SW   = 2;
constexpr unsigned BA_EXPAND_NUM_LED  = 2;
constexpr unsigned BA_EXPAND_NUM_ENC  = 0;

constexpr uint8_t BA_EXPAND_POT1_PIN = A0; // 14_A0_TX3_SPDIFOUT
constexpr uint8_t BA_EXPAND_POT2_PIN = A1; // 15_A1_RX3_SPDIFIN
constexpr uint8_t BA_EXPAND_POT3_PIN = A2; // 16_A2_RX4_SCL1
constexpr uint8_t BA_EXPAND_SW1_PIN  = 3;  // 3_LRCLK2
constexpr uint8_t BA_EXPAND_SW2_PIN  = 4;  // 4_BCLK2
constexpr uint8_t BA_EXPAND_LED1_PIN = 5;  // 5_IN2
constexpr uint8_t BA_EXPAND_LED2_PIN = 6;  // 6_OUT1D

#else

#warning Your processor is not yet supported in BALibrary

#endif

} // namespace BALibrary

#endif /* __BALIBRARY_BAHARDWARE_H */
