/*
 * BAGpio.cpp
 *
 *  Created on: Sep 17, 2017
 *      Author: slascos
 */

#include "Arduino.h"
#include "BAGpio.h"

namespace BAGuitar {

BAGpio::BAGpio()
{
	// Set all GPIOs to input
//	for (auto i=0; i<NUM_GPIO; i++) {
//		m_gpioState = 1;
//	}

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

	pinMode(USR_LED_ID, OUTPUT);
	m_ledState = 0;


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
void BAGpio::toggleGPIO(GPIO gpioId)
{
	int data = digitalRead(static_cast<uint8_t>(gpioId));
	digitalWrite(static_cast<uint8_t>(gpioId), ~data);
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
void BAGpio::toggleLed()
{
	m_ledState = ~m_ledState;
	digitalWrite(USR_LED_ID, m_ledState);
}


} /* namespace BAGuitar */
