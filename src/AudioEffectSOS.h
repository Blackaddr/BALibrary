/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  AudioEffectSOS is a class f
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

#ifndef __BAEFFECTS_BAAUDIOEFFECTSOS_H
#define __BAEFFECTS_BAAUDIOEFFECTSOS_H

#include <Audio.h>
#include "LibBasicFunctions.h"

namespace BAEffects {

/**************************************************************************//**
 * AudioEffectSOS
 *****************************************************************************/
class AudioEffectSOS : public AudioStream {
public:

    ///< List of AudioEffectAnalogDelay MIDI controllable parameters
    enum {
        BYPASS = 0,      ///< controls effect bypass
        GATE_TRIGGER,    ///< begins the gate sequence
        GATE_OPEN_TIME,  ///< controls how long it takes to open the gate
        //GATE_HOLD_TIME,  ///< controls how long the gate stays open at unity
        GATE_CLOSE_TIME, ///< controls how long it takes to close the gate (release)
        CLEAR_FEEDBACK_TRIGGER, ///< begins the sequence to clear out the looping feedback
        FEEDBACK,        ///< controls the amount of feedback, more gives longer SOS sustain
        VOLUME,          ///< controls the output volume level
        NUM_CONTROLS     ///< this can be used as an alias for the number of MIDI controls
    };

    // *** CONSTRUCTORS ***
    AudioEffectSOS() = delete;
    AudioEffectSOS(float maxDelayMs);
    AudioEffectSOS(size_t numSamples);

    /// Construct an analog delay using external SPI via an ExtMemSlot. The amount of
    /// delay will be determined by the amount of memory in the slot.
    /// @param slot A pointer to the ExtMemSlot to use for the delay.
    AudioEffectSOS(BALibrary::ExtMemSlot *slot); // requires sufficiently sized pre-allocated memory

    virtual ~AudioEffectSOS(); ///< Destructor

    void setGateLedGpio(int pinId);

    // *** PARAMETERS ***
    void gateOpenTime(float milliseconds);

    void gateCloseTime(float milliseconds);

    void feedback(float feedback) { m_feedback = feedback; }

    /// Bypass the effect.
    /// @param byp when true, bypass wil disable the effect, when false, effect is enabled.
    /// Note that audio still passes through when bypass is enabled.
    void bypass(bool byp) { m_bypass = byp; }

    /// Activate the gate automation. Input gate will open, then close.
    void trigger() { m_inputGateAuto.trigger(); }

    /// Activate the delay clearing automation. Input signal will mute, gate will open, then close.
    void clear() { m_clearFeedbackAuto.trigger(); }

    /// Set the output volume. This affect both the wet and dry signals.
    /// @details The default is 1.0.
    /// @param vol Sets the output volume between -1.0 and +1.0
    void volume(float vol) {m_volume = vol; }

    // ** ENABLE  / DISABLE **

    /// Enables audio processing. Note: when not enabled, CPU load is nearly zero.
    void enable();

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
    bool m_isOmni = false;
    bool m_bypass = true;
    bool m_enable = false;
    BALibrary::AudioDelay *m_memory = nullptr;
    bool m_externalMemory = true;
    audio_block_t *m_previousBlock = nullptr;
    audio_block_t *m_blockToRelease  = nullptr;
    size_t m_maxDelaySamples = 0;
    int m_gateLedPinId = -1;

    // Controls
    int m_midiConfig[NUM_CONTROLS][2]; // stores the midi parameter mapping
    size_t m_delaySamples = 0;
    float m_openTimeMs = 0.0f;
    float m_closeTimeMs = 0.0f;
    float m_feedback = 0.0f;
    float m_volume = 1.0f;

    // Automated Controls
    BALibrary::ParameterAutomationSequence<float> m_inputGateAuto     = BALibrary::ParameterAutomationSequence<float>(3);
    BALibrary::ParameterAutomationSequence<float> m_clearFeedbackAuto = BALibrary::ParameterAutomationSequence<float>(3);

    // Private functions
    void m_preProcessing (audio_block_t *out, audio_block_t *input, audio_block_t *delayedSignal);
    void m_postProcessing(audio_block_t *out, audio_block_t *input);
};

}

#endif /* __BAEFFECTS_BAAUDIOEFFECTSOS_H */
