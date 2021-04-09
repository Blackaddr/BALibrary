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
	pinMode(GPIO0, INPUT);
	pinMode(GPIO1, INPUT);
	pinMode(GPIO2, INPUT);
	pinMode(GPIO3, INPUT);
	pinMode(GPIO4, INPUT);
	pinMode(GPIO5, INPUT);
	pinMode(GPIO6, INPUT);
	pinMode(GPIO7, INPUT);
	pinMode(TP1, INPUT);
	pinMode(TP2, INPUT);

	// Set the LED to ouput
	pinMode(USR_LED_ID, OUTPUT);
	clearLed(); // turn off the LED

}

BAGpio::~BAGpio()
{
}

void BAGpio::setGPIODirection(GPIO gpioId, int direction)
{
    pinMode(enumToPinNumber(gpioId), direction);
}
void BAGpio::setGPIO(GPIO gpioId)
{
	digitalWrite(enumToPinNumber(gpioId), 0x1);
}
void BAGpio::clearGPIO(GPIO gpioId)
{
	digitalWrite(enumToPinNumber(gpioId), 0);

}
int BAGpio::toggleGPIO(GPIO gpioId)
{
	int data = digitalRead(enumToPinNumber(gpioId));
	digitalWrite(enumToPinNumber(gpioId), ~data);
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

uint8_t enumToPinNumber(GPIO gpio)
{
    uint8_t pinNumber;
    switch(gpio) {
    case GPIO::GPIO0 : pinNumber = GPIO0; break;
    case GPIO::GPIO1 : pinNumber = GPIO1; break;
    case GPIO::GPIO2 : pinNumber = GPIO2; break;
    case GPIO::GPIO3 : pinNumber = GPIO3; break;
    case GPIO::GPIO4 : pinNumber = GPIO4; break;
    case GPIO::GPIO5 : pinNumber = GPIO5; break;
    case GPIO::GPIO6 : pinNumber = GPIO6; break;
    case GPIO::GPIO7 : pinNumber = GPIO7; break;
    case GPIO::TP1   : pinNumber = TP1;   break;
    case GPIO::TP2   : pinNumber = TP2;   break;
    default          : pinNumber = 0;     break;
    }
    return pinNumber;
}


} /* namespace BALibrary */
