/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  PitchShift for creating your own audio effects
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

#ifndef __BAEFFECTS_AUDIOEFFECTPITCHSHIFT_H
#define __BAEFFECTS_AUDIOEFFECTPITCHSHIFT_H

#include <Audio.h>
#include "BATypes.h"
#include "LibBasicFunctions.h"

namespace BAEffects {

/**************************************************************************//**
 * AudioEffectPitchShift
 *****************************************************************************/
class AudioEffectPitchShift : public AudioStream {
public:

    static constexpr unsigned ANALYSIS_SIZE = 1024;
    static constexpr unsigned FFT_OVERSAMPLE_FACTOR = 1;
    static constexpr float    FFT_OVERSAMPLE_FACTOR_F = 1.0f;
    static constexpr unsigned SYNTHESIS_SIZE = ANALYSIS_SIZE * FFT_OVERSAMPLE_FACTOR;
    static constexpr float    SYNTHESIS_SIZE_F =  (float)(ANALYSIS_SIZE * FFT_OVERSAMPLE_FACTOR);
    static constexpr float OVERLAP_FACTOR_F = (float)ANALYSIS_SIZE / (float)AUDIO_BLOCK_SAMPLES;

	///< List of AudioEffectTremolo MIDI controllable parameters
	enum {
		BYPASS = 0,  ///<  controls effect bypass
		VOLUME,      ///< controls the output volume level
		NUM_CONTROLS ///< this can be used as an alias for the number of MIDI controls
	};

	// *** CONSTRUCTORS ***
	AudioEffectPitchShift();

	virtual ~AudioEffectPitchShift(); ///< Destructor

	// *** PARAMETERS ***

	/// Bypass the effect.
	/// @param byp when true, bypass wil disable the effect, when false, effect is enabled.
    /// Note that audio still passes through when bypass is enabled.
	void bypass(bool byp) { m_bypass = byp; }

	/// Get if the effect is bypassed
	/// @returns true if bypassed, false if not bypassed
	bool isBypass() { return m_bypass; }

	/// Toggle the bypass effect
	void toggleBypass() { m_bypass = !m_bypass; }

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


	virtual void update(void); ///< update automatically called by the Teesny Audio Library

private:
	audio_block_t *m_inputQueueArray[1];
	//BALibrary::RingBuffer<audio_block_t*> m_inputFifo = BALibrary::RingBuffer<audio_block_t*>(ANALYSIS_SIZE/AUDIO_BLOCK_SAMPLES);
	float m_analysisBuffer[ANALYSIS_SIZE];
	float m_analysisFreqBuffer[2*ANALYSIS_SIZE];
	float m_synthesisFreqBuffer[2*SYNTHESIS_SIZE];
	float m_synthesisBuffer[SYNTHESIS_SIZE];

	bool m_initFailed = false;

	unsigned m_frameIndex = 0;

//	arm_rfft_instance_f32 fftFwdReal, fftInvReal;
//    arm_cfft_radix4_instance_f32 fftFwdComplex, fftInvComplex;
//    float32_t *bufInputReal;
//    float32_t *bufInputComplex;
//    float32_t *bufOutputReal;
//    float32_t *bufOutputComplex;

    //arm_cfft_radix4_instance_f32 fft_inst_fwd, fft_inst_inv;
	//arm_rfft_instance_f32 rfftForwardInst, rfftInverseInst;
	arm_cfft_radix4_instance_f32 cfftForwardInst, cfftInverseInst;

    //uint8_t ifftFlag = 0; // 0 is FFT, 1 is IFFT
    //uint8_t doBitReverse = 1;

	int m_midiConfig[NUM_CONTROLS][2]; // stores the midi parameter mapping
	bool m_isOmni = false;
	bool m_bypass = true;
	bool m_enable = false;

	float m_volume = 1.0f;
	float m_pitchScale = 1.0f;

	void m_ocean(float *inputFreq, float *outputFreq, float frameIndex, float pitchScale);

};

}

#endif /* __BAEFFECTS_AUDIOEFFECTPITCHSHIFT_H */
