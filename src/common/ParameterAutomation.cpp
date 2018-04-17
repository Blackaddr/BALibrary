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

constexpr int LINEAR_SLOPE = 0;

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
    m_currentValueX = startValue;
    m_duration = durationSamples;

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
    case Function::LINEAR :
    default :
        m_coeffs[LINEAR_SLOPE] = (endValue - startValue) / static_cast<T>(m_duration);
        break;
    }
}


template <class T>
void ParameterAutomation<T>::trigger()
{
    m_currentValueX = m_startValue;
    m_running = true;
}

template <class T>
T ParameterAutomation<T>::getNextValue()
{
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
    case Function::LINEAR :
    default :
        // output = m_currentValueX + slope
        m_currentValueX += m_coeffs[LINEAR_SLOPE];
        if (m_currentValueX >= m_endValue) {
            m_currentValueX = m_endValue;
            m_running = false;
        }
        break;
    }
    return m_currentValueX;
}

}
