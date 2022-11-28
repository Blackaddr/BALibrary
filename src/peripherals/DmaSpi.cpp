/*
 * DmaSpi.cpp
 *
 *  Created on: Jan 7, 2018
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
#include "DmaSpi.h"

#if defined(__IMXRT1062__) // T4.X
    DmaSpi0 DMASPI0;
    #if defined(ARDUINO_TEENSY_MICROMOD) || defined (ARDUINO_TEENSY41)
        DmaSpi1 DMASPI1;
    #endif

#elif defined(KINETISK)
    DmaSpi0 DMASPI0;
    #if defined(__MK66FX1M0__)
        DmaSpi1 DMASPI1;
        //DmaSpi2 DMASPI2;
    #endif
#elif defined (KINETISL)
    DmaSpi0 DMASPI0;
    DmaSpi1 DMASPI1;
#else
#endif // defined
