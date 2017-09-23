/**************************************************************************//**
 *  BAGPio is convenience class for accessing the the various GPIOs available
 *  on the Teensy Guitar Audio series boards.
 *
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
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

#ifndef SRC_BAGPIO_H_
#define SRC_BAGPIO_H_

namespace BAGuitar {

constexpr uint8_t USR_LED_ID = 16; ///< Teensy IO number for the user LED.

/**************************************************************************//**
 * GPIOs and Testpoints are accessed via enumerated class constants.
 *****************************************************************************/
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
 * BAGpio provides a convince class to easily control the direction and state
 * of the GPIO pins available on the TGA headers.
 * @details you can always control this directly with Arduino commands like
 * digitalWrite(), etc.
 *****************************************************************************/
class BAGpio {
public:
	BAGpio();
	virtual ~BAGpio();

	/// Set the direction of the specified GPIO pin.
	/// @param gpioId Specify a GPIO pin such as GPIO::GPIO0
	/// @param specify direction as INPUT or OUTPUT which are Arduino constants
	void setGPIODirection(GPIO gpioId, int direction);

	/// Set the state of the specified GPIO to high
	/// @param gpioId gpioId Specify a GPIO pin such as GPIO::GPIO0
	void setGPIO(GPIO gpioId);

	/// Clear the state of the specified GPIO pin.
	/// @param gpioId gpioId Specify a GPIO pin such as GPIO::GPIO0
	void clearGPIO(GPIO gpioId);

	/// Toggle the state of the specified GPIO pin. Only works if configured as output.
	/// @param gpioId gpioId Specify a GPIO pin such as GPIO::GPIO0
	/// @returns the new state of the pin
	int toggleGPIO(GPIO gpioId);

	/// Turn on the user LED
	void setLed();

	/// Turn off the user LED
	void clearLed();

	/// Toggle the stage of the user LED
	/// @returns the new stage of the user LED.
	int toggleLed();

private:
	uint8_t m_ledState;
};

} /* namespace BAGuitar */

#endif /* SRC_BAGPIO_H_ */
