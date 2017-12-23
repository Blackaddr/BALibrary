/*
 * LibBasicFunctions.h
 *
 *  Created on: Dec 23, 2017
 *      Author: slascos
 */

#ifndef SRC_LIBBASICFUNCTIONS_H_
#define SRC_LIBBASICFUNCTIONS_H_

#include "Audio.h"
#include "LibExtMemoryManagement.h"
#include "BASpiMemory.h"

namespace BAGuitar {

void updateAudioMemorySlot(BAGuitar::BASpiMemory *mem, MemSlot slot, audio_block_t *block);
void zeroMemorySlot(BAGuitar::BASpiMemory *mem, MemSlot slot);

}


#endif /* SRC_LIBBASICFUNCTIONS_H_ */
