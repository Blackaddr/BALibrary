/*
 * ParameterAutomation.cpp
 *
 *  Created on: April 14, 2018
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

#include "LibBasicFunctions.h"

using namespace BAGuitar;

namespace BALibrary {

///////////////////////////////////////////////////////////////////////////////
// ParameterAutomation
///////////////////////////////////////////////////////////////////////////////
constexpr int LINEAR_SLOPE = 0;

template <class T>
ParameterAutomation<T>::ParameterAutomation()
{
    reconfigure(0.0f, 0.0f, static_cast<size_t>(0), Function::NOT_CONFIGURED);
}

template <class T>
ParameterAutomation<T>::ParameterAutomation(T startValue, T endValue, float durationMilliseconds, Function function)
{
    reconfigure(startValue, endValue, calcAudioSamples(durationMilliseconds), function);
}

template <class T>
ParameterAutomation<T>::ParameterAutomation(T startValue, T endValue, size_t durationSamples, Function function)
{
    reconfigure(startValue, endValue, durationSamples, function);
}

template <class T>
ParameterAutomation<T>::~ParameterAutomation()
{

}

template <class T>
void ParameterAutomation<T>::reconfigure(T startValue, T endValue, float durationMilliseconds, Function function)
{
    reconfigure(startValue, endValue, calcAudioSamples(durationMilliseconds), function);
}

template <class T>
void ParameterAutomation<T>::reconfigure(T startValue, T endValue, size_t durationSamples, Function function)
{
    m_function = function;
    m_startValue = startValue;
    m_endValue = endValue;
    m_currentValueX = static_cast<float>(startValue);
    m_duration = durationSamples;
    m_running = false;

    if (endValue >= startValue) {
        // value is increasing
        m_positiveSlope = true;
    } else {
        // value is decreasing
        m_positiveSlope = false;
    }

    float duration = m_duration / static_cast<float>(AUDIO_BLOCK_SAMPLES);

    // Pre-compute any necessary coefficients
    switch(m_function) {
    case Function::EXPONENTIAL :
        break;
    case Function::LOGARITHMIC :
        break;
    case Function::PARABOLIC :
        break;
    case Function::LOOKUP_TABLE :
        break;

    // Default will be same as LINEAR
    case Function::HOLD   :
        m_coeffs[LINEAR_SLOPE] = (1.0f / static_cast<float>(duration)); // convert duration from ms to sec
        break;
    case Function::LINEAR :
    default :
        // The number of parameter updates will be duration in samples divided by audio sample block size since
        // we only update once per block.
        m_coeffs[LINEAR_SLOPE] = static_cast<float>(endValue - startValue) / duration; // convert duration from ms to sec
        break;
    }
}


template <class T>
void ParameterAutomation<T>::trigger()
{
    if (m_function == Function::HOLD) {
        // The HOLD function will move currentValueX from 0 to 1.0 over the desired duration,
        // but will always return the startValue.
        m_currentValueX = 0.0f;
    } else {
        m_currentValueX = static_cast<float>(m_startValue);
    }
    m_running = true;
    //Serial.println("ParameterAutomation<T>::trigger() called");
}

template <class T>
T ParameterAutomation<T>::getNextValue()
{
    if (m_running == false) {
        return m_startValue;
    }

    if (m_function == Function::HOLD) {
        // HOLD is treated as a special case
        m_currentValueX += m_coeffs[LINEAR_SLOPE];
        if (m_currentValueX >= 1.0) {
            m_running = false;
        }
        return m_startValue;
    }

    switch(m_function) {
    case Function::EXPONENTIAL :
        break;
    case Function::LOGARITHMIC :
        break;
    case Function::PARABOLIC :
        break;
    case Function::LOOKUP_TABLE :
        break;
    case Function::LINEAR :
    default :
        // output = m_currentValueX + slope
        m_currentValueX += m_coeffs[LINEAR_SLOPE];
        break;
    }

    // Check if the automation is finished.
    if ( ( m_positiveSlope && (m_currentValueX >= m_endValue)) ||
         (!m_positiveSlope && (m_currentValueX <= m_endValue)) ) {
        m_running = false;
        return m_endValue;
    } else {
        return static_cast<T>(m_currentValueX);
    }
}

// Template instantiation
//template class MyStack<int, 6>;
template class ParameterAutomation<float>;
template class ParameterAutomation<int>;
template class ParameterAutomation<unsigned>;

///////////////////////////////////////////////////////////////////////////////
// ParameterAutomationSequence
///////////////////////////////////////////////////////////////////////////////
template <class T>
ParameterAutomationSequence<T>::ParameterAutomationSequence(int numStages)
{
    if (numStages < MAX_PARAMETER_SEQUENCES) {
        for (int i=0; i<numStages; i++) {
            m_paramArray[i] = new ParameterAutomation<T>();
        }
        for (int i=numStages; i<MAX_PARAMETER_SEQUENCES; i++) {
            m_paramArray[i] = nullptr;
        }
        m_numStages = numStages;
    }
}

template <class T>
ParameterAutomationSequence<T>::~ParameterAutomationSequence()
{
    for (int i=0; i<m_numStages; i++) {
        if (m_paramArray[i]) {
            delete m_paramArray[i];
        }
    }
}

template <class T>
void ParameterAutomationSequence<T>::setupParameter(int index, T startValue, T endValue, size_t durationSamples, typename ParameterAutomation<T>::Function function)
{
    m_paramArray[index]->reconfigure(startValue, endValue, durationSamples, function);
    m_currentIndex = 0;
}

template <class T>
void ParameterAutomationSequence<T>::setupParameter(int index, T startValue, T endValue, float durationMilliseconds, typename ParameterAutomation<T>::Function function)
{
    m_paramArray[index]->reconfigure(startValue, endValue, durationMilliseconds, function);
    m_currentIndex = 0;
}

template <class T>
void ParameterAutomationSequence<T>::trigger(void)
{
    m_currentIndex = 0;
    m_paramArray[0]->trigger();
    m_running = true;
    //Serial.println("ParameterAutomationSequence<T>::trigger() called");
}

template <class T>
T ParameterAutomationSequence<T>::getNextValue()
{
    // Get the next value
    T nextValue = m_paramArray[m_currentIndex]->getNextValue();

    if (m_running) {
        //Serial.println(String("ParameterAutomationSequence<T>::getNextValue() is ") + nextValue
        //        + String(" from stage ") + m_currentIndex);

        // If current stage is done, trigger the next
        if (m_paramArray[m_currentIndex]->isFinished()) {
            Serial.println(String("Finished stage ") + m_currentIndex);
            m_currentIndex++;

            if (m_currentIndex >= m_numStages) {
                // Last stage already finished
                m_running = false;
                m_currentIndex = 0;
            } else {
                // trigger the next stage
                m_paramArray[m_currentIndex]->trigger();
            }
        }
    }

    return nextValue;
}

template <class T>
bool ParameterAutomationSequence<T>::isFinished()
{
    bool finished = true;
    for (int i=0; i<m_numStages; i++) {
        if (!m_paramArray[i]->isFinished()) {
            finished = false;
            break;
        }
    }
    m_running = !finished;
    return finished;
}

// Template instantiation
template class ParameterAutomationSequence<float>;

}
