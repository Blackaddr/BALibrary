/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BAPhysicalControls is a general purpose class for handling an array of
 *  pots, switches, rotary encoders and outputs (for LEDs or relays).
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
#include <Bounce2.h>

#include "Arduino.h"

namespace BALibrary {

constexpr bool SWAP_DIRECTION = true;    ///< Use when specifying direction should be swapped
constexpr bool NOSWAP_DIRECTION = false; ///< Use when specifying direction should not be swapped

/// Convenience class for handling a digital output with regard to setting and toggling stage
class DigitalOutput {
public:
	DigitalOutput() = delete; // delete the default constructor

	/// Construct an object to control the specified pin
	/// @param pin the "Logical Arduino" pin number (not the physical pin number
	DigitalOutput(uint8_t pin) : m_pin(pin) {}

	/// Set the output value
	/// @param val when zero output is low, otherwise output his high
	void set(int val);

	/// Toggle the output value current state
	void toggle(void);

private:
	uint8_t m_pin; ///< store the pin associated with this output
	int m_val = 0; ///< store the value to support toggling
};

/// Convenience class for handling digital inputs. This wraps Bounce with the abilty
/// to invert the polarity of the pin.
class DigitalInput : public Bounce {
public:
	/// Create an input where digital low is return true. Most switches ground when pressed.
	DigitalInput() : m_isPolarityInverted(true) {}

	/// Create an input and specify if input polarity is inverted.
	/// @param isPolarityInverted when false, a high voltage on the pin returns true, 0V returns false.
	DigitalInput(bool isPolarityInverted) : m_isPolarityInverted(isPolarityInverted) {}

	/// Read the state of the pin according to the polarity
	/// @returns true when the input should be interpreted as the switch is closed, else false.
	bool read() { return Bounce::read() != m_isPolarityInverted; } // logical XOR to conditionally invert polarity

	/// Set whether the input pin polarity
	/// @param polarity when true, a low voltage on the pin is considered true by read(), else false.
	void setPolarityInverted(bool polarity) { m_isPolarityInverted = polarity; }

	/// Check if input has toggled from low to high to low by looking for falling edges
	/// @returns true if the switch has toggled
	bool hasInputToggled();

	/// Check if the input is asserted
	/// @returns true if the switch is held
	bool isInputAssert();

	/// Get the raw input value (ignores polarity inversion)
	/// @returns returns true is physical pin is high, else false
	bool getPinInputValue();

	/// Store the current switch state and return true if it has changed.
	/// @param switchState variable to store switch state in
	/// @returns true when switch stage has changed since last check
	bool hasInputChanged(bool &switchState);

private:
	bool m_isPolarityInverted;
	bool m_isPushed;
};

/// Convenience class for handling an analog pot as a control. When calibrated,
/// returns a float between 0.0 and 1.0.
class Potentiometer {
public:
	/// Calibration data for a pot includes it's min and max value, as well as whether
	/// direction should be swapped. Swapping depends on the physical orientation of the
	/// pot.
	struct Calib {
		unsigned min; ///< The value from analogRead() when the pot is fully counter-clockwise (normal orientation)
		unsigned max; ///< The value from analogRead() when the pot is fully clockwise (normal orientation)
		bool swap;    ///< when orientation is such that fully clockwise would give max reading, swap changes it to the min
	};

	Potentiometer() = delete; // delete the default constructor

	/// Construction requires the Arduino analog pin number, as well as calibration values.
	/// @param analogPin The analog Arduino pin literal. This the number on the Teensy pinout preceeeded by an A in the local pin name. E.g. "A17".
	/// @param minCalibration See Potentiometer::calibrate()
	/// @param maxCalibration See Potentiometer::calibrate()
	/// @param swapDirection Optional param. See Potentiometer::calibrate()
	Potentiometer(uint8_t analogPin, unsigned minCalibration, unsigned maxCalibration, bool swapDirection = false);

	/// Get new value from the pot.
	/// @param value reference to a float, the new value will be written here. Value is between 0.0 and 1.0f.
	/// @returns true if the value has changed, false if it has not
	bool getValue(float &value);

	/// Get the raw int value directly from analogRead()
	/// @returns an integer between 0 and 1023.
	int getRawValue();

	/// Adjust the calibrate threshold. This is a factor that shrinks the calibration range slightly to
	/// ensure the full range from 0.0f to 1.0f can be obtained.
	/// @details temperature change can slightly alter the calibration values. This factor causes the value
	/// to hit min or max just before the end of the pots travel to compensate.
	/// @param thresholdFactor typical value is 0.01f to 0.05f
	void adjustCalibrationThreshold(float thresholdFactor);

	/// Set the amount of feedback in the IIR filter used to smooth the pot readings
	/// @details actual filter response depends on the rate you call getValue()
	/// @param filterValue typical values are 0.80f to 0.95f
	void setFeedbackFitlerValue(float fitlerValue);

	/// Set the calibration values for the pots
	/// @param min analog pot reading for min value
	/// @param max analog pot reading for max value
	/// @param swapDirection when true min and max are reversed. (Depends on physical pot orientation)
	void setCalibrationValues(unsigned min, unsigned max, bool swapDirection);

	/// Sets a MINIMUM sampling interval for the pot in milliseconds.
	/// @details When making a call to getValue(), if the time since the last reading is
	/// less than this interval, a new reading will not be taken.
	/// @param intervalMs the desired minimum sampling interval in milliseconds
	void setSamplingIntervalMs(unsigned intervalMs);

	/// Sets the minimum change between previous reading and new reading to be considered valid.
	/// @details Increasing this value will help remove noise. Default is 8.
	/// @param changeThreshold new change threshold for ADC
	void setChangeThreshold(float changeThreshold);

	/// Call this static function before creating the object to obtain calibration data. The sequence
	/// involves prompts over the Serial port.
	/// @details E.g. call Potentiometer::calibrate(PIN). See BAExpansionCalibrate.ino in the library examples.
	/// @param analogPin the Arduino analog pin number connected to the pot you wish to calibraate.
	/// @returns populated Potentiometer::Calib structure
	static Calib calibrate(uint8_t analogPin);

private:
    uint8_t m_pin;                      ///< store the Arduino pin literal, e.g. A17
	bool m_swapDirection;               ///< swap when pot orientation is upside down
	unsigned m_minCalibration;          ///< stores the min pot value
	unsigned m_maxCalibration;          ///< stores the max pot value
	unsigned m_lastValue = 0;           ///< stores previous value
	float m_feedbackFitlerValue = 0.8f; ///< feedback value for POT filter
	float m_thresholdFactor = 0.05f;    ///< threshold factor causes values pot to saturate faster at the limits, default is 5%
    unsigned m_minCalibrationThresholded; ///< stores the min pot value after thresholding
    unsigned m_maxCalibrationThresholded; ///< stores the max pot value after thresholding
    unsigned m_rangeThresholded;          ///< stores the range of max - min after thresholding

    unsigned m_changeThreshold = 8;     ///< a new reading must change by this amount to be valid
    unsigned m_lastValidValue = 0;           ///< stores previous value
    unsigned m_samplingIntervalMs = 20; ///< the sampling interval in milliseconds
    elapsedMillis m_timerMs;            ///< special Teensy variable that tracks time
};

/// Convenience class for rotary (quadrature) encoders. Uses Arduino Encoder under the hood.
class RotaryEncoder : public Encoder {
public:
  RotaryEncoder() = delete; // delete default constructor

  /// Constructor an encoder with the specified pins. Optionally swap direction and divide down the number of encoder ticks
  /// @param pin1 the Arduino logical pin number for the 'A' on the encoder.
  /// @param pin2 the Arduino logical pin number for the 'B' on the encoder.
  /// @param swapDirection (OPTIONAL) set to true or false to obtain clockwise increments the counter-clockwise decrements
  /// @param divider (OPTIONAL) controls the sensitivity of the divider. Use powers of 2. E.g. 1, 2, 4, 8, etc.
  RotaryEncoder(uint8_t pin1, uint8_t pin2, bool swapDirection = false, int divider = 1) : Encoder(pin1,pin2), m_swapDirection(swapDirection), m_divider(divider) {}

  /// Get the delta (as a positive or negative number) since last call
  /// @returns an integer representing the net change since last call
  int getChange();

  /// Set the divider on the internal counter. High resolution encoders without detents can be overly sensitive.
  /// This will helf reduce sensisitive by increasing the divider. Default = 1.
  /// @pram divider controls the sensitivity of the divider. Use powers of 2. E.g. 1, 2, 4, 8, etc.
  void setDivider(int divider);

private:
  bool m_swapDirection;       ///< specifies if increment/decrement should be swapped
  int32_t m_lastPosition = 0; ///< store the last recorded position
  int32_t m_divider;          ///< divides down the magnitude of change read by the encoder.
};

/// Specifies the type of control
enum class ControlType : unsigned {
  SWITCH_MOMENTARY = 0,  ///< a momentary switch, which is only on when pressed.
  SWITCH_LATCHING  = 1,  ///< a latching switch, which toggles between on and off with each press and release
  ROTARY_KNOB      = 2,  ///< a rotary encoder knob
  POT              = 3,  ///< an analog potentiometer
  UNDEFINED        = 255 ///< undefined or uninitialized
};

/// Tjis class provides convenient interface for combinary an arbitrary number of controls of different types into
/// one object. Supports switches, pots, encoders and digital outputs (useful for LEDs, relays).
class BAPhysicalControls {
public:
	BAPhysicalControls() = delete;

	/// Construct an object and reserve memory for the specified number of controls. Encoders and outptus are optional params.
	/// @param numSwitches the number of switches or buttons
	/// @param numPots the number of analog potentiometers
	/// @param numEncoders the number of quadrature encoders
	/// @param numOutputs the number of digital outputs. E.g. LEDs, relays, etc.
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

	/// add an output to the controls
	/// @param pin the pin number connected to the Arduino output
	/// @returns a handle (unsigned) to the added output. Use this to access the output.
	unsigned addOutput(uint8_t pin);

	/// Set the output specified by the provided handle
	/// @param handle the handle that was provided previously by calling addOutput()
	/// @param val the value to set the output. 0 is low, not zero is high.
	void setOutput(unsigned handle, int val);

	/// Set the output specified by the provided handle
	/// @param handle the handle that was provided previously by calling addOutput()
	/// @param val the value to set the output. True is high, false is low.
	void setOutput(unsigned handle, bool val);

	/// Toggle the output specified by the provided handle
	/// @param handle the handle that was provided previously by calling addOutput()
	void toggleOutput(unsigned handle);

	/// Retrieve the change in position on the specified rotary encoder
	/// @param handle the handle that was provided previously by calling addRotary()
	/// @returns an integer value. Positive is clockwise, negative is counter-clockwise rotation.
	int getRotaryAdjustUnit(unsigned handle);

	/// Check if the pot specified by the handle has been updated.
	/// @param handle the handle that was provided previously by calling addPot()
	/// @param value a reference to a float, the pot value will be written to this variable.
	/// @returns true if the pot value has changed since previous check, otherwise false
	bool checkPotValue(unsigned handle, float &value);

	/// Get the raw uncalibrated value from the pot
	/// @returns uncalibrated pot value
	int getPotRawValue(unsigned handle);

	/// Override the calibration values with new values
	/// @param handle handle the handle that was provided previously by calling addPot()
	/// @param min the min raw value for the pot
	/// @param max the max raw value for the pot
	/// @param swapDirection when true, max raw value will mean min control value
	/// @returns false when handle is out of range
	bool setCalibrationValues(unsigned handle, unsigned min, unsigned max, bool swapDirection);

	/// Check if the switch has been toggled since last call
	/// @param handle the handle that was provided previously by calling addSwitch()
	/// @returns true if the switch changed state, otherwise false
    bool isSwitchToggled(unsigned handle);

    /// Check if the switch is currently being pressed (held)
	/// @param handle the handle that was provided previously by calling addSwitch()
	/// @returns true if the switch is held in a pressed or closed state
    bool isSwitchHeld(unsigned handle);

    /// Get the value of the switch
	/// @param handle the handle that was provided previously by calling addSwitch()
    /// @returns the value at the switch pin, either 0 or 1.
    bool getSwitchValue(unsigned handle);

    /// Determine if a switch has changed value
    /// @param handle the handle that was provided previously by calling addSwitch()
    /// @param switchValue a boolean to store the new switch value
    /// @returns true if the switch has changed
    bool hasSwitchChanged(unsigned handle, bool &switchValue);

private:
  std::vector<Potentiometer> m_pots;     ///< a vector of all added pots
  std::vector<RotaryEncoder> m_encoders; ///< a vector of all added encoders
  std::vector<DigitalInput>  m_switches;       ///< a vector of all added switches
  std::vector<DigitalOutput> m_outputs;  ///< a vector of all added outputs
};

} // BALibrary

#endif /* __BAPHYSICALCONTROLS_H */
