/*
 * BAGuitar.h
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

#ifndef SRC_BATGUITAR_H_
#define SRC_BATGUITAR_H_

#include "BAAudioControlWM8731.h" // Codec Control
#include "BASpiMemory.h"
#include "BAGpio.h"

namespace BAGuitar {

#define AUDIO_BLOCK_SIZE 128

// Spi Memory
constexpr int SPI_MAX_ADDR = 131071;

// LED
constexpr int usrLED = 16;

}

#endif /* SRC_BATGUITAR_H_ */
