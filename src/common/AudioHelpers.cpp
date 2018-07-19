/*
 * AudioHelpers.cpp
 *
 *  Created on: January 1, 2018
 *      Author: slascos
 *
 *  This program is free software: you can redistribute it and/or modify
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
*/

#include "Audio.h"
#include "LibBasicFunctions.h"

namespace BALibrary {

size_t calcAudioSamples(float milliseconds)
{
	return (size_t)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
}

float calcAudioTimeMs(size_t numSamples)
{
	return ((float)(numSamples) / AUDIO_SAMPLE_RATE_EXACT) * 1000.0f;
}

QueuePosition calcQueuePosition(size_t numSamples)
{
	QueuePosition queuePosition;
	queuePosition.index = (int)(numSamples / AUDIO_BLOCK_SAMPLES);
	queuePosition.offset = numSamples % AUDIO_BLOCK_SAMPLES;
	return queuePosition;
}
QueuePosition calcQueuePosition(float milliseconds) {
	size_t numSamples = (int)((milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f);
	return calcQueuePosition(numSamples);
}

size_t calcOffset(QueuePosition position)
{
	return (position.index*AUDIO_BLOCK_SAMPLES) + position.offset;
}

void alphaBlend(audio_block_t *out, audio_block_t *dry, audio_block_t* wet, float mix)
{
	// ARM DSP optimized
	int16_t wetBuffer[AUDIO_BLOCK_SAMPLES];
	int16_t dryBuffer[AUDIO_BLOCK_SAMPLES];
	int16_t scaleFractWet = (int16_t)(mix * 32767.0f);
	int16_t scaleFractDry = 32767-scaleFractWet;

	arm_scale_q15(dry->data, scaleFractDry, 0, dryBuffer, AUDIO_BLOCK_SAMPLES);
	arm_scale_q15(wet->data, scaleFractWet, 0, wetBuffer, AUDIO_BLOCK_SAMPLES);
	arm_add_q15(wetBuffer, dryBuffer, out->data, AUDIO_BLOCK_SAMPLES);
}

void gainAdjust(audio_block_t *out, audio_block_t *in, float vol, int coeffShift)
{
	int16_t scale = (int16_t)(vol * 32767.0f);
	arm_scale_q15(in->data, scale, coeffShift, out->data, AUDIO_BLOCK_SAMPLES);
}

void combine(audio_block_t *out, audio_block_t *in0, audio_block_t *in1)
{
    arm_add_q15 (in0->data, in1->data, out->data, AUDIO_BLOCK_SAMPLES);
}

void clearAudioBlock(audio_block_t *block)
{
	memset(block->data, 0, sizeof(int16_t)*AUDIO_BLOCK_SAMPLES);
}

}

