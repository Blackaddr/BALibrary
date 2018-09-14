/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo provides an example guitar tone consisting of some slap-back delay,
 * followed by a reverb and a low-pass cabinet filter.
 * 
 */
#include <Wire.h>
#include <Audio.h>
#include <MIDI.h>
#include "BALibrary.h"

using namespace BALibrary;

BAAudioControlWM8731      codecControl;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

AudioMixer4              gainModule; // This will be used simply to reduce the gain before the reverb
AudioEffectDelay         delayModule; // we'll add a little slapback echo
AudioEffectReverb        reverb; // Add a bit of 'verb to our tone
AudioMixer4              mixer; // Used to mix the original dry with the wet (effects) path.
AudioFilterBiquad        cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.


// Audio Connections
AudioConnection      patchIn(i2sIn,0, delayModule, 0); // route the input to the delay

AudioConnection      patch2(delayModule,0, gainModule, 0); // send the delay to the gain module
AudioConnection      patch2b(gainModule, 0, reverb, 0); // then to the reverb


AudioConnection      patch1(i2sIn,0, mixer,0); // mixer input 0 is our original dry signal
AudioConnection      patch3(reverb, 0, mixer, 1); // mixer input 1 is our wet

AudioConnection      patch4(mixer, 0, cabFilter, 0); // mixer outpt to the cabinet filter


AudioConnection      patch5(cabFilter, 0, i2sOut, 0); // connect the cab filter to the output.
AudioConnection      patch5b(cabFilter, 0, i2sOut, 1); // connect the cab filter to the output.

void setup() {

  delay(5); // wait a few ms to make sure the GTA Pro is fully powered up
  AudioMemory(48);

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  codecControl.enable();
  delay(100);

  // Configure our effects
  delayModule.delay(0, 50.0f); // 50 ms slapback delay
  gainModule.gain(0, 0.25); // the reverb unit clips easily if the input is too high
  mixer.gain(0, 1.0f); // unity gain on the dry
  mixer.gain(1, 1.0f); // unity gain on the wet

  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
  
  
}

void loop() {  

  // The audio flows automatically through the Teensy Audio Library

}

