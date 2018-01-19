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

#include "Arduino.h"
#include "Audio.h"

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


template <class T>
class RingBuffer {
public:
	RingBuffer() = delete;
	RingBuffer(const size_t maxSize) : m_maxSize(maxSize) {
		m_buffer = new T[maxSize];
	}
	virtual ~RingBuffer(){
		if (m_buffer) delete [] m_buffer;
	}

	int push_back(T element) {

		Serial.println(String("RingBuffer::push_back...") + m_head + String(":") + m_tail + String(":") + m_size);
		if ( (m_head == m_tail) && (m_size > 0) ) {
			// overflow
			Serial.println("RingBuffer::push_back: overflow");
			return -1;
		}

		m_buffer[m_head] = element;
		if (m_head < (m_maxSize-1) ) {
			m_head++;
		} else {
			m_head = 0;
		}
		m_size++;

		return 0;
	}

	int pop_front() {

		if (m_size == 0) {
			// buffer is empty
			Serial.println("RingBuffer::pop_front: buffer is empty\n");
			return -1;
		}
		if (m_tail < m_maxSize-1) {
			m_tail++;
		} else {
			m_tail = 0;
		}
		m_size--;
		Serial.println(String("RingBuffer::pop_front: ") + m_head + String(":") + m_tail + String(":") + m_size);
		return 0;
	}

	T front() const {
		return m_buffer[m_tail];
	}

	T back() const {
		return m_buffer[m_head-1];
	}

	size_t get_index_from_back(size_t offset = 0) const {
		// the target at m_head - 1 - offset or m_maxSize + m_head -1 - offset;
		size_t idx = (m_maxSize + m_head -1 - offset);

		if ( idx >= m_maxSize) {
			idx -= m_maxSize;
		}

		return idx;
	}

	size_t size() const {
		return m_size;
	}

	size_t max_size() const {
	    return m_maxSize;
	}

    T& operator[] (size_t index) {
        return m_buffer[index];
    }

    T at(size_t index) const {
    	return m_buffer[index];
    }

    void print() const {
    	for (int idx=0; idx<m_maxSize; idx++) {
    		Serial.println(idx + String(" address: ") + (uint32_t)m_buffer[idx] + String(" data: ") + (uint32_t)m_buffer[idx]->data);
    	}
    }
private:
    size_t m_head=0;
    size_t m_tail=0;
    size_t m_size=0;
    T *m_buffer = nullptr;
    const size_t m_maxSize;
};

}


#endif /* __LIBBASICFUNCTIONS_H */
