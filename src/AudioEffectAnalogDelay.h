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

#ifndef __BAGUITAR_BAAUDIOEFFECTANALOGDELAY_H
#define __BAGUITAR_BAAUDIOEFFECTANALOGDELAY_H

#include <Audio.h>
#include "LibBasicFunctions.h"

namespace BAGuitar {

/**************************************************************************//**
 * AudioEffectAnalogDelay models BBD based analog delays. It provides controls
 * for delay, feedback (or regen), mix and output level. All parameters can be
 * controlled by MIDI. The class supports internal memory, or external SPI
 * memory by providing an ExtMemSlot. External memory access uses DMA to reduce
 * process load.
 *****************************************************************************/
class AudioEffectAnalogDelay : public AudioStream {
public:
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
	AudioEffectAnalogDelay(ExtMemSlot *slot); // requires sufficiently sized pre-allocated memory

	virtual ~AudioEffectAnalogDelay(); ///< Destructor

	/// Set the delay in milliseconds.
	/// @param milliseconds the request delay in milliseconds. Must be less than max delay.
	void delay(float milliseconds);

	/// Set the delay in number of audio samples.
	/// @param delaySamples the request delay in audio samples. Must be less than max delay.
	void delay(size_t delaySamples);

	/// Bypass the effect.
	/// @param byp when true, bypass wil disable the effect, when false, effect is enabled.
    /// Note that audio still passes through when bypass is enabled.
	void bypass(bool byp) { m_bypass = byp; }

	/// Set the amount of echo feedback (a.k.a regeneration).
	/// @param feedback a floating point number between 0.0 and 1.0.
	void feedback(float feedback) { m_feedback = feedback; }

	/// Set the amount of blending between dry and wet (echo) at the output.
	/// @param mix When 0.0, output is 100% dry, when 1.0, output is 100% wet. When
	/// 0.5, output is 50% Dry, 50% Wet.
	void mix(float mix) { m_mix = mix; }

	/// Enables audio processing. Note: when not enabled, CPU load is nearly zero.
	void enable() { m_enable = true; }

	/// Disables audio process. When disabled, CPU load is nearly zero.
	void disable() { m_enable = false; }

	void processMidi(int channel, int control, int value);
	void mapMidiBypass(int control, int channel = 0);
	void mapMidiDelay(int control, int channel = 0);
	void mapMidiFeedback(int control, int channel = 0);
	void mapMidiMix(int control, int channel = 0);

	virtual void update(void); ///< update automatically called by the Teesny Audio Library

private:
	audio_block_t *m_inputQueueArray[1];
	bool m_bypass = true;
	bool m_enable = false;
	bool m_externalMemory = false;
	AudioDelay *m_memory = nullptr;
	size_t m_maxDelaySamples = 0;
	audio_block_t *m_previousBlock = nullptr;
	audio_block_t *m_blockToRelease  = nullptr;
	IirBiQuadFilterHQ *m_iir = nullptr;

	// Controls
	int m_midiConfig[4][2];
	size_t m_delaySamples = 0;
	float m_feedback = 0.0f;
	float m_mix = 0.0f;

	void m_preProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet);
	void m_postProcessing(audio_block_t *out, audio_block_t *dry, audio_block_t *wet);

	size_t m_callCount = 0;
};

}

#endif /* __BAGUITAR_BAAUDIOEFFECTANALOGDELAY_H */
