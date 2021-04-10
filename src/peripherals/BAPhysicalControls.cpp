/*
 * BAPhysicalControls.cpp
 *
 *  This file provides a class for handling physical controls such as
 *  switches, pots and rotary encoders.
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
#include "BAPhysicalControls.h"

// These calls must be define in order to get vector to work on arduino
namespace std {
void __throw_bad_alloc() {
  Serial.println("Unable to allocate memory");
  abort();
}
void __throw_length_error( char const*e ) {
  Serial.print("Length Error :"); Serial.println(e);
  abort();
}
}

namespace BALibrary {

BAPhysicalControls::BAPhysicalControls(unsigned numSwitches, unsigned numPots, unsigned numEncoders, unsigned numOutputs) {
	if (numSwitches > 0) {
		m_switches.reserve(numSwitches);
	}
	if (numPots > 0) {
		m_pots.reserve(numPots);
	}
	if (numEncoders > 0) {
		m_encoders.reserve(numEncoders);
	}
	if (numOutputs > 0) {
		m_outputs.reserve(numOutputs);
	}

}

unsigned BAPhysicalControls::addRotary(uint8_t pin1, uint8_t pin2, bool swapDirection, int divider) {
	m_encoders.emplace_back(pin1, pin2, swapDirection, divider);
	pinMode(pin1, INPUT);
	pinMode(pin2, INPUT);
	return  m_encoders.size()-1;
}

unsigned BAPhysicalControls::addSwitch(uint8_t pin, unsigned long intervalMilliseconds) {
	m_switches.emplace_back();
	m_switches.back().attach(pin);
	m_switches.back().interval(10);
	pinMode(pin, INPUT);
	return m_switches.size()-1;
}

unsigned BAPhysicalControls::addPot(uint8_t pin, unsigned minCalibration, unsigned maxCalibration) {
	m_pots.emplace_back(pin, minCalibration, maxCalibration);
	pinMode(pin, INPUT);
	return m_pots.size()-1;
}

unsigned BAPhysicalControls::addPot(uint8_t pin, unsigned minCalibration, unsigned maxCalibration, bool swapDirection) {
	m_pots.emplace_back(pin, minCalibration, maxCalibration, swapDirection);
	pinMode(pin, INPUT);
	return m_pots.size()-1;
}

unsigned BAPhysicalControls::addOutput(uint8_t pin) {
	m_outputs.emplace_back(pin);
	pinMode(pin, OUTPUT);
	return m_outputs.size()-1;
}

void BAPhysicalControls::setOutput(unsigned handle, int val) {
	if (handle >= m_outputs.size()) { return; }
	m_outputs[handle].set(val);
}

void BAPhysicalControls::setOutput(unsigned handle, bool val) {
	if (handle >= m_outputs.size()) { return; }
	unsigned value = val ? 1 : 0;
	m_outputs[handle].set(value);
}

void BAPhysicalControls::toggleOutput(unsigned handle) {
	if (handle >= m_outputs.size()) { return; }
	m_outputs[handle].toggle();
}


int BAPhysicalControls::getRotaryAdjustUnit(unsigned handle) {
	if (handle >= m_encoders.size()) { return 0; } // handle is greater than number of encoders

	int encoderAdjust = m_encoders[handle].getChange();
	if (encoderAdjust != 0) {
		  // clip the adjust to maximum abs value of 1.
		  int encoderAdjust = (encoderAdjust > 0) ? 1 : -1;
	}

	return encoderAdjust;
}

bool BAPhysicalControls::checkPotValue(unsigned handle, float &value) {
	if (handle >= m_pots.size()) { return false;} // handle is greater than number of pots
	return m_pots[handle].getValue(value);
}

int BAPhysicalControls::getPotRawValue(unsigned handle)
{
    if (handle >= m_pots.size()) { return false;} // handle is greater than number of pots
    return m_pots[handle].getRawValue();
}

bool BAPhysicalControls::setCalibrationValues(unsigned handle, unsigned min, unsigned max, bool swapDirection)
{
    if (handle >= m_pots.size()) { return false;} // handle is greater than number of pots
    m_pots[handle].setCalibrationValues(min, max, swapDirection);
    return true;
}

bool BAPhysicalControls::isSwitchToggled(unsigned handle) {
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	DigitalInput &sw = m_switches[handle];

	return sw.hasInputToggled();
}

bool BAPhysicalControls::isSwitchHeld(unsigned handle)
{
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	DigitalInput &sw = m_switches[handle];

	return sw.isInputAssert();
}

bool BAPhysicalControls::getSwitchValue(unsigned handle)
{
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	DigitalInput &sw = m_switches[handle];

	return sw.read();
}

bool BAPhysicalControls::hasSwitchChanged(unsigned handle, bool &switchValue)
{
    if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
    DigitalInput &sw = m_switches[handle];

    return sw.hasInputChanged(switchValue);
}

///////////////////////////
// DigitalInput
///////////////////////////
bool DigitalInput::hasInputToggled() {

    update();
    if (fell() && (m_isPolarityInverted == false)) {
        // switch fell and polarity is not inverted
        return true;
    } else if (rose() && (m_isPolarityInverted == true)) {
        // switch rose and polarity is inveretd
        return true;
    } else {
        return false;
    }
}

bool DigitalInput::isInputAssert()
{
    update();
    // if polarity is inverted, return the opposite state
    bool retValue = Bounce::read() ^ m_isPolarityInverted;
    return retValue;
}

bool DigitalInput::getPinInputValue()
{
    update();
    return Bounce::read();
}

bool DigitalInput::hasInputChanged(bool &switchState)
{
    update();
    if (rose()) {
        // return true if not inverted
        switchState = m_isPolarityInverted ? false : true;
        return true;
    } else if (fell()) {
        // return false if not inverted
        switchState = m_isPolarityInverted ? true : false;
        return true;
    } else {
        // return current value
        switchState = Bounce::read() != m_isPolarityInverted;
        return false;
    }
}

///////////////////////////
// DigitalOutput
///////////////////////////
void DigitalOutput::set(int val) {
	m_val = val;
	digitalWriteFast(m_pin, m_val);
}

void DigitalOutput::toggle(void) {
	m_val = !m_val;
	digitalWriteFast(m_pin, m_val);
}

///////////////////////////
// Potentiometer
///////////////////////////
Potentiometer::Potentiometer(uint8_t analogPin, unsigned minCalibration, unsigned maxCalibration, bool swapDirection)
        : m_pin(analogPin), m_swapDirection(swapDirection), m_minCalibration(minCalibration), m_maxCalibration(maxCalibration)
{
    adjustCalibrationThreshold(m_thresholdFactor); // Calculate the thresholded values
}


void Potentiometer::setFeedbackFitlerValue(float fitlerValue)
{
	m_feedbackFitlerValue = fitlerValue;
}

bool Potentiometer::getValue(float &value) {

    // Check if the minimum sampling time has elapsed
    if (m_timerMs < m_samplingIntervalMs) {
        return false;
    }

    m_timerMs = 0; // reset the sampling timer
    unsigned val = analogRead(m_pin); // read the raw value

    // constrain it within the calibration values, them map it to the desired range.
    val = constrain(val, m_minCalibration, m_maxCalibration);

    // Use an IIR filter to smooth out the noise in the pot readings
    unsigned valFilter = static_cast<int>( (1.0f - m_feedbackFitlerValue)*val + (m_feedbackFitlerValue*m_lastValue));
    m_lastValue = valFilter;

    // Apply a hysteresis check
    if (valFilter == m_lastValidValue) { // check if value hasn't changed
        return false;
    }
    if (abs((int)valFilter - (int)m_lastValidValue) < m_changeThreshold) {
        // The value has not exceeded the change threshold. Suppress the change only if it's not
        // near the limits. This is necessary to ensure the limits can be reached.
        if ( (valFilter < m_maxCalibrationThresholded) && (valFilter > m_minCalibrationThresholded)) {
            return false;
        }
    }

    // Convert the integer reading to a float value range 0.0 to 1.0f
    if (valFilter < m_minCalibrationThresholded) {
        m_lastValue = m_minCalibrationThresholded;
        if (m_lastValidValue == m_minCalibrationThresholded) { return false; }
        m_lastValidValue = m_lastValue;
        value = 0.0f;
    }
    else if (valFilter > m_maxCalibrationThresholded) {
        m_lastValue = m_maxCalibrationThresholded;
        if (m_lastValidValue == m_maxCalibrationThresholded) { return false; }
        m_lastValidValue = m_lastValue;
        value = 1.0f;
    }
    else {
        m_lastValidValue = m_lastValue;
        value = static_cast<float>(valFilter - m_minCalibrationThresholded) / static_cast<float>(m_rangeThresholded);
    }

    if (m_swapDirection) {
        value = 1.0f - value;
    }

    return true;
}

int Potentiometer::getRawValue() {
	return analogRead(m_pin);
}

// Recalculate thresholded limits based on thresholdFactor
void Potentiometer::adjustCalibrationThreshold(float thresholdFactor)
{
    m_thresholdFactor = thresholdFactor;
    // the threshold is specificed as a fraction of the min/max range.
	unsigned threshold = static_cast<unsigned>((m_maxCalibration - m_minCalibration) * thresholdFactor);

	// Update the thresholded values
	m_minCalibrationThresholded = m_minCalibration + threshold;
	m_maxCalibrationThresholded = m_maxCalibration - threshold;
	m_rangeThresholded = m_maxCalibrationThresholded - m_minCalibrationThresholded;
}

void Potentiometer::setCalibrationValues(unsigned min, unsigned max, bool swapDirection)
{
    m_minCalibration = min;
    m_maxCalibration = max;
    m_swapDirection = swapDirection;
    adjustCalibrationThreshold(m_thresholdFactor);
}

void Potentiometer::setSamplingIntervalMs(unsigned intervalMs)
{
    m_samplingIntervalMs = intervalMs;
}

void Potentiometer::setChangeThreshold(float changeThreshold)
{
    m_changeThreshold = changeThreshold;
}

Potentiometer::Calib Potentiometer::calibrate(uint8_t pin) {
	Calib calib;

	// Flush the serial port input buffer
	while (Serial.available() > 0) {}

	Serial.print("Calibration pin "); Serial.println(pin);
	Serial.println("Move the pot fully counter-clockwise to the minimum setting and press any key then ENTER");
	while (true) {
		delay(100);
		if (Serial.available() > 0) {
			calib.min = analogRead(pin);
			while (Serial.available()) { Serial.read(); }
			break;
		}
	}

	Serial.println("Move the pot fully clockwise to the maximum setting and press any key then ENTER");
	while (true) {
		delay(100);
		if (Serial.available() > 0) {
			calib.max = analogRead(pin);
			while (Serial.available()) { Serial.read(); }
			break;
		}
	}

	if (calib.min > calib.max) {
		unsigned tmp = calib.max;
		calib.max = calib.min;
		calib.min = tmp;
		calib.swap = true;
	}

	Serial.print("The calibration for pin "); Serial.print(pin);
	Serial.print(" is min:"); Serial.print(calib.min);
	Serial.print("  max:"); Serial.print(calib.max);
	Serial.print("  swap: "); Serial.println(calib.swap);

	return calib;
}


int RotaryEncoder::getChange() {
  int32_t newPosition = read();
  int delta = newPosition - m_lastPosition;
  m_lastPosition = newPosition;
  if (m_swapDirection) { delta = -delta; }
  return delta/m_divider;
}

void RotaryEncoder::setDivider(int divider) {
  m_divider = divider;
}

} // BALibrary




