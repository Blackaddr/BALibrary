/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo provides a very simple pass-through, clean audio example. 
 * This can be used to double checking everything is hooked up and working correctly.
 *
 * This example also demonstrates the bare minimum code necessary to pass audio
 * through the TGA Pro:
 *     - BAAudioControlWM8731 to enable and control the CODEC
 *     - AudioInputI2S to receive input audio from the CODEC (it's ADC)
 *     - AudioOutputI2S to send output audio to the CODEC (it's DAC)
 * 
 */
#include <Audio.h>
#include "BALibrary.h"

using namespace BALibrary; // This prevents us having to put BALibrary:: in front of all the Blackaddr Audio components

BAAudioControlWM8731 codecControl;
AudioInputI2S        i2sIn;
AudioOutputI2S       i2sOut;

// Audio Connections: name(channel)
// - Setup a mono signal chain, send the mono signal to both output channels in case you are using headphone
// i2sIn(0) --> i2sOut(0)
// i2sIn(1) --> i2sOut(1)
AudioConnection      patch0(i2sIn, 0, i2sOut, 0);  // connect the cab filter to the output.
AudioConnection      patch1(i2sIn, 0, i2sOut, 1); // connect the cab filter to the output.

void setup() {

  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);

  delay(5); // wait a few ms to make sure the GTA Pro is fully powered up
  AudioMemory(48); // Provide an arbitrarily large number of audio buffers (48 blocks) for the effects (delays use a lot more than others)

  // If the codec was already powered up (due to reboot) power it down first
  codecControl.disable();
  delay(100);
  codecControl.enable();
  delay(100);
}

void loop() {  

  // The audio flows automatically through the Teensy Audio Library

}
