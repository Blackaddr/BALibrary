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

#include <arm_math.h>
#include "Arduino.h"
#include "Audio.h"

#include "BATypes.h"
#include "LibMemoryManagement.h"

#ifndef __LIBBASICFUNCTIONS_H
#define __LIBBASICFUNCTIONS_H

namespace BAGuitar {

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
size_t        calcAudioSamples(float milliseconds);

/// Calculate the number of audio samples (usually an offset) from
/// a queue position.
/// @param position specifies the index and offset within a queue
/// @returns the number of samples from the start of the queue array to the
/// specified position.
size_t calcOffset(QueuePosition position);

void clearAudioBlock(audio_block_t *block);


void alphaBlend(audio_block_t *out, audio_block_t *dry, audio_block_t* wet, float mix);


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

    /// Construct an audio buffer using a slot configured with the BAGuitar::ExternalSramManager
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

    /// Retrieve an audio block (or samples) from the buffer.
    /// @details when using INTERNAL memory, only supported size is AUDIO_BLOCK_SAMPLES. When using
    /// EXTERNAL, a size smaller than AUDIO_BLOCK_SAMPLES can be requested.
    /// @param dest pointer to the target audio block to write the samples to.
    /// @param offset data will start being transferred offset samples from the start of the audio buffer
    /// @param numSamples default value is AUDIO_BLOCK_SAMPLES, so typically you don't have to specify this parameter.
    /// @returns true on success, false on error.
    bool getSamples(audio_block_t *dest, size_t offset, size_t numSamples = AUDIO_BLOCK_SAMPLES);

    /// When using EXTERNAL memory, this function can return a pointer to the underlying ExtMemSlot object associated
    /// with the buffer.
    /// @returns pointer to the underlying ExtMemSlot.
    ExtMemSlot *getSlot() const { return m_slot; }

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
	IirBiQuadFilter(unsigned numStages, const int32_t *coeffs, int coeffShift = 0);
	virtual ~IirBiQuadFilter();
	bool process(int16_t *output, int16_t *input, size_t numSamples);
private:
	const unsigned NUM_STAGES;
	int32_t *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_casd_df1_inst_q31 m_iirCfg;
	int32_t *m_state = nullptr;
};


class IirBiQuadFilterHQ {
public:
	IirBiQuadFilterHQ() = delete;
	IirBiQuadFilterHQ(unsigned numStages, const int32_t *coeffs, int coeffShift = 0);
	virtual ~IirBiQuadFilterHQ();
	bool process(int16_t *output, int16_t *input, size_t numSamples);
private:


	const unsigned NUM_STAGES;
	int32_t *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_cas_df1_32x64_ins_q31 m_iirCfg;
	int64_t *m_state = nullptr;

};

class IirBiQuadFilterFloat {
public:
	IirBiQuadFilterFloat() = delete;
	IirBiQuadFilterFloat(unsigned numStages, const float *coeffs);
	virtual ~IirBiQuadFilterFloat();
	bool process(float *output, float *input, size_t numSamples);
private:
	const unsigned NUM_STAGES;
	float *m_coeffs = nullptr;

	// ARM DSP Math library filter instance
	arm_biquad_cascade_df2T_instance_f32 m_iirCfg;
	float *m_state = nullptr;

};

///**************************************************************************//**
// * Customer RingBuffer with random access
// *****************************************************************************/
//template <class T>
//class RingBuffer {
//public:
//	RingBuffer() = delete;
//
//	/// Construct a RingBuffer of specified max size
//	/// @param maxSize number of entries in ring buffer
//	RingBuffer(const size_t maxSize) : m_maxSize(maxSize) {
//		m_buffer = new T[maxSize]();
//	}
//	virtual ~RingBuffer(){
//		if (m_buffer) delete [] m_buffer;
//	}
//
//	/// Add an element to the back of the queue
//	/// @param element element to add to queue
//	/// returns 0 if success, otherwise error
//	int push_back(T element) {
//
//		//Serial.println(String("RingBuffer::push_back...") + m_head + String(":") + m_tail + String(":") + m_size);
//		if ( (m_head == m_tail) && (m_size > 0) ) {
//			// overflow
//			Serial.println("RingBuffer::push_back: overflow");
//			return -1;
//		}
//
//		m_buffer[m_head] = element;
//		if (m_head < (m_maxSize-1) ) {
//			m_head++;
//		} else {
//			m_head = 0;
//		}
//		m_size++;
//
//		return 0;
//	}
//
//	/// Remove the element at teh front of the queue
//	/// @returns 0 if success, otherwise error
//	int pop_front() {
//
//		if (m_size == 0) {
//			// buffer is empty
//			//Serial.println("RingBuffer::pop_front: buffer is empty\n");
//			return -1;
//		}
//		if (m_tail < m_maxSize-1) {
//			m_tail++;
//		} else {
//			m_tail = 0;
//		}
//		m_size--;
//		//Serial.println(String("RingBuffer::pop_front: ") + m_head + String(":") + m_tail + String(":") + m_size);
//		return 0;
//	}
//
//	/// Get the element at the front of the queue
//	/// @returns element at front of queue
//	T front() const {
//		return m_buffer[m_tail];
//	}
//
//	/// get the element at the back of the queue
//	/// @returns element at the back of the queue
//	T back() const {
//		return m_buffer[m_head-1];
//	}
//
//	/// Get a previously pushed elememt
//	/// @param offset zero is last pushed, 1 is second last, etc.
//	/// @returns the absolute index corresponding to the requested offset.
//	size_t get_index_from_back(size_t offset = 0) const {
//		// the target at m_head - 1 - offset or m_maxSize + m_head -1 - offset;
//		size_t idx = (m_maxSize + m_head -1 - offset);
//
//		if ( idx >= m_maxSize) {
//			idx -= m_maxSize;
//		}
//
//		return idx;
//	}
//
//	/// get the current size of the queue
//	/// @returns size of the queue
//	size_t size() const {
//		return m_size;
//	}
//
//	/// get the maximum size the queue can hold
//	/// @returns maximum size of the queue
//	size_t max_size() const {
//	    return m_maxSize;
//	}
//
//	/// get the element at the specified absolute index
//	/// @param index element to retrieve from absolute queue position
//	/// @returns the request element
//    T& operator[] (size_t index) {
//        return m_buffer[index];
//    }
//
//	/// get the element at the specified absolute index
//	/// @param index element to retrieve from absolute queue position
//	/// @returns the request element
//    T at(size_t index) const {
//    	return m_buffer[index];
//    }
//
//    /// DEBUG: Prints the status of the Ringbuffer. NOte using this much printing will usually cause audio glitches
//    void print() const {
//    	for (int idx=0; idx<m_maxSize; idx++) {
//    		Serial.print(idx + String(" address: ")); Serial.print((uint32_t)m_buffer[idx], HEX);
//    		Serial.print(" data: "); Serial.println((uint32_t)m_buffer[idx]->data, HEX);
//    	}
//    }
//private:
//    size_t m_head=0;        ///< back of the queue
//    size_t m_tail=0;        ///< front of the queue
//    size_t m_size=0;        ///< current size of the qeueu
//    T *m_buffer = nullptr;  ///< pointer to the allocated buffer array
//    const size_t m_maxSize; ///< maximum size of the queue
//};

}


#endif /* __LIBBASICFUNCTIONS_H */
