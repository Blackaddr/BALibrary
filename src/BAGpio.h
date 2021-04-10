/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BAGPio is convenience class for accessing the the various GPIOs available
 *  on the Teensy Guitar Audio series boards.
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

#ifndef __BALIBRARY_BAGPIO_H
#define __BALIBRARY_BAGPIO_H

#include "BAHardware.h"

namespace BALibrary {

/**************************************************************************//**
 * BAGpio provides a convince class to easily control the direction and state
 * of the GPIO pins available on the TGA headers.
 * @details you can always control this directly with Arduino commands like
 * digitalWrite(), etc.
 *****************************************************************************/
class BAGpio {
public:
	/// Construct a GPIO object for controlling the various GPIO and user pins
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

	/// Convert the GPIO enum to the underlying logical pin number
	/// @param gpio the enum value to convert
	/// @returns the logical pin number for the GPIO
	uint8_t enumToPinNumber(GPIO gpio);

private:
	uint8_t m_ledState;
};

} /* namespace BALibrary */

#endif /* __BALIBRARY_BAGPIO_H */
