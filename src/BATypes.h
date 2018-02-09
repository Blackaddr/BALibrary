/*
 * BATypes.h
 *
 *  Created on: Feb 7, 2018
 *      Author: slascos
 */

#ifndef SRC_BATYPES_H_
#define SRC_BATYPES_H_

namespace BAGuitar {

/**************************************************************************//**
 * Customer RingBuffer with random access
 *****************************************************************************/
template <class T>
class RingBuffer {
public:
	RingBuffer() = delete;

	/// Construct a RingBuffer of specified max size
	/// @param maxSize number of entries in ring buffer
	RingBuffer(const size_t maxSize) : m_maxSize(maxSize) {
		m_buffer = new T[maxSize]();
	}
	virtual ~RingBuffer(){
		if (m_buffer) delete [] m_buffer;
	}

	/// Add an element to the back of the queue
	/// @param element element to add to queue
	/// returns 0 if success, otherwise error
	int push_back(T element) {

		//Serial.println(String("RingBuffer::push_back...") + m_head + String(":") + m_tail + String(":") + m_size);
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

	/// Remove the element at teh front of the queue
	/// @returns 0 if success, otherwise error
	int pop_front() {

		if (m_size == 0) {
			// buffer is empty
			//Serial.println("RingBuffer::pop_front: buffer is empty\n");
			return -1;
		}
		if (m_tail < m_maxSize-1) {
			m_tail++;
		} else {
			m_tail = 0;
		}
		m_size--;
		//Serial.println(String("RingBuffer::pop_front: ") + m_head + String(":") + m_tail + String(":") + m_size);
		return 0;
	}

	/// Get the element at the front of the queue
	/// @returns element at front of queue
	T front() const {
		return m_buffer[m_tail];
	}

	/// get the element at the back of the queue
	/// @returns element at the back of the queue
	T back() const {
		return m_buffer[m_head-1];
	}

	/// Get a previously pushed elememt
	/// @param offset zero is last pushed, 1 is second last, etc.
	/// @returns the absolute index corresponding to the requested offset.
	size_t get_index_from_back(size_t offset = 0) const {
		// the target at m_head - 1 - offset or m_maxSize + m_head -1 - offset;
		size_t idx = (m_maxSize + m_head -1 - offset);

		if ( idx >= m_maxSize) {
			idx -= m_maxSize;
		}

		return idx;
	}

	/// get the current size of the queue
	/// @returns size of the queue
	size_t size() const {
		return m_size;
	}

	/// get the maximum size the queue can hold
	/// @returns maximum size of the queue
	size_t max_size() const {
	    return m_maxSize;
	}

	/// get the element at the specified absolute index
	/// @param index element to retrieve from absolute queue position
	/// @returns the request element
    T& operator[] (size_t index) {
        return m_buffer[index];
    }

	/// get the element at the specified absolute index
	/// @param index element to retrieve from absolute queue position
	/// @returns the request element
    T at(size_t index) const {
    	return m_buffer[index];
    }

    /// DEBUG: Prints the status of the Ringbuffer. NOte using this much printing will usually cause audio glitches
    void print() const {
    	for (int idx=0; idx<m_maxSize; idx++) {
    		Serial.print(idx + String(" address: ")); Serial.print((uint32_t)m_buffer[idx], HEX);
    		Serial.print(" data: "); Serial.println((uint32_t)m_buffer[idx]->data, HEX);
    	}
    }
private:
    size_t m_head=0;        ///< back of the queue
    size_t m_tail=0;        ///< front of the queue
    size_t m_size=0;        ///< current size of the qeueu
    T *m_buffer = nullptr;  ///< pointer to the allocated buffer array
    const size_t m_maxSize; ///< maximum size of the queue
};

} // BAGuitar


#endif /* SRC_BATYPES_H_ */
