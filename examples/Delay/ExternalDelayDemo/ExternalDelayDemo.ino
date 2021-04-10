/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo shows to use the BAAudioEffectDelayExternal audio class
 * to create long delays using external SPI RAM.
 * 
 * Eight delay taps are configured from a single SRAM device.
 * 
 */
#include <Audio.h>
#include "BALibrary.h"
#include "BAEffects.h"

using namespace BAEffects;
using namespace BALibrary;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

BAAudioControlWM8731        codecControl;
//AudioEffectDelayExternal longDelay;
BAAudioEffectDelayExternal  longDelay(MemSelect::MEM0); // comment this line to use MEM1
//BAAudioEffectDelayExternal  longDelay(MemSelect::MEM1); // and uncomment this one to use MEM1
AudioMixer4                 delayOutMixerA, delayOutMixerB, delayMixer;

AudioConnection  fromInput(i2sIn,0, longDelay, 0);

AudioConnection  fromDelay0(longDelay,0, delayOutMixerA,0);
AudioConnection  fromDelay1(longDelay,1, delayOutMixerA,1);
AudioConnection  fromDelay2(longDelay,2, delayOutMixerA,2);
AudioConnection  fromDelay3(longDelay,3, delayOutMixerA,3);

AudioConnection  fromDelay4(longDelay,4, delayOutMixerB,0);
AudioConnection  fromDelay5(longDelay,5, delayOutMixerB,1);
AudioConnection  fromDelay6(longDelay,6, delayOutMixerB,2);
AudioConnection  fromDelay7(longDelay,7, delayOutMixerB,3);

AudioConnection  fromDelayA(delayOutMixerA, 0, delayMixer, 0);
AudioConnection  fromDelayB(delayOutMixerB, 0, delayMixer, 1);

AudioConnection  dry(i2sIn, 0, delayMixer, 2);
AudioConnection  outputLeft(delayMixer, 0, i2sOut, 0);
AudioConnection  outputRight(delayMixer, 0, i2sOut, 1);


void setup() {

  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);

  SPI_MEM0_4M();
  //SPI_MEM0_1M(); // use this line instead of you have the older 1Mbit memory

  Serial.begin(57600);
  delay(200);
  AudioMemory(64);
  
  delay(500);
  Serial.println(String("Starting...\n"));
  delay(100);

  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(100);

  // Set up 8 delay taps, each with evenly spaced, longer delays
  longDelay.delay(0, 150.0f);
  longDelay.delay(1, 300.0f);
  longDelay.delay(2, 450.0f);
  longDelay.delay(3, 600.0f);
  longDelay.delay(4, 750.0f);
  longDelay.delay(5, 900.0f);
  longDelay.delay(6, 1050.0f);
  longDelay.delay(7, 1200.0f);

  // mix the first four taps together with increasing attenuation (lower volume)
  delayOutMixerA.gain(0, 1.0f);
  delayOutMixerA.gain(1, 0.8f);
  delayOutMixerA.gain(2, 0.4f);
  delayOutMixerA.gain(3, 0.2f);

  // mix the next four taps with decreasing volume
  delayOutMixerB.gain(0, 0.1f);
  delayOutMixerB.gain(1, 0.05f);
  delayOutMixerB.gain(2, 0.025f);
  delayOutMixerB.gain(3, 0.0125f);
  
  // mix the original signal with the two delay mixers
  delayMixer.gain(0, 1.0f);
  delayMixer.gain(1, 1.0f);
  delayMixer.gain(2, 1.0f);
  
}

void loop() {



}
