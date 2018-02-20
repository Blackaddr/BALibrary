/*
 * IirBiquadFilter.cpp
 *
 *  Created on: January 1, 2018
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

#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BAGuitar {

////////////////////////////////////////////////////
// IirBiQuadFilter
////////////////////////////////////////////////////
IirBiQuadFilter::IirBiQuadFilter(unsigned numStages, const int32_t *coeffs, int coeffShift)
: NUM_STAGES(numStages)
{
	m_coeffs = new int32_t[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(int32_t));

	m_state  = new int32_t[4*numStages];
	arm_biquad_cascade_df1_init_q31(&m_iirCfg, numStages, m_coeffs, m_state, coeffShift);
}

IirBiQuadFilter::~IirBiQuadFilter()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilter::process(int16_t *output, int16_t *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(int16_t));
	} else {

		// create convertion buffers on teh stack
		int32_t input32[numSamples];
		int32_t output32[numSamples];
		for (size_t i=0; i<numSamples; i++) {
			input32[i] = (int32_t)(input[i]);
		}

		arm_biquad_cascade_df1_fast_q31(&m_iirCfg, input32, output32, numSamples);

		for (size_t i=0; i<numSamples; i++) {
			output[i] = (int16_t)(output32[i] & 0xffff);
		}
	}
	return true;
}

// HIGH QUALITY
IirBiQuadFilterHQ::IirBiQuadFilterHQ(unsigned numStages, const int32_t *coeffs, int coeffShift)
: NUM_STAGES(numStages)
{
	m_coeffs = new int32_t[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(int32_t));

	m_state = new int64_t[4*numStages];;
	arm_biquad_cas_df1_32x64_init_q31(&m_iirCfg, numStages, m_coeffs, m_state, coeffShift);
}

IirBiQuadFilterHQ::~IirBiQuadFilterHQ()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilterHQ::process(int16_t *output, int16_t *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(int16_t));
	} else {

		// create convertion buffers on teh stack
		int32_t input32[numSamples];
		int32_t output32[numSamples];
		for (size_t i=0; i<numSamples; i++) {
			input32[i] = (int32_t)(input[i]);
		}

		arm_biquad_cas_df1_32x64_q31(&m_iirCfg, input32, output32, numSamples);

		for (size_t i=0; i<numSamples; i++) {
			output[i] = (int16_t)(output32[i] & 0xffff);
		}
	}
	return true;
}

// FLOAT
IirBiQuadFilterFloat::IirBiQuadFilterFloat(unsigned numStages, const float *coeffs)
: NUM_STAGES(numStages)
{
	m_coeffs = new float[5*numStages];
	memcpy(m_coeffs, coeffs, 5*numStages * sizeof(float));

	m_state = new float[4*numStages];;
	arm_biquad_cascade_df2T_init_f32(&m_iirCfg, numStages, m_coeffs, m_state);
}

IirBiQuadFilterFloat::~IirBiQuadFilterFloat()
{
	if (m_coeffs) delete [] m_coeffs;
	if (m_state)  delete [] m_state;
}


bool IirBiQuadFilterFloat::process(float *output, float *input, size_t numSamples)
{
	if (!output) return false;
	if (!input) {
		// send zeros
		memset(output, 0, numSamples * sizeof(float));
	} else {

		arm_biquad_cascade_df2T_f32(&m_iirCfg, input, output, numSamples);

	}
	return true;
}

}

