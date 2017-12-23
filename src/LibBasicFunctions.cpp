/*
 * LibBasicFunctions.cpp
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#include "LibBasicFunctions.h"

namespace BAGuitar {

void updateAudioMemorySlot(BASpiMemory *mem, MemSlot slot, audio_block_t *block)
{
	if (block) {
		if (slot.currentPosition + AUDIO_BLOCK_SAMPLES-1 <= slot.end) {
			// entire block fits in memory slot without wrapping
			mem->write16(slot.currentPosition, (uint16_t *)block->data, AUDIO_BLOCK_SAMPLES); // cast audio data to uint.
		} else {
			// this write will wrap the memory slot
			size_t numBytes = slot.end - slot.currentPosition + 1;
			mem->write16(slot.currentPosition, (uint16_t *)block->data, numBytes);
			size_t remainingBytes = AUDIO_BLOCK_SAMPLES - numBytes; // calculate the remaining bytes
			mem->write16(slot.start, (uint16_t *)block->data + numBytes, remainingBytes); // write remaining bytes are start
		}
	}
}


void zeroMemorySlot(BASpiMemory *mem, MemSlot slot)
{
	mem->zero16(slot.start, slot.end-slot.start+1);
}

}

