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
	m_switches.emplace_back(pin, intervalMilliseconds);
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
	unsigned value = val ? true : false;
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

bool BAPhysicalControls::isSwitchToggled(unsigned handle) {
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	Bounce &sw = m_switches[handle];

	if (sw.update() && sw.fallingEdge()) {
	  return true;
	} else {
	  return false;
	}
}

bool BAPhysicalControls::isSwitchHeld(unsigned handle)
{
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	Bounce &sw = m_switches[handle];

	if (sw.read()) {
		return true;
	} else {
		return false;
	}
}

int BAPhysicalControls::getSwitchValue(unsigned handle)
{
	if (handle >= m_switches.size()) { return 0; } // handle is greater than number of switches
	Bounce &sw = m_switches[handle];

	return sw.read();
}

///////////////////////////

void DigitalOutput::set(int val) {
	m_val = val;
	digitalWriteFast(m_pin, m_val);
}

void DigitalOutput::toggle(void) {
	m_val = !m_val;
	digitalWriteFast(m_pin, m_val);
}

void Potentiometer::setFeedbackFitlerValue(float fitlerValue)
{
	m_feedbackFitlerValue = fitlerValue;
}

bool Potentiometer::getValue(float &value) {

	bool newValue = true;

	unsigned val = analogRead(m_pin); // read the raw value
	// Use an IIR filter to smooth out the noise in the pot readings
	unsigned valFilter = ( (1.0f - m_feedbackFitlerValue)*val + m_feedbackFitlerValue*m_lastValue);

	// constrain it within the calibration values, them map it to the desired range.
	valFilter = constrain(valFilter, m_minCalibration, m_maxCalibration);
	if (valFilter == m_lastValue) {
		newValue = false;
	}
	m_lastValue = valFilter;

	value = static_cast<float>(valFilter - m_minCalibration) / static_cast<float>(m_maxCalibration);
	if (m_swapDirection) {
		value = 1.0f - value;
	}
    return newValue;
}

int Potentiometer::getRawValue() {
	return analogRead(m_pin);
}

void Potentiometer::adjustCalibrationThreshold(float thresholdFactor)
{
	float threshold = m_maxCalibration * thresholdFactor;
	m_maxCalibration -= static_cast<unsigned>(threshold);
	m_minCalibration += static_cast<unsigned>(threshold);
}

Potentiometer::Calib Potentiometer::calibrate(uint8_t pin) {
	Calib calib;

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




