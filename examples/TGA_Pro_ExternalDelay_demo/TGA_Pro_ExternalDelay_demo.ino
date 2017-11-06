/*************************************************************************
 * This demo uses the BAGuitar library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BAGuitar
 * 
 * This demo demonstrates how to override the default AudioEffectDelayExternal
 * in the Teensy Library with the one in the BAGuitar library.  This is necessary
 * because the SPI pins in AudioEffectDelayExternal are hard-coded and not the
 * same at the TGA Pro.
 * 
 * Simply replace AudioEffectDelayExternal with BAAudioEffectDelayExternal
 * and it should work the same as the default Audio library.
 * 
 * This demo mixes the original guitar signal with one delayed by 1.45 ms
 * the the external SRAM MEM0 on the TGA Pro
 * 
 */
#include <Wire.h>
//#include <Audio.h>
#include <MIDI.h>
#include <SPI.h>
#include "BAGuitar.h"

using namespace BAGuitar;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

BAAudioControlWM8731      codecControl;
BAAudioEffectDelayExternal  longDelay;
AudioMixer4               delayMixer;

// Audio Connections
AudioConnection  fromInput(i2sIn,0, longDelay, 0);
AudioConnection  fromDelayL(longDelay, 0, delayMixer, 0);
AudioConnection  dry(i2sIn, 1, delayMixer, 1);
AudioConnection  outputLeft(delayMixer, 0, i2sOut, 0);
AudioConnection  outputRight(delayMixer, 0, i2sOut, 1);


void setup() {

  Serial.begin(57600);
  delay(1000);

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  AudioMemory(128);

  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(1000);

  longDelay.delay(0, 1450.0f);
  delayMixer.gain(0, 1.0f);
  delayMixer.gain(1, 1.0f);
  
}

void loop() {



}

