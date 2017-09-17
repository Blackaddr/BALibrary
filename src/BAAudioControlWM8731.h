/*
 * BAAudioControlWM8731.h
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

#ifndef INC_BAAUDIOCONTROLWM8731_H_
#define INC_BAAUDIOCONTROLWM8731_H_

#include <Audio.h>

namespace BAGuitar {

// use const instead of define for proper scoping
constexpr int WM8731_I2C_ADDR = 0x1A;
constexpr int WM8731_NUM_REGS = 10;

constexpr int WM8731_REG_LLINEIN   = 0;
constexpr int WM8731_REG_RLINEIN   = 1;
constexpr int WM8731_REG_LHEADOUT  = 2;
constexpr int WM8731_REG_RHEADOUT  = 3;
constexpr int WM8731_REG_ANALOG     =4;
constexpr int WM8731_REG_DIGITAL   = 5;
constexpr int WM8731_REG_POWERDOWN = 6;
constexpr int WM8731_REG_INTERFACE = 7;
constexpr int WM8731_REG_SAMPLING  = 8;
constexpr int WM8731_REG_ACTIVE    = 9;
constexpr int WM8731_REG_RESET    = 15;

// Register Masks and Shifts
// Register 0
constexpr int WM8731_LEFT_INPUT_GAIN_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_LEFT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_LEFT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_ADDR = 0;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_MASK = 0x100;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_SHIFT = 8;
// Register 1
constexpr int WM8731_RIGHT_INPUT_GAIN_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_RIGHT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_RIGHT_INPUT_MUTE_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_RIGHT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_ADDR = 1;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_MASK = 0x100;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_SHIFT = 8;
// Register 4
constexpr int WM8731_ADC_BYPASS_ADDR = 4;
constexpr int WM8731_ADC_BYPASS_MASK = 0x8;
constexpr int WM8731_ADC_BYPASS_SHIFT = 3;
constexpr int WM8731_DAC_SELECT_ADDR = 4;
constexpr int WM8731_DAC_SELECT_MASK = 0x10;
constexpr int WM8731_DAC_SELECT_SHIFT = 4;
// Register 5
constexpr int WM8731_DAC_MUTE_ADDR = 5;
constexpr int WM8731_DAC_MUTE_MASK = 0x8;
constexpr int WM8731_DAC_MUTE_SHIFT = 3;
constexpr int WM8731_HPF_DISABLE_ADDR = 5;
constexpr int WM8731_HPF_DISABLE_MASK = 0x1;
constexpr int WM8731_HPF_DISABLE_SHIFT = 0;

// Register 9
constexpr int WM8731_ACTIVATE_ADDR = 9;
constexpr int WM8731_ACTIVATE_MASK = 0x1;


class BAAudioControlWM8731 {
public:
	BAAudioControlWM8731();
	virtual ~BAAudioControlWM8731();

	void disable(void);
	bool enable(void);
	void setLeftInputGain(int val);
	void setRightInputGain(int val);
	void setLeftInMute(bool val);
	void setRightInMute(bool val);
	void setLinkLeftRightIn(bool val);
	void setDacMute(bool val);
	void setDacSelect(bool val);
	void setAdcBypass(bool val);
	void setHPFDisable(bool val);


	void setActivate(bool val);
	void resetCodec(void);

	void writeI2C(unsigned int addr, unsigned int val);
protected:
	int regArray[WM8731_NUM_REGS];

private:
	bool write(unsigned int reg, unsigned int val);
	void resetInternalReg(void);

};

} /* namespace BAGuitar */

#endif /* INC_BAAUDIOCONTROLWM8731_H_ */
