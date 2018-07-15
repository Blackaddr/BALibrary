/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  This file constains precomputed co-efficients for the AudioEffectAnalogDelay
 *  class.
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
#include <cstdint>

namespace BAEffects {

// The number of stages in the analog-response Biquad filter
constexpr unsigned MAX_NUM_FILTER_STAGES = 4;
constexpr unsigned NUM_COEFFS_PER_STAGE = 5;

// Matlab/Octave can be helpful to design a filter. Once you have the IIR filter (bz,az) coefficients
// in the z-domain, they can be converted to second-order-sections. AudioEffectAnalogDelay is designed
// to accept up to a maximum of an 8th order filter, broken into four, 2nd order stages.
//
// Second order sections can be created with:
// [sos] = tf2sos(bz,az);
// The results coefficents must be converted the Q31 format required by the ARM CMSIS-DSP library. This means
// all coefficients must lie between -1.0 and +0.9999. If your (bz,az) coefficients exceed this, you must divide
// them down by a power of 2. For example, if your largest magnitude coefficient is -3.5, you must divide by
// 2^shift where 4=2^2 and thus shift = 2. You must then mutliply by 2^31 to get a 32-bit signed integer value
// that represents the required Q31 coefficient.

// BOSS DM-3 Filters
// b(z) = 1.0e-03 * (0.0032    0.0257    0.0900    0.1800    0.2250    0.1800    0.0900    0.0257    0.0032)
// a(z) = 1.0000   -5.7677   14.6935  -21.3811   19.1491  -10.5202    3.2584   -0.4244   -0.0067
constexpr unsigned DM3_NUM_STAGES = 4;
constexpr unsigned DM3_COEFF_SHIFT = 2;
constexpr int32_t DM3[5*MAX_NUM_FILTER_STAGES] = {
    536870912,            988616936,            455608573,            834606945,           -482959709,
    536870912,           1031466345,            498793368,            965834205,           -467402235,
    536870912,           1105821939,            573646688,            928470657,           -448083489,
    2339,                      5093,                 2776,            302068995,              4412722
};


// Blackaddr WARM Filter
// Butterworth, 8th order, cutoff = 2000 Hz
// Matlab/Octave command: [bz, az] = butter(8, 2000/44100/2);
// b(z) = 1.0e-05 * (0.0086    0.0689    0.2411    0.4821    0.6027    0.4821    0.2411    0.0689    0.0086_
// a(z) = 1.0000   -6.5399   18.8246  -31.1340   32.3473  -21.6114    9.0643   -2.1815    0.2306
constexpr unsigned WARM_NUM_STAGES = 4;
constexpr unsigned WARM_COEFF_SHIFT = 2;
constexpr int32_t WARM[5*MAX_NUM_FILTER_STAGES] = {
    536870912,1060309346,523602393,976869875,-481046241,
    536870912,1073413910,536711084,891250612,-391829326,
    536870912,1087173998,550475248,835222426,-333446881,
    46,92,46,807741349,-304811072
};

// Blackaddr DARK Filter
// Chebychev Type II, 8th order, stopband = 60db, cutoff = 1000 Hz
// Matlab command: [bz, az] = cheby2(8, 60, 1000/44100/2);
// b(z) = 0.0009   -0.0066    0.0219   -0.0423    0.0522   -0.0423    0.0219   -0.0066    0.0009
// a(z) = 1.0000   -7.4618   24.3762  -45.5356   53.1991  -39.8032   18.6245   -4.9829    0.5836
constexpr unsigned DARK_NUM_STAGES = 4;
constexpr unsigned DARK_COEFF_SHIFT = 1;
constexpr int32_t DARK[5*MAX_NUM_FILTER_STAGES] = {
	1073741824,-2124867808,1073741824,2107780229,-1043948409,
	1073741824,-2116080466,1073741824,2042553796,-979786242,
	1073741824,-2077777790,1073741824,1964779896,-904264933,
	957356,-1462833,957356,1896884898,-838694612
};

};
