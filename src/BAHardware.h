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

#ifndef BALIBRARY_BAHARDWARE_H_
#define BALIBRARY_BAHARDWARE_H_

#include <Arduino.h>
#include <cstdint>
#include <cstddef>

/**************************************************************************//**
 * BALibrary is a namespace/Library for Guitar processing from Blackaddr Audio.
 *****************************************************************************/
namespace BALibrary {

/******************************************************************************
 * Hardware Configuration
 *****************************************************************************/
/// enum to specify the TGA Board revision
enum class TgaBoard : unsigned {
    REV_A = 0, ///< indicates using REV A of the TGA Pro
    REV_B,     ///< indicates using REV B of the TGA Pro
    MKII_REV1, ///< indicates using MKII, Rev 1 of the TGA Pro
    AVALON
};

/// enum to specify the TGA Board revision
enum class TeensyProcessor : unsigned {
    TEENSY3 = 0, ///< indicates using REV A of the TGA Pro
    TEENSY4,     ///< indicates using REV B of the TGA Pro
};

/// enum to specify the TGA Pro Expansion Board revision
enum class ExpansionBoard : unsigned {
    NO_EXPANSION = 0, ///< default, indicates no expansion board is present
    REV_1,            ///< indicates using REV 1 of the Expansion Board
    REV_2,            ///< indicates using REV 2 of the Expansion Board
    REV_3             ///< indicates using REV 3 of the Expansion Board (MKII Series)
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

// GPIOs and Testpoints are accessed via enumerated class constants.
enum class GPIO {
    GPIO0 = 0,
    GPIO1 = 1,
    GPIO2 = 2,
    GPIO3 = 3,

    GPIO4 = 4,
    GPIO5 = 5,
    GPIO6 = 6,
    GPIO7 = 7,

    TP1 = 8,
    TP2 = 9
};

/**************************************************************************//**
 * BAHardware is a global object that holds hardware configuration options for
 * board revisions and ordering options. It is created automatically, and only
 * one is present. For configuration, the MACROS specified at the top of
 * BAHardware.h should be used.
 *****************************************************************************/
class BAHardware {
public:
    BAHardware(); ///< default constructor

    /// sets the TGA Pro board revision
    /// @param tgaBoard enum to specify board revision
    void set(TgaBoard tgaBoard);

    /// get the configured TGA Pro board revision
    /// @returns enum for the board revision
    TgaBoard getTgaBoard(void);

    /// get the configured Teensy Processor
    /// @returns enum for the processor
    TeensyProcessor getTeensyProcessor();

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

    TgaBoard            m_tgaBoard        = TgaBoard::MKII_REV1;              ///< stores the configured TGA Pro revision
    TeensyProcessor     m_teensyProcessor = TeensyProcessor::TEENSY4;   ///< store the processor in use
    ExpansionBoard      m_expansionBoard  = ExpansionBoard::NO_EXPANSION; ///< stores the configured Expansion Board revision
    SpiMemoryDefinition m_spiMem0         = SPI_MEMORY_NONE;              ///< stores the definition for MEM0
    SpiMemoryDefinition m_spiMem1         = SPI_MEMORY_NONE;              ///< stores the definition for MEM1
};

extern BAHardware BAHardwareConfig; ///< external definition of global configuration class object

// In your Arudino .ino file, use #defines for your TGA Pro revision and options
// to correctly configure your hardware
#define TGA_PRO_REVA(x)        BALibrary::BAHardwareConfig.set(TgaBoard::REV_A)     ///< Macro for specifying REV A of the TGA Pro
#define TGA_PRO_REVB(x)        BALibrary::BAHardwareConfig.set(TgaBoard::REV_B)     ///< Macro for specifying REV B of the TGA Pro
#define TGA_PRO_MKII_REV1(x)   BALibrary::BAHardwareConfig.set(TgaBoard::MKII_REV1) ///< Macro for specifying REV B of the TGA Pro

#define TGA_PRO_EXPAND_REV2(x) BALibrary::BAHardwareConfig.setExpansionBoard(ExpansionBoard::REV_2) ///< Macro for specifying REV 2 of the Expansion Board
#define TGA_PRO_EXPAND_REV3(x) BALibrary::BAHardwareConfig.setExpansionBoard(ExpansionBoard::REV_3) ///< Macro for specifying REV 2 of the Expansion Board

#define SPI_MEM0_1M(x)         BALibrary::BAHardwareConfig.set(MEM0, SPI_MEMORY_1M) ///< Macro for specifying MEM0 is 1Mbit
#define SPI_MEM0_4M(x)         BALibrary::BAHardwareConfig.set(MEM0, SPI_MEMORY_4M) ///< Macro for specifying MEM1 is 4Mbit
#define SPI_MEM1_1M(x)         BALibrary::BAHardwareConfig.set(MEM1, SPI_MEMORY_1M) ///< Macro for specifying MEM0 is 1Mbit
#define SPI_MEM1_4M(x)         BALibrary::BAHardwareConfig.set(MEM1, SPI_MEMORY_4M) ///< Macro for specifying MEM1 is 1Mbit

extern uint8_t USR_LED_ID; ///< Teensy IO number for the user LED.

extern unsigned BA_EXPAND_NUM_POT;
extern unsigned BA_EXPAND_NUM_SW;
extern unsigned BA_EXPAND_NUM_LED;
extern unsigned BA_EXPAND_NUM_ENC;

extern uint8_t BA_EXPAND_POT1_PIN; // 14_A0_TX3_SPDIFOUT
extern uint8_t BA_EXPAND_POT2_PIN; // 15_A1_RX3_SPDIFIN
extern uint8_t BA_EXPAND_POT3_PIN; // 16_A2_RX4_SCL1

extern uint8_t BA_EXPAND_SW1_PIN;  // 2_OUT2
extern uint8_t BA_EXPAND_SW2_PIN;  // 3_LRCLK2
extern uint8_t BA_EXPAND_LED1_PIN;  // 4_BLCK2
extern uint8_t BA_EXPAND_LED2_PIN;  // 5_IN2

extern uint8_t GPIO0;
extern uint8_t GPIO1;
extern uint8_t GPIO2;
extern uint8_t GPIO3;
extern uint8_t GPIO4;
extern uint8_t GPIO5;
extern uint8_t GPIO6;
extern uint8_t GPIO7;
extern uint8_t TP1;
extern uint8_t TP2;

// SPI0 and SPI1 pinouts
extern uint8_t SPI0_SCK_PIN;
extern uint8_t SPI0_CS_PIN;
extern uint8_t SPI0_MISO_PIN;
extern uint8_t SPI0_MOSI_PIN;

extern uint8_t SPI1_SCK_PIN;
extern uint8_t SPI1_CS_PIN;
extern uint8_t SPI1_MISO_PIN;
extern uint8_t SPI1_MOSI_PIN;

#if defined(ARDUINO_TEENSY41) || defined(__MK66FX1M0__) || defined(__MK64FX512__)
#define SPI1_AVAILABLE
#endif

/**************************************************************************//**
 * Teensy 4.0 Hardware Settings
 *****************************************************************************/
#if defined(__IMXRT1062__) // T4.0

#define SCL_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00
#define SDA_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01
constexpr uint32_t SCL_SDA_PAD_CFG = 0xF808;

#define MCLK_PAD_CTRL  IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_09
#define BCLK_PAD_CTRL  IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10
#define LRCLK_PAD_CTRL IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11
#define DAC_PAD_CTRL   IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01
constexpr uint32_t I2S_PAD_CFG = 0x0008;
#endif



} // namespace BALibrary

#endif /* BALIBRARY_BAHARDWARE_H_ */
