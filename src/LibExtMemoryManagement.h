/*
 * ExtMemoryManagement.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#ifndef SRC_LIBEXTMEMORYMANAGEMENT_H_
#define SRC_LIBEXTMEMORYMANAGEMENT_H_

#include <cstddef>

#include "BAHardware.h"

namespace BAGuitar {

struct MemConfig {
	size_t size;
	size_t totalAvailable;
	size_t nextAvailable;
};

struct MemSlot {
	size_t start;
	size_t end;
	size_t currentPosition;
};

class ExternalSramManager {
public:
	ExternalSramManager() = delete;
	ExternalSramManager(BAGuitar::MemSelect mem) {

		// Initialize the static memory configuration structs
		m_memConfig[MEM0].size = MEM0_MAX_ADDR;
		m_memConfig[MEM0].totalAvailable = MEM0_MAX_ADDR;
		m_memConfig[MEM0].nextAvailable = 0;

		m_memConfig[MEM0].size = MEM0_MAX_ADDR;
		m_memConfig[MEM0].totalAvailable = MEM0_MAX_ADDR;
		m_memConfig[MEM0].nextAvailable = 0;
	}


	bool getMemory(BAGuitar::MemSelect mem, size_t size, MemSlot &slot) {
		if (m_memConfig[mem].totalAvailable >= size) {
			slot.start = m_memConfig[mem].nextAvailable;
			slot.end   = slot.start + size -1;
			slot.currentPosition = slot.start;

			// Update the mem config
			m_memConfig[mem].nextAvailable   = slot.end+1;
			m_memConfig[mem].totalAvailable -= size;
			return true;
		} else {
			return false;
		}
	}

	static MemConfig m_memConfig[BAGuitar::NUM_MEM_SLOTS];

};

}

#endif /* SRC_LIBEXTMEMORYMANAGEMENT_H_ */
