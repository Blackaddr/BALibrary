/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  @brief Measure the RMS noise of a channel
 *
 *  @copyright This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.*
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY__BAEFFECTS_AUDIOEFFECTDELAYEXTERNAL_H; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef __BAEFFECTS_AUDIOEFFECTRMSMEASURE_H
#define __BAEFFECTS_AUDIOEFFECTRMSMEASURE_H

#include <stdint.h>
#include <Audio.h>

namespace BAEffects {

/**************************************************************************//**
 * AudioEffectRmsMeasure
 *****************************************************************************/
class AudioEffectRmsMeasure : public AudioStream {
public:

	// *** CONSTRUCTORS ***
	/// Create the measurement object. Default is to measure and calculate over
	/// approximate 1 second when rate is roughly 44.1 KHz
	/// @param numBlockMeasurements specifies how many audio blocks to calculate the noise
	/// over.
	AudioEffectRmsMeasure(unsigned numBlockMeasurements = 345);

	virtual ~AudioEffectRmsMeasure(); ///< Destructor

	/// Get the most recently calculated RMS value
	/// @returns the non-normalized RMS
	float getRms(void) { return m_rms; }

	/// Get the most recently calculated dBFS value
	/// @returns the RMS as dB with respecdt to dBFS
	float getDb(void) { return m_dbfs; }

	/// Bypass the effect.
	/// @param byp when true, bypass wil disable the effect, when false, effect is enabled.
    /// Note that audio still passes through when bypass is enabled.
	void bypass(bool byp) { m_bypass = byp; }

	/// Get if the effect is bypassed
	/// @returns true if bypassed, false if not bypassed
	bool isBypass() { return m_bypass; }

	/// Toggle the bypass effect
	void toggleBypass() { m_bypass = !m_bypass; }

	/// Set the output volume.
	/// @details The default is 1.0.
	/// @param vol Sets the output volume between -1.0 and +1.0
	void volume(float vol) {m_volume = vol; }

	/// Enables audio processing. Note: when not enabled, CPU load is nearly zero.
	void enable() { m_enable = true; }

	/// Disables audio process. When disabled, CPU load is nearly zero.
	void disable() { m_enable = false; }

	virtual void update(void); ///< update automatically called by the Teesny Audio Library

private:
	audio_block_t *m_inputQueueArray[1];

	bool     m_bypass               = true;
	bool     m_enable               = false;
	float    m_volume               = 1.0f;
	unsigned m_numBlockMeasurements = 0;
	unsigned m_accumulatorCount     = 0;
	int64_t  m_sum                  = 0;
	float    m_rms                  = 0.0f;
	float    m_dbfs                 = 0.0f;

};

}

#endif /* __BAEFFECTS_AUDIOEFFECTRMSMEASURE_H */
