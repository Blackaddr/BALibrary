/*
 * LibBasicFunctions.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#include <cstddef>
#include <new>

#include "Arduino.h"

#ifndef SRC_LIBBASICFUNCTIONS_H_
#define SRC_LIBBASICFUNCTIONS_H_

namespace BAGuitar {

struct INTERNAL_MEMORY {};
struct EXTERNAL_MEMORY {};

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

		if ( (m_head == m_tail) && (m_size > 0) ) {
			// overflow
			Serial.println("RingBuffer::push_back: overflow");
			return -1;
		}

		m_buffer[m_head] = element;
		if (m_head < m_maxSize-1) {
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
		return 0;
	}

	T front() const {
		return m_buffer[m_tail];
	}

	size_t size() const {
		size_t size = 0;
		if (m_head == m_tail)     { size = 0; }
		else if (m_head > m_tail) { size =  (m_head - m_tail); }
		else                      { size = (m_tail - m_head + 1); }
		return size;
	}

    T& operator[] (size_t index) {
        return m_buffer[index];
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
