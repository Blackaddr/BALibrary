/*
 * AudioEffectAnalogDelay.h
 *
 *  Created on: Jan 7, 2018
 *      Author: slascos
 */

#ifndef SRC_AUDIOEFFECTANALOGDELAY_H_
#define SRC_AUDIOEFFECTANALOGDELAY_H_

#include <Audio.h>
#include "LibBasicFunctions.h"

namespace BAGuitar {

class AudioEffectAnalogDelay : public AudioStream {
public:
	AudioEffectAnalogDelay() = delete;
	AudioEffectAnalogDelay(float maxDelay);
	AudioEffectAnalogDelay(size_t numSamples);
	AudioEffectAnalogDelay(ExtMemSlot *slot); // requires sufficiently sized pre-allocated memory
	virtual ~AudioEffectAnalogDelay();

	virtual void update(void);
	void delay(float milliseconds);
	void delay(size_t delaySamples);

	void bypass(bool byp) { m_bypass = byp; }
	void feedback(float feedback) { m_feedback = feedback; }
	void mix(float mix) { m_mix = mix; }
	void enable() { m_enable = true; }
	void disable() { m_enable = false; }

	void processMidi(int channel, int control, int value);
	void mapMidiBypass(int control, int channel = 0);
	void mapMidiDelay(int control, int channel = 0);
	void mapMidiFeedback(int control, int channel = 0);
	void mapMidiMix(int control, int channel = 0);

private:
	audio_block_t *m_inputQueueArray[1];
	bool m_bypass = true;
	bool m_enable = false;
	bool m_externalMemory = false;
	AudioDelay *m_memory = nullptr;
	size_t m_maxDelaySamples = 0;
	audio_block_t *m_previousBlock = nullptr;
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

#endif /* SRC_AUDIOEFFECTANALOGDELAY_H_ */
