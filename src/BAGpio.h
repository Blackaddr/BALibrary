/*
 * BAGpio.h
 *
 *  Created on: May 22, 2017
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

#ifndef SRC_BAGPIO_H_
#define SRC_BAGPIO_H_

namespace BAGuitar {

//constexpr int NUM_GPIO = 10;

constexpr uint8_t USR_LED_ID = 16;

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
//// J3 - GPIO Header
//constexpr int GPIO0 = 2;
//constexpr int GPIO1 = 3;
//constexpr int GPIO2 = 4;
//constexpr int GPIO3 = 6;
//
//// J6 - GPIO Header
//constexpr int GPIO4 = 12;
//constexpr int GPIO5 = 32;
//constexpr int GPIO6 = 27;
//constexpr int GPIO7 = 28;


// Test points
//constexpr int TP1 = 34;
//constexpr int TP2 = 33;

class BAGpio {
public:
	BAGpio();
	virtual ~BAGpio();

	void setGPIODirection(GPIO gpioId, int direction);
	void setGPIO(GPIO gpioId);
	void clearGPIO(GPIO gpioId);
	void toggleGPIO(GPIO gpioId);

	void setLed();
	void clearLed();
	void toggleLed();

private:
	//bool m_gpioState[NUM_GPIO];
	uint8_t m_ledState;
};

} /* namespace BAGuitar */

#endif /* SRC_BAGPIO_H_ */
