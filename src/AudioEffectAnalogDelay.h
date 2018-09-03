/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  AudioEffectAnalogDelay is a class for simulating a classic BBD based delay
 *  like the Boss DM-2. This class works with either internal RAM, or external
 *  SPI RAM for longer delays. The exteranl ram uses DMA to minimize load on the
 *  CPU.
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

#ifndef __BAEFFECTS_BAAUDIOEFFECTANALOGDELAY_H
#define __BAEFFECTS_BAAUDIOEFFECTANALOGDELAY_H

#include <Audio.h>
#include "LibBasicFunctions.h"

namespace BAEffects {

/**************************************************************************//**
 * AudioEffectAnalogDelay models BBD based analog delays. It provides controls
 * for delay, feedback (or regen), mix and output level. All parameters can be
 * controlled by MIDI. The class supports internal memory, or external SPI
 * memory by providing an ExtMemSlot. External memory access uses DMA to reduce
 * process load.
 *****************************************************************************/
class AudioEffectAnalogDelay : public AudioStream {
public:

	///< List of AudioEffectAnalogDelay MIDI controllable parameters
	enum {
		BYPASS = 0,  ///<  controls effect bypass
		DELAY,       ///< controls the amount of delay
		FEEDBACK,    ///< controls the amount of echo feedback (regen)
		MIX,         ///< controls the the mix of input and echo signals
		VOLUME,      ///< controls the output volume level
		NUM_CONTROLS ///< this can be used as an alias for the number of MIDI controls
	};

	enum class Filter {
        DM3 = 0,
        WARM,
        DARK
	};

	// *** CONSTRUCTORS ***
	AudioEffectAnalogDelay() = delete;

	/// Construct an analog delay using internal memory by specifying the maximum
	/// delay in milliseconds.
	/// @param maxDelayMs maximum delay in milliseconds. Larger delays use more memory.
	AudioEffectAnalogDelay(float maxDelayMs);

	/// Construct an analog delay using internal memory by specifying the maximum
	/// delay in audio samples.
	/// @param numSamples maximum delay in audio samples. Larger delays use more memory.
	AudioEffectAnalogDelay(size_t numSamples);

	/// Construct an analog delay using external SPI via an ExtMemSlot. The amount of
	/// delay will be determined by the amount of memory in the slot.
	/// @param slot A pointer to the ExtMemSlot to use for the delay.
	AudioEffectAnalogDelay(BALibrary::ExtMemSlot *slot); // requires sufficiently sized pre-allocated memory

	virtual ~AudioEffectAnalogDelay(); ///< Destructor

	// *** PARAMETERS ***

	/// Set the delay in milliseconds.
	/// @param milliseconds the request delay in milliseconds. Must be less than max delay.
	void delay(float milliseconds);

	/// Set the delay in number of audio samples.
	/// @param delaySamples the request delay in audio samples. Must be less than max delay.
	void delay(size_t delaySamples);

	/// Set the delay as a fraction of the maximum delay.
	/// The value should be between 0.0f and 1.0f
	void delayFractionMax(float delayFraction);

	/// Bypass the effect.
	/// @param byp when true, bypass wil disable the effect, when false, effect is enabled.
    /// Note that audio still passes through when bypass is enabled.
	void bypass(bool byp) { m_bypass = byp; }

	/// Get if the effect is bypassed
	/// @returns true if bypassed, false if not bypassed
	bool isBypass() { return m_bypass; }

	/// Toggle the bypass effect
	void toggleBypass() { m_bypass = !m_bypass; }

	/// Set the amount of echo feedback (a.k.a regeneration).
	/// @param feedback a floating point number between 0.0 and 1.0.
	void feedback(float feedback) { m_feedback = feedback; }

	/// Set the amount of blending between dry and wet (echo) at the output.
	/// @param mix When 0.0, output is 100% dry, when 1.0, output is 100% wet. When
	/// 0.5, output is 50% Dry, 50% Wet.
	void mix(float mix) { m_mix = mix; }

	/// Set the output volume. This affect both the wet and dry signals.
	/// @details The default is 1.0.
	/// @param vol Sets the output volume between -1.0 and +1.0
	void volume(float vol) {m_volume = vol; }

	// ** ENABLE  / DISABLE **

	/// Enables audio processing. Note: when not enabled, CPU load is nearly zero.
	void enable() { m_enable = true; }

	/// Disables audio process. When disabled, CPU load is nearly zero.
	void disable() { m_enable = false; }

	// ** MIDI **

	/// Sets whether MIDI OMNI channel is processig on or off. When on,
	/// all midi channels are used for matching CCs.
	/// @param isOmni when true, all channels are processed, when false, channel
	/// must match configured value.
	void setMidiOmni(bool isOmni) { m_isOmni = isOmni; }

	/// Configure an effect parameter to be controlled by a MIDI CC
	/// number on a particular channel.
	/// @param parameter one of the parameter names in the class enum
	/// @param midiCC the CC number from 0 to 127
	/// @param midiChannel the effect will only response to the CC on this channel
	/// when OMNI mode is off.
	void mapMidiControl(int parameter, int midiCC, int midiChannel = 0);

	/// process a MIDI Continous-Controller (CC) message
	/// @param channel the MIDI channel from 0 to 15)
	/// @param midiCC the CC number from 0 to 127
	/// @param value the CC value from 0 to 127
	void processMidi(int channel, int midiCC, int value);

	// ** FILTER COEFFICIENTS **

	/// Set the filter coefficients to one of the presets. See AudioEffectAnalogDelay::Filter
	/// for options.
	/// @details See AudioEffectAnalogDelayFIlters.h for more details.
	/// @param filter the preset filter. E.g. AudioEffectAnalogDelay::Filter::WARM
	void setFilter(Filter filter);

	/// Override the default coefficients with your own. The number of filters stages affects how
	/// much CPU is consumed.
	/// @details The effect uses the CMSIS-DSP library for biquads which requires coefficents.
	/// be in q31 format, which means they are 32-bit signed integers representing -1.0 to slightly
	/// less than +1.0. The coeffShift parameter effectively multiplies the coefficients by 2^shift. <br>
	/// Example: If you really want +1.5, must instead use +0.75 * 2^1, thus 0.75 in q31 format is
	/// (0.75 * 2^31) = 1610612736 and coeffShift = 1.
	/// @param numStages the actual number of filter stages you want to use. Must be <= MAX_NUM_FILTER_STAGES.
	/// @param coeffs pointer to an integer array of coefficients in q31 format.
	/// @param coeffShift Coefficient scaling factor = 2^coeffShift.
	void setFilterCoeffs(int numStages, const int32_t *coeffs, int coeffShift);

	virtual void update(void); ///< update automatically called by the Teesny Audio Library

private:
	audio_block_t *m_inputQueueArray[1];
	bool m_isOmni = false;
	bool m_bypass = true;
	bool m_enable = false;
	bool m_externalMemory = false;
	BALibrary::AudioDelay *m_memory = nullptr;
	size_t m_maxDelaySamples = 0;
	audio_block_t *m_previousBlock = nullptr;
	audio_block_t *m_blockToRelease  = nullptr;
	BALibrary::IirBiQuadFilterHQ *m_iir = nullptr;

	// Controls
	int m_midiConfig[NUM_CONTROLS][2]; // stores the midi parameter mapping
	size_t m_delaySamples = 0;
	float m_feedback = 0.0f;
	float m_mix = 0.0f;
	float m_volume = 1.0f;

	void m_preProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet);
	void m_postProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet);

	// Coefficients
	void m_constructFilter(void);
};

}

#endif /* __BAEFFECTS_BAAUDIOEFFECTANALOGDELAY_H */
