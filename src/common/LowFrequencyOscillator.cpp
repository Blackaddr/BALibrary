/*
 * LowFrequencyOscillator.cpp
 *
 *  Created on: October 12, 2018
 *      Author: Steve Lascos
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

#include <assert.h>
#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BALibrary {

constexpr float TWO_PI_F = 2.0f*3.1415927;

template <class T>
LowFrequencyOscillator<T>::LowFrequencyOscillator(Waveform mode, unsigned frequencyHz)
{
	// Given the fixed sample rate, determine how many samples are in the waveform.
	m_periodSamples = AUDIO_SAMPLE_RATE_EXACT / frequencyHz;
	m_radiansPerSample = (float)TWO_PI_F / (float)m_periodSamples;
	m_mode = mode;

	switch(mode) {
	case Waveform::SINE :
		break;
	case Waveform::SQUARE :
		break;
	case Waveform::TRIANGLE :
		break;
	case Waveform::RANDOM :
		break;
	default :
		assert(0); // This occurs if a Waveform type is missing from the switch statement
	}
}

template <class T>
LowFrequencyOscillator<T>::~LowFrequencyOscillator()
{

}

template <class T>
void LowFrequencyOscillator<T>::reset()
{
	m_phase = 0;
}

template <class T>
inline void LowFrequencyOscillator<T>::updatePhase()
{
	//if (m_phase < m_periodSamples-1) { m_phase++; }
	//else { m_phase = 0; }
	m_phase +=  m_radiansPerSample;
	//if (m_phase < (TWO_PI_F-m_radiansPerSample)) { m_phase +=  m_radiansPerSample; }
	//else { m_phase = 0.0f; }
}

template <class T>
T LowFrequencyOscillator<T>::getNext()
{
	T value = 0.0f;
	updatePhase();
	switch(m_mode) {
	case Waveform::SINE :
		value = sin(m_phase);
		break;
	case Waveform::SQUARE :
		break;
	case Waveform::TRIANGLE :
		break;
	case Waveform::RANDOM :
		break;
	default :
		assert(0); // This occurs if a Waveform type is missing from the switch statement
	}

	return value;
}

template class LowFrequencyOscillator<float>;

} // namespace BALibrary

