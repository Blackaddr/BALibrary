/*
 * BAGpio.cpp
 *
 *  Created on: November 1, 2017
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

#include "Arduino.h"
#include "BAGpio.h"

namespace BALibrary {

BAGpio::BAGpio()
{
	// Set all GPIOs to input
	pinMode(static_cast<uint8_t>(GPIO::GPIO0), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO1), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO2), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO3), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO4), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO5), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO6), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::GPIO7), INPUT);
	pinMode(static_cast<uint8_t>(GPIO::TP1),   INPUT);
	pinMode(static_cast<uint8_t>(GPIO::TP2),   INPUT);

	// Set the LED ot ouput
	pinMode(USR_LED_ID, OUTPUT);
	clearLed(); // turn off the LED

}

BAGpio::~BAGpio()
{
}

void BAGpio::setGPIODirection(GPIO gpioId, int direction)
{
	pinMode(static_cast<uint8_t>(gpioId), direction);
}
void BAGpio::setGPIO(GPIO gpioId)
{
	digitalWrite(static_cast<uint8_t>(gpioId), 0x1);
}
void BAGpio::clearGPIO(GPIO gpioId)
{
	digitalWrite(static_cast<uint8_t>(gpioId), 0);

}
int BAGpio::toggleGPIO(GPIO gpioId)
{
	int data = digitalRead(static_cast<uint8_t>(gpioId));
	digitalWrite(static_cast<uint8_t>(gpioId), ~data);
	return ~data;
}

void BAGpio::setLed()
{
	digitalWrite(USR_LED_ID, 0x1);
	m_ledState = 1;
}
void BAGpio::clearLed()
{
	digitalWrite(USR_LED_ID, 0);
	m_ledState = 0;
}
int BAGpio::toggleLed()
{
	m_ledState = ~m_ledState;
	digitalWrite(USR_LED_ID, m_ledState);
	return m_ledState;
}


} /* namespace BALibrary */
