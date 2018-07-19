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

namespace BALibrary {

///////////////////////////////////////////////////////////////////////////////
// ParameterAutomation
///////////////////////////////////////////////////////////////////////////////
constexpr int LINEAR_SLOPE = 0;
constexpr float EXPONENTIAL_K = 5.0f;
constexpr float EXP_EXPONENTIAL_K = expf(EXPONENTIAL_K);

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
    m_currentValueX = 0.0f;
    m_duration = durationSamples;
    m_running = false;

    float duration = m_duration / static_cast<float>(AUDIO_BLOCK_SAMPLES);
    m_slopeX = (1.0f / static_cast<float>(duration));
    m_scaleY = abs(endValue - startValue);
    if (endValue >= startValue) {
        // value is increasing
        m_positiveSlope = true;
    } else {
        // value is decreasing
        m_positiveSlope = false;
    }
}


template <class T>
void ParameterAutomation<T>::trigger()
{
    m_currentValueX = 0.0f;
    m_running = true;
}

template <class T>
T ParameterAutomation<T>::getNextValue()
{
    if (m_running == false) {
        return m_startValue;
    }

    m_currentValueX += m_slopeX;
    float value;
    float returnValue;

    switch(m_function) {
    case Function::EXPONENTIAL :

        if (m_positiveSlope) {
            // Growth: f(x) = exp(k*x) / exp(k)
            value = expf(EXPONENTIAL_K*m_currentValueX) / EXP_EXPONENTIAL_K;
        } else {
            // Decay: f(x) = 1 - exp(-k*x)
            value = 1.0f - expf(-EXPONENTIAL_K*m_currentValueX);
        }
        break;
    case Function::PARABOLIC :
        value = m_currentValueX*m_currentValueX;
        break;
    case Function::LOOKUP_TABLE :
    case Function::LINEAR :
    default :
        value = m_currentValueX;
        break;
    }

    // Check if the automation is finished.
    if (m_currentValueX >= 1.0f) {
        m_currentValueX = 0.0f;
        m_running = false;
        return m_endValue;
    }


    if (m_positiveSlope) {
        returnValue = m_startValue + (m_scaleY*value);
    } else {
        returnValue = m_startValue - (m_scaleY*value);
    }
//    Serial.println(String("Start/End values: ") + m_startValue + String(":") + m_endValue);
    //Serial.print("Parameter m_currentValueX is "); Serial.println(m_currentValueX, 6);
    //Serial.print("Parameter returnValue is "); Serial.println(returnValue, 6);
    return returnValue;
}

// Template instantiation
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
    Serial.println(String("setupParameter() called with samples: ") + durationSamples);
    m_paramArray[index]->reconfigure(startValue, endValue, durationSamples, function);
    m_currentIndex = 0;
}

template <class T>
void ParameterAutomationSequence<T>::setupParameter(int index, T startValue, T endValue, float durationMilliseconds, typename ParameterAutomation<T>::Function function)
{
    Serial.print(String("setupParameter() called with time: ")); Serial.println(durationMilliseconds, 6);
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
                Serial.println("Last stage finished");
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
    return !m_running;
}

// Template instantiation
template class ParameterAutomationSequence<float>;
template class ParameterAutomationSequence<int>;
template class ParameterAutomationSequence<unsigned>;

}
