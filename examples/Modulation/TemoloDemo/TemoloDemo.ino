/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This example demonstrates teh AudioEffectsTremolo effect. It can
 * be controlled using USB MIDI. You can get a free USB MIDI Controller
 * appliation at 
 * http://www.blackaddr.com/downloads/BAMidiTester/
 * or the source code at
 * https://github.com/Blackaddr/BAMidiTester
 * 
 * Even if you don't control the guitar effect with USB MIDI, you must set
 * the Arduino IDE USB-Type under Tools to "Serial + MIDI"
 */
 #include <MIDI.h>
#include "BALibrary.h"
#include "BAEffects.h"

using namespace midi;
MIDI_CREATE_DEFAULT_INSTANCE();

using namespace BAEffects;
using namespace BALibrary;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

/// IMPORTANT /////
// YOU MUST USE TEENSYDUINO 1.41 or greater
// YOU MUST COMPILE THIS DEMO USING Serial + Midi

//#define USE_CAB_FILTER // uncomment this line to add a simple low-pass filter to simulate a cabinet if you are going straight to headphones
#define MIDI_DEBUG // uncomment to see raw MIDI info in terminal

AudioEffectTremolo tremolo;

#if defined(USE_CAB_FILTER)
AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.
#endif

AudioConnection input(i2sIn,0, tremolo,0);
#if defined(USE_CAB_FILTER)
AudioConnection tremOut(tremolo, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);
#else
AudioConnection leftOut(tremolo,0, i2sOut, 0);
AudioConnection rightOut(tremolo,0, i2sOut, 1);
#endif

elapsedMillis timer;

void OnControlChange(byte channel, byte control, byte value) {
  tremolo.processMidi(channel-1, control, value);
  #ifdef MIDI_DEBUG
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.print(value, DEC);
  Serial.println();
  #endif  
}

void setup() {

  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);
  
  delay(100);
  Serial.begin(57600); // Start the serial port

  // Disable the codec first
  codec.disable();
  delay(100);
  AudioMemory(128);
  delay(5);

  // Enable the codec
  Serial.println("Enabling codec...\n");
  codec.enable();
  delay(100);

  // Setup MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  
  // Configure which MIDI CC's will control the effect parameters
  tremolo.mapMidiControl(AudioEffectTremolo::BYPASS,16);
  tremolo.mapMidiControl(AudioEffectTremolo::RATE,20);
  tremolo.mapMidiControl(AudioEffectTremolo::DEPTH,21);
  tremolo.mapMidiControl(AudioEffectTremolo::VOLUME,22);

  // Besure to enable the tremolo. When disabled, audio is is completely blocked
  // to minimize resources to nearly zero.
  tremolo.enable(); 

  // Set some default values.
  // These can be changed by sending MIDI CC messages over the USB using
  // the BAMidiTester application.
  tremolo.rate(0.5f); // initial LFO rate of 1/2 max which is about 10 Hz
  tremolo.bypass(false);
  tremolo.depth(0.5f); // 50% depth modulation

#if defined(USE_CAB_FILTER)
  // Guitar cabinet: Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
#endif
}

void loop() {
  // usbMIDI.read() needs to be called rapidly from loop().

  if (timer > 1000) {
    timer = 0;
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" tremolo: "); Serial.print(tremolo.processorUsage());
    Serial.println("%");
  }

  MIDI.read();
  usbMIDI.read();

}
