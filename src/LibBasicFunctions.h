/*
 * LibBasicFunctions.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#include <cstddef>
#include <new>

#include "Arduino.h"
#include "Audio.h"

#include "LibMemoryManagement.h"

#ifndef SRC_LIBBASICFUNCTIONS_H_
#define SRC_LIBBASICFUNCTIONS_H_

namespace BAGuitar {

struct QueuePosition {
	int offset;
	int index;
};
QueuePosition calcQueuePosition(float milliseconds);
QueuePosition calcQueuePosition(size_t numSamples);
size_t        calcAudioSamples(float milliseconds);
size_t calcOffset(QueuePosition position);

template <class T>
class RingBuffer; // forward declare

enum class MemType : unsigned {
    MEM_INTERNAL = 0,
    MEM_EXTERNAL
};

struct INTERNAL_MEMORY {};
struct EXTERNAL_MEMORY {};

class AudioDelay {
public:
    AudioDelay() = delete;
    AudioDelay(size_t maxSamples);
    AudioDelay(float maxDelayTimeMs);
    AudioDelay(ExtMemSlot *slot);
    ~AudioDelay();

    // Internal memory member functions
    audio_block_t *addBlock(audio_block_t *blockIn);
    audio_block_t *getBlock(size_t index);
    bool getSamples(audio_block_t *dest, size_t offset, size_t numSamples = AUDIO_BLOCK_SAMPLES);

    // External memory member functions
    ExtMemSlot *getSlot() const { return m_slot; }

private:
    MemType m_type;
    RingBuffer<audio_block_t *> *m_ringBuffer = nullptr;
    ExtMemSlot *m_slot = nullptr;
};

template <class T>
class RingBuffer {
public:
	RingBuffer() = delete;
	RingBuffer(const size_t maxSize) : m_maxSize(maxSize) {
		m_buffer = new T[maxSize];
		//Serial.println(String("New RingBuffer: max size is ") + m_maxSize);
	}
	virtual ~RingBuffer(){
		if (m_buffer) delete [] m_buffer;
	}

	int push_back(T element) {

		Serial.println(String("RingBuffer::push_back...") + m_head + String(":") + m_tail + String(":") + m_size);
		if ( (m_head == m_tail) && (m_size > 0) ) {
			// overflow
			Serial.println("RingBuffer::push_back: overflow");
			while(1) {} // TODO REMOVE
			return -1;
		}

		m_buffer[m_head] = element;
		if (m_head < (m_maxSize-1) ) {
			m_head++;
		} else {
			m_head = 0;
		}
		m_size++;
		//Serial.println(" ...Done push");

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

		//Serial.println(String("BackIndex is ") + idx + String(" address: ") + (uint32_t)m_buffer[idx] + String(" data: ") + (uint32_t)m_buffer[idx]->data);
//		for (int i=0; i<m_maxSize; i++) {
//			Serial.println(i + String(":") + (uint32_t)m_buffer[i]->data);
//		}
		return idx;
	}

	size_t size() const {
		//Serial.println(String("RingBuffer::size: ") + m_head + String(":") + m_tail + String(":") + m_size);
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


#endif /* SRC_LIBBASICFUNCTIONS_H_ */
