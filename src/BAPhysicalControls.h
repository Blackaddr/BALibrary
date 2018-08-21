/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BAPhysicalControls is a general purpose class for handling an array of
 *  pots and switches.
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
#ifndef __BAPHYSICALCONTROLS_H
#define __BAPHYSICALCONTROLS_H

#include <vector>
#include <Encoder.h>
#include <Bounce.h>

namespace BALibrary {

class DigitalOutput {
public:
	DigitalOutput(uint8_t pin) : m_pin(pin) {}
	void set(int val);
	void toggle(void);

private:
	uint8_t m_pin;
	int m_val = 0;
};

class Potentiometer {
public:
	struct Calib {
		unsigned min;
		unsigned max;
		bool swap;
	};
	Potentiometer(uint8_t pin, unsigned minCalibration, unsigned maxCalibration)
        : m_pin(pin), m_swapDirection(false), m_minCalibration(minCalibration), m_maxCalibration(maxCalibration) {}

	Potentiometer(uint8_t pin, unsigned minCalibration, unsigned maxCalibration, bool swapDirection)
        : m_pin(pin), m_swapDirection(swapDirection), m_minCalibration(minCalibration), m_maxCalibration(maxCalibration) {}

	bool getValue(float &value);
	int getRawValue();
	void adjustCalibrationThreshold(float thresholdFactor);
	static Calib calibrate(uint8_t pin);

private:
    uint8_t m_pin;
	bool m_swapDirection;
	unsigned m_minCalibration;
	unsigned m_maxCalibration;
	unsigned m_lastValue = 0;
	unsigned m_lastValue2 = 0;
};

constexpr bool ENCODER_SWAP = true;
constexpr bool ENCODER_NOSWAP = false;

class RotaryEncoder : public Encoder {
public:
  RotaryEncoder(uint8_t pin1, uint8_t pin2, bool swapDirection = false, int divider = 1) : Encoder(pin1,pin2), m_swapDirection(swapDirection), m_divider(divider) {}

  int getChange();
  void setDivider(int divider);

private:
  bool m_swapDirection;
  int32_t m_lastPosition = 0;
  int32_t m_divider;
};

/// Specifies the type of control
enum class ControlType : unsigned {
  SWITCH_MOMENTARY = 0,  ///< a momentary switch, which is only on when pressed.
  SWITCH_LATCHING  = 1,  ///< a latching switch, which toggles between on and off with each press and release
  ROTARY_KNOB      = 2,  ///< a rotary encoder knob
  POT              = 3,  ///< an analog potentiometer
  UNDEFINED        = 255 ///< undefined or uninitialized
};

class BAPhysicalControls {
public:
	BAPhysicalControls() = delete;
	BAPhysicalControls(unsigned numSwitches, unsigned numPots, unsigned numEncoders = 0, unsigned numOutputs = 0);
	~BAPhysicalControls() {}

	/// add a rotary encoders to the controls
	/// @param pin1 the pin number corresponding to 'A' on the encoder
	/// @param pin2 the pin number corresponding to 'B' on the encoder
	/// @param swapDirection When true, reverses which rotation direction is positive, and which is negative
	/// @param divider optional, for encoders with high resolution this divides down the rotation measurement.
	/// @returns the index in the encoder vector the new encoder was placed at.
	unsigned addRotary(uint8_t pin1, uint8_t pin2, bool swapDirection = false, int divider = 1);

	/// add a switch to the controls
	/// @param pin the pin number connected to the switch
	/// @param intervalMilliseconds, optional, specifies the filtering time to debounce a switch
	/// @returns the index in the switch vector the new switch was placed at.
	unsigned addSwitch(uint8_t pin, unsigned long intervalMilliseconds = 10);

	/// add a pot to the controls
	/// @param pin the pin number connected to the wiper of the pot
	/// @param minCalibration the value corresponding to lowest pot setting
	/// @param maxCalibration the value corresponding to the highest pot setting
	unsigned addPot(uint8_t pin, unsigned minCalibration, unsigned maxCalibration);

	/// add a pot to the controls
	/// @param pin the pin number connected to the wiper of the pot
	/// @param minCalibration the value corresponding to lowest pot setting
	/// @param maxCalibration the value corresponding to the highest pot setting
	/// @param swapDirection reverses the which direction is considered pot minimum value
	/// @param range the pot raw value will be mapped into a range of 0 to range
	unsigned addPot(uint8_t pin, unsigned minCalibration, unsigned maxCalibration, bool swapDirection);
	unsigned addOutput(uint8_t pin);
	void setOutput(unsigned index, int val);
	void toggleOutput(unsigned index);
	int getRotaryAdjustUnit(unsigned index);
	bool checkPotValue(unsigned index, float &value);
    bool isSwitchToggled(unsigned index);

private:
  std::vector<Potentiometer> m_pots;
  std::vector<RotaryEncoder> m_encoders;
  std::vector<Bounce>  m_switches;
  std::vector<DigitalOutput> m_outputs;
};





} // BALibrary


#endif /* __BAPHYSICALCONTROLS_H */
