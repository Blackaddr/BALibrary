/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  LibBasicFunctions is a collection of helpful functions and classes that make
 *  it easier to perform common tasks in Audio applications.
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

#include <cstddef>
#include <new>
#include <atomic>

#include <arm_math.h>
#include "Arduino.h"
#include "Audio.h"

#include "BATypes.h"
#include "LibMemoryManagement.h"

#ifndef __BALIBRARY_LIBBASICFUNCTIONS_H
#define __BALIBRARY_LIBBASICFUNCTIONS_H

namespace BALibrary {

/**************************************************************************//**
 * QueuePosition is used for storing the index (in an array of queues) and the
 * offset within an audio_block_t data buffer. Useful for dealing with large
 * windows of audio spread across multiple audio data blocks.
 *****************************************************************************/
struct QueuePosition {
	int offset; ///< offset in samples within an audio_block_t data buffer
	int index;  ///< index in an array of audio data blocks
};

/// Calculate the exact sample position in an array of audio blocks that corresponds
/// to a particular offset given as time.
/// @param milliseconds length of the interval in milliseconds
/// @returns a struct containing the index and offset
QueuePosition calcQueuePosition(float milliseconds);

/// Calculate the exact sample position in an array of audio blocks that corresponds
/// to a particular offset given as a number of samples
/// @param milliseconds length of the interval in milliseconds
/// @returns a struct containing the index and offset
QueuePosition calcQueuePosition(size_t numSamples);

/// Calculate the number of audio samples (rounded up) that correspond to a
/// given length of time.
/// @param milliseconds length of the interval in milliseconds
/// @returns the number of corresonding audio samples.
size_t calcAudioSamples(float milliseconds);

/// Calculate a length of time in milliseconds from the number of audio samples.
/// @param numSamples Number of audio samples to convert to time
/// @return the equivalent time in milliseconds.
float calcAudioTimeMs(size_t numSamples);

/// Calculate the number of audio samples (usually an offset) from
/// a queue position.
/// @param position specifies the index and offset within a queue
/// @returns the number of samples from the start of the queue array to the
/// specified position.
size_t calcOffset(QueuePosition position);

/// Clear the contents of an audio block to zero
/// @param block pointer to the audio block to clear
void clearAudioBlock(audio_block_t *block);

/// Perform an alpha blend between to audio blocks. Performs <br>
/// out = dry*(1-mix) + wet*(mix)
/// @param out pointer to the destination audio block
/// @param dry pointer to the dry audio
/// @param wet pointer to the wet audio
/// @param mix float between 0.0 and 1.0.
void alphaBlend(audio_block_t *out, audio_block_t *dry, audio_block_t* wet, float mix);

/// Applies a gain to the audio via fixed-point scaling according to <br>
/// out = int * (vol * 2^coeffShift)
/// @param out pointer to output audio block
/// @param in  pointer to input audio block
/// @param vol volume coefficient between -1.0 and +1.0
/// @param coeffShift number of bits to shift the coefficient
void gainAdjust(audio_block_t *out, audio_block_t *in, float vol, int coeffShift = 0);

/// Combine two audio blocks through vector addition
/// out[n] = in0[n] + in1[n]
/// @param out pointer to output audio block
/// @param in0 pointer to first input audio block to combine
/// @param in1 pointer to second input audio block to combine
void combine(audio_block_t *out, audio_block_t *in0, audio_block_t *in1);

template <class T>
class RingBuffer; // forward declare so AudioDelay can use it.


/**************************************************************************//**
 * Audio delays are a very common function in audio processing. In addition to
 * being used for simply create a delay effect, it can also be used for buffering
 * a sliding window in time of audio samples. This is useful when combining
 * several audio_block_t data buffers together to form one large buffer for
 * FFTs, etc.
 * @details The buffer works like a queue. You add new audio_block_t when available,
 * and the class will return an old buffer when it is to be discarded from the queue.<br>
 * Note that using INTERNAL memory means the class will only store a queue
 * of pointers to audio_block_t buffers, since the Teensy Audio uses a shared memory
 * approach. When using EXTERNAL memory, data is actually copyied to/from an external
 * SRAM device.
 *****************************************************************************/
constexpr size_t AUDIO_BLOCK_SIZE = sizeof(int16_t)*AUDIO_BLOCK_SAMPLES;
class AudioDelay {
public:
    AudioDelay() = delete;

    /// Construct an audio buffer using INTERNAL memory by specifying the max number
    /// of audio samples you will want.
    /// @param maxSamples equal or greater than your longest delay requirement
    AudioDelay(size_t maxSamples);

    /// Construct an audio buffer using INTERNAL memory by specifying the max amount of
    /// time you will want available in the buffer.
    /// @param maxDelayTimeMs max length of time you want in the buffer specified in milliseconds
    AudioDelay(float maxDelayTimeMs);

    /// Construct an audio buffer using a slot configured with the BALibrary::ExternalSramManager
    /// @param slot a pointer to the slot representing the memory you wish to use for the buffer.
    AudioDelay(ExtMemSlot *slot);

    ~AudioDelay();

    /// Add a new audio block into the buffer. When the buffer is filled,
    /// adding a new block will push out the oldest once which is returned.
    /// @param blockIn pointer to the most recent block of audio
    /// @returns the buffer to be discarded, or nullptr if not filled (INTERNAL), or
    /// not applicable (EXTERNAL).
    audio_block_t *addBlock(audio_block_t *blockIn);

    /// When using INTERNAL memory, returns the pointer for the specified index into buffer.
    /// @details, the most recent block is 0, 2nd most recent is 1, ..., etc.
    /// @param index the specifies how many buffers older than the current to retrieve
    /// @returns a pointer to the requested audio_block_t
    audio_block_t *getBlock(size_t index);

    /// Returns the max possible delay samples. For INTERNAL memory, the delay can be equal to
    /// the full maxValue specified. For EXTERNAL memory, the max delay is actually one audio
    /// block less then the full size to prevent wrapping.
    /// @returns the maximum delay offset in units of samples.
    size_t getMaxDelaySamples();

    /// Retrieve an audio block (or samples) from the buffer into a destination block
    /// @details when using INTERNAL memory, only supported size is AUDIO_BLOCK_SAMPLES. When using
    /// EXTERNAL, a size smaller than AUDIO_BLOCK_SAMPLES can be requested.
    /// @param dest pointer to the target audio block to write the samples to.
    /// @param offsetSamples data will start being transferred offset samples from the start of the audio buffer
    /// @param numSamples default value is AUDIO_BLOCK_SAMPLES, so typically you don't have to specify this parameter.
    /// @returns true on success, false on error.
    bool getSamples(audio_block_t *dest, size_t offsetSamples, size_t numSamples = AUDIO_BLOCK_SAMPLES);

    /// Retrieve an audio block (or samples) from the buffer into a destination sample array
    /// @details when using INTERNAL memory, only supported size is AUDIO_BLOCK_SAMPLES. When using
    /// EXTERNAL, a size smaller than AUDIO_BLOCK_SAMPLES can be requested.
    /// @param dest pointer to the target sample array to write the samples to.
    /// @param offsetSamples data will start being transferred offset samples from the start of the audio buffer
    /// @param numSamples number of samples to transfer
    /// @returns true on success, false on error.
    bool getSamples(int16_t *dest, size_t offsetSamples, size_t numSamples);


    /// Provides linearly interpolated samples between discrete samples in the sample buffer. The SOURCE buffer MUST BE OVERSIZED
    /// to numSamples+1. This is because the last output sample is interpolated from between NUM_SAMPLES and NUM_SAMPLES+1.
    /// @details this function is typically not used with audio blocks directly since you need AUDIO_BLOCK_SAMPLES+1 as the source size
    /// even though output size is still only AUDIO_BLOCK_SAMPLES. Manually create an oversized buffer and fill it with AUDIO_BLOCK_SAMPLES+1.
    /// e.g. 129 instead of 128 samples. The destBuffer does not need to be oversized.
    /// @param extendedSourceBuffer A source array that contains one more input sample than output samples needed.
    /// @param dest pointer to the target sample array to write the samples to.
    /// @param fraction a value between 0.0f and 1.0f that sets the interpolation point between the discrete samples.
    /// @param numSamples number of samples to transfer
    /// @returns true on success, false on error.
    bool interpolateDelay(int16_t *extendedSourceBuffer, int16_t *destBuffer, float fraction, size_t numSamples = AUDIO_BLOCK_SAMPLES);

    /// When using EXTERNAL memory, this function can return a pointer to the underlying ExtMemSlot object associated
    /// with the buffer.
    /// @returns pointer to the underlying ExtMemSlot.
    ExtMemSlot *getSlot() const { return m_slot; }

    /// Calling this function will force the underlying SPI DMA to use an extra copy buffer. This is needed if the user
    /// audio buffers are not guarenteed to be cache aligned.
    /// @returns true if suceess, false if an error occurs
    bool setSpiDmaCopyBuffer(void);

    /// When using INTERNAL memory, this function can return a pointer to the underlying RingBuffer that contains
    /// audio_block_t * pointers.
    /// @returns pointer to the underlying RingBuffer
    RingBuffer<audio_block_t*> *getRingBuffer() const { return m_ringBuffer; }

private:

    /// enumerates whether the underlying memory buffer uses INTERNAL or EXTERNAL memory
    enum class MemType : unsigned {
        MEM_INTERNAL = 0, ///< internal audio_block_t from the Teensy Audio Library is used
        MEM_EXTERNAL      ///< external SPI based ram is used
    };

    MemType m_type;                                      ///< when 0, INTERNAL memory, when 1, external MEMORY.
    RingBuffer<audio_block_t *> *m_ringBuffer = nullptr; ///< When using INTERNAL memory, a RingBuffer will be created.
    ExtMemSlot *m_slot = nullptr;                        ///< When using EXTERNAL memory, an ExtMemSlot must be provided.
    size_t m_maxDelaySamples = 0;                        ///< stores the number of audio samples in the AudioDelay.
    bool m_getSamples(int16_t *dest, size_t offsetSamples, size_t numSamples); ///< operates directly on int16_y buffers
};

/**************************************************************************//**
 * IIR BiQuad Filter - Direct Form I <br>
 * y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]<br>
 * Some design tools (like Matlab assume the feedback coefficients 'a' are negated. You
 * may have to negate your 'a' coefficients.
 * @details Note that the ARM CMSIS-DSP library requires an extra zero between first
 * and second 'b' coefficients. E.g. <br>
 * {b10, 0, b11, b12, a11, a12, b20, 0, b21, b22, a21, a22, ...}
 *****************************************************************************/
class IirBiQuadFilter {
public:
	IirBiQuadFilter() = delete;
	/// Construct a Biquad filter with specified number of stages, coefficients and scaling.
	/// @details See CMSIS-DSP documentation for more details
	/// @param numStages number of biquad stages. Each stage has 5 coefficients.
	/// @param coeffs pointer to an array of Q31 fixed-point coefficients (range -1 to +0.999...)
	/// @param coeffShift coeffs are multiplied by 2^coeffShift to support coefficient range scaling
	IirBiQuadFilter(unsigned numStages, const int32_t *coeffs, int coeffShift = 0);
	virtual ~IirBiQuadFilter();

	/// Reconfigure the filter coefficients.
	/// @details See CMSIS-DSP documentation for more details
	/// @param numStages number of biquad stages. Each stage has 5 coefficients.
	/// @param coeffs pointer to an array of Q31 fixed-point coefficients (range -1 to +0.999...)
	/// @param coeffShift coeffs are multiplied by 2^coeffShift to support coefficient range scaling
	void changeFilterCoeffs(unsigned numStages, const int32_t *coeffs, int coeffShift = 0);

	/// Process the data using the configured IIR filter
	/// @details output and input can be the same pointer if in-place modification is desired
	/// @param output pointer to where the output results will be written
	/// @param input pointer to where the input data will be read from
    /// @param numSampmles number of samples to process
	bool process(int16_t *output, int16_t *input, size_t numSamples);
private:
	const unsigned NUM_STAGES;
	int32_t *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_casd_df1_inst_q31 m_iirCfg;
	int32_t *m_state = nullptr;
};

/**************************************************************************//**
 * A High-precision version of IirBiQuadFilter often necessary for complex, multistage
 * filters. This class uses CMSIS-DSP biquads with 64-bit internal precision instead
 * of 32-bit.
 *****************************************************************************/
class IirBiQuadFilterHQ {
public:
	IirBiQuadFilterHQ() = delete;
    /// Construct a Biquad filter with specified number of stages, coefficients and scaling.
    /// @details See CMSIS-DSP documentation for more details
    /// @param numStages number of biquad stages. Each stage has 5 coefficients.
    /// @param coeffs pointer to an array of Q31 fixed-point coefficients (range -1 to +0.999...)
    /// @param coeffShift coeffs are multiplied by 2^coeffShift to support coefficient range scaling
	IirBiQuadFilterHQ(unsigned maxNumStages, const int32_t *coeffs, int coeffShift = 0);
	virtual ~IirBiQuadFilterHQ();

	/// Reconfigure the filter coefficients.
	/// @details See CMSIS-DSP documentation for more details
	/// @param numStages number of biquad stages. Each stage has 5 coefficients.
	/// @param coeffs pointer to an array of Q31 fixed-point coefficients (range -1 to +0.999...)
	/// @param coeffShift coeffs are multiplied by 2^coeffShift to support coefficient range scaling
	void changeFilterCoeffs(unsigned numStages, const int32_t *coeffs, int coeffShift = 0);

    /// Process the data using the configured IIR filter
    /// @details output and input can be the same pointer if in-place modification is desired
    /// @param output poinvoid combine(audio_block_t *out, audio_block_t *in0, audio_block_t *in1)ter to where the output results will be written
    /// @param input pointer to where the input data will be read from
    /// @param numSampmles number of samples to process
	bool process(int16_t *output, int16_t *input, size_t numSamples);
private:
	const unsigned NUM_STAGES;
	int32_t *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_cas_df1_32x64_ins_q31 m_iirCfg;
	int64_t *m_state = nullptr;
};

/**************************************************************************//**
 * A single-precision floating-point biquad using CMSIS-DSP hardware instructions.
 * @details Use this when IirBiQuadFilterHQ is insufficient, since that version
 * is still faster with 64-bit fixed-point arithmetic.
 *****************************************************************************/
class IirBiQuadFilterFloat {
public:
	IirBiQuadFilterFloat() = delete;

    /// Construct a Biquad filter with specified number of stages and coefficients
    /// @details See CMSIS-DSP documentation for more details
    /// @param numStages number of biquad stages. Each stage has 5 coefficients.
    /// @param coeffs pointer to an array of single-precision floating-point coefficients
	IirBiQuadFilterFloat(unsigned maxNumStages, const float *coeffs);
	virtual ~IirBiQuadFilterFloat();

	/// Reconfigure the filter coefficients.
	/// @details See CMSIS-DSP documentation for more details
	/// @param numStages number of biquad stages. Each stage has 5 coefficients.
    /// @param coeffs pointer to an array of single-precision floating-point coefficients
	void changeFilterCoeffs(unsigned numStages, const float *coeffs);

    /// Process the data using the configured IIR filter
    /// @details output and input can be the same pointer if in-place modification is desired
    /// @param output pointer to where the output results will be written
    /// @param input pointer to where the input data will be read from
	/// @param numberSampmles number of samples to process
	bool process(float *output, float *input, size_t numSamples);
private:
	const unsigned NUM_STAGES;
	float *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_cascade_df2T_instance_f32 m_iirCfg;
	float *m_state = nullptr;

};

} // namespace BALibrary

namespace BALibrary {

/**************************************************************************//**
 * The class will automate a parameter using a trigger from a start value to an
 * end value, using either a preprogrammed function or a user-provided LUT.
 *****************************************************************************/
template <typename T>
class ParameterAutomation
{
public:
    enum class Function : unsigned {
        NOT_CONFIGURED = 0, ///< Initial, unconfigured stage
        HOLD,         ///< f(x) = constant
        LINEAR,       ///< f(x) = x
        EXPONENTIAL,  ///< f(x) = exp(-k*x)
        LOGARITHMIC,  ///< f(x) =
        PARABOLIC,    ///< f(x) = x^2
        LOOKUP_TABLE  ///< f(x) = lut(x)
    };
    ParameterAutomation();
    ParameterAutomation(T startValue, T endValue, size_t durationSamples,     Function function = Function::LINEAR);
    ParameterAutomation(T startValue, T endValue, float durationMilliseconds, Function function = Function::LINEAR);
    virtual ~ParameterAutomation();

    /// set the start and end values for the automation
    /// @param function select which automation curve (function) to use
    /// @param startValue after reset, parameter automation start from this value
    /// @param endValue after the automation duration, paramter will finish at this value
    /// @param durationSamples number of samples to transition from startValue to endValue
    void reconfigure(T startValue, T endValue, size_t durationSamples,     Function function = Function::LINEAR);
    void reconfigure(T startValue, T endValue, float durationMilliseconds, Function function = Function::LINEAR);

    /// Start the automation from startValue
    void trigger();

    /// Retrieve the next calculated automation value
    /// @returns the calculated parameter value of templated type T
    T getNextValue();

    bool isFinished() { return !m_running; }

private:
    Function m_function;
    T m_startValue;
    T m_endValue;
    bool m_running = false;
    float m_currentValueX; ///< the current value of x in f(x)
    size_t m_duration;
    //float m_coeffs[3]; ///< some general coefficient storage
    float m_slopeX;
    float m_scaleY;
    bool m_positiveSlope = true;
};


// TODO: initialize with const number of sequences with null type that automatically skips
// then register each new sequence.
constexpr int MAX_PARAMETER_SEQUENCES = 32;
template <typename T>
class ParameterAutomationSequence
{
public:
    ParameterAutomationSequence() = delete;
    ParameterAutomationSequence(int numStages);
    virtual ~ParameterAutomationSequence();

    void setupParameter(int index, T startValue, T endValue, size_t durationSamples,     typename ParameterAutomation<T>::Function function);
    void setupParameter(int index, T startValue, T endValue, float durationMilliseconds, typename ParameterAutomation<T>::Function function);

    /// Trigger a the automation sequence until numStages is reached or a Function is ParameterAutomation<T>::Function::NOT_CONFIGURED
    void trigger();

    T getNextValue();
    bool isFinished();

private:
    ParameterAutomation<T> *m_paramArray[MAX_PARAMETER_SEQUENCES];
    int m_currentIndex = 0;
    int m_numStages = 0;
    bool m_running = false;
};

/// Supported LFO waveforms
enum class Waveform : unsigned {
	SINE,     ///< sinewave
	TRIANGLE, ///< triangle wave
	SQUARE,   ///< square wave
	SAWTOOTH, ///< sawtooth wave
	RANDOM,   ///< a non-repeating random waveform
	NUM_WAVEFORMS, ///< the number of defined waveforms
};


/**************************************************************************//**
 * The LFO is commonly used on modulation effects where some parameter (delay,
 * volume, etc.) is modulated via  waveform at a frequency below 20 Hz. Waveforms
 * vary between -1.0f and +1.0f.
 * @details this LFO is for operating on vectors of audio block samples.
 *****************************************************************************/
template <class T>
class LowFrequencyOscillatorVector {
public:
	/// Default constructor, uses SINE as default waveform
	LowFrequencyOscillatorVector() {}

	/// Specifies the desired waveform at construction time.
	/// @param waveform specifies desired waveform
	LowFrequencyOscillatorVector(Waveform waveform) : m_waveform(waveform) {};
	/// Default destructor
    ~LowFrequencyOscillatorVector() {}

    /// Change the waveform
	/// @param waveform specifies desired waveform
	void setWaveform(Waveform waveform) { m_waveform = waveform; }

	/// Set the LFO rate in Hertz
	/// @param frequencyHz the LFO frequency in Hertz
	void setRateAudio(float frequencyHz);

	/// Set the LFO rate as a fraction
	/// @param ratio the radians/sample will be 2*pi*ratio
	void setRateRatio(float ratio);

	/// Get the next waveform value
	/// @returns the next vector of phase values
	T *getNextVector();

private:
	void m_updatePhase(); ///< called internally when updating the phase vector
	void m_initPhase(T radiansPerSample); ///< called internally to reset phase upon rate change
	Waveform m_waveform = Waveform::SINE; ///< LFO waveform
	T m_phaseVec[AUDIO_BLOCK_SAMPLES]; ///< the vector of next phase values
	T m_radiansPerBlock = 0.0f; ///< stores the change in radians over one block of data
	T m_outputVec[AUDIO_BLOCK_SAMPLES]; ///< stores the output LFO values
	std::atomic_flag m_phaseLock = ATOMIC_FLAG_INIT; ///< used for thread-safety on m_phaseVec
	const T PI_F = 3.14159265358979323846264338327950288419716939937510582097494459230781640628620899; ///< 2*PI
	const T TWO_PI_F = 2.0 *  3.14159265358979323846264338327950288419716939937510582097494459230781640628620899; ///< 2*PI
	const T PI_DIV2_F = 0.5 *  3.14159265358979323846264338327950288419716939937510582097494459230781640628620899; ///< PI/2
	const T THREE_PI_DIV2_F = 1.5 *  3.14159265358979323846264338327950288419716939937510582097494459230781640628620899; ///< 3*PI/2
    const T TRIANGE_POS_SLOPE = 2.0f/PI_F;
    const T TRIANGE_NEG_SLOPE = -2.0f/PI_F;
    const T SAWTOOTH_SLOPE = -1.0f/PI_F;
};

} // BALibrary


#endif /* __BALIBRARY_LIBBASICFUNCTIONS_H */
