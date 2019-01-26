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

template <class T>
void LowFrequencyOscillatorVector<T>::m_initPhase(T radiansPerSample)
{
	// Initialize the phase vector starting at 0 radians, and incrementing
	// by radiansPerSample for each element in the vector.
	T initialPhase[AUDIO_BLOCK_SAMPLES];
	for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
		initialPhase[i] = (T)i * radiansPerSample;
	}
	m_radiansPerBlock = radiansPerSample * (T)AUDIO_BLOCK_SAMPLES;

	// there could be different threads controlling the LFO rate and consuming
	// the LFO output, so we need to protected the m_phaseVec for thread-safety.
	while (m_phaseLock.test_and_set()) {}
	memcpy(m_phaseVec, initialPhase, sizeof(T)*AUDIO_BLOCK_SAMPLES);
	m_phaseLock.clear();
}

// This function takes in the frequency of the LFO in hertz and uses knowledge
// about the the audio sample rate to calcuate the correct radians per sample.
template <class T>
void LowFrequencyOscillatorVector<T>::setRateAudio(float frequencyHz)
{
	T radiansPerSample;
	if (frequencyHz == 0) {
		radiansPerSample = 0;
	} else {
		T periodSamples = AUDIO_SAMPLE_RATE_EXACT / frequencyHz;
		radiansPerSample = (T)TWO_PI_F / periodSamples;
	}
	m_initPhase(radiansPerSample);
}


// This function is used when the LFO is being called at some rate other than
// the audio rate. Here you can manually set the radians per sample as a fraction
// of 2*PI
template <class T>
void LowFrequencyOscillatorVector<T>::setRateRatio(float ratio)
{
	T radiansPerSample;
	if (ratio == 0) {
		radiansPerSample = 0;
	} else {
		radiansPerSample = (T)TWO_PI_F * ratio;
	}
	m_initPhase(radiansPerSample);
}

// When this function is called, it will update the phase vector by incrementing by
// radians per block which is radians per sample * block size.
template <class T>
inline void LowFrequencyOscillatorVector<T>::m_updatePhase()
{
	if (m_phaseLock.test_and_set()) { return; }

	if (m_phaseVec[0] > TWO_PI_F) {
		arm_offset_f32(m_phaseVec, -TWO_PI_F + m_radiansPerBlock, m_phaseVec, AUDIO_BLOCK_SAMPLES);
	} else {
		arm_offset_f32(m_phaseVec, m_radiansPerBlock, m_phaseVec, AUDIO_BLOCK_SAMPLES);
	}
	m_phaseLock.clear();
}

// This function will compute the vector of samples for the output waveform using
// the current phase vector.
template <class T>
T *LowFrequencyOscillatorVector<T>::getNextVector()
{
	switch(m_waveform) {
	case Waveform::SINE :
		for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
			m_outputVec[i] = arm_sin_f32(m_phaseVec[i]);
		}
		break;
	case Waveform::SQUARE :
		for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {

		    if (m_phaseVec[i] < PI_F) {
		        m_outputVec[i] = -1.0f;
		    } else {
		        m_outputVec[i] = 1.0f;
		    }
		}
		break;
	case Waveform::TRIANGLE :
	    // A triangle is made up from two different line equations of form y=mx+b
	    // where m = +2/pi or -2/pi. A "Triangle Cos" starts at +1.0 and moves to
	    // -1.0 from angles 0 to PI radians.
		for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
            if (m_phaseVec[i] < PI_F) {
                // y = (-2/pi)*x + 1.0
                m_outputVec[i] = (TRIANGE_NEG_SLOPE * m_phaseVec[i]) + 1.0f;
            } else {
                // y = (2/pi)*x -1.0
                m_outputVec[i] = (TRIANGE_POS_SLOPE * (m_phaseVec[i]-PI_F)) - 1.0f;
            }
		}
		break;
	case Waveform::SAWTOOTH :
	    // A sawtooth is made up of a single equation of the form y=mx+b
	    // where m = -pi + 1.0
	    for (auto i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
	        m_outputVec[i] = (SAWTOOTH_SLOPE * m_phaseVec[i]) + 1.0f;
	    }
	    break;
	case Waveform::RANDOM :
		break;
	default :
		assert(0); // This occurs if a Waveform type is missing from the switch statement
	}

	m_updatePhase();
	return m_outputVec;
}

template class LowFrequencyOscillatorVector<float>;

} // namespace BALibrary

