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
using namespace BAEffects;
using namespace BALibrary;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

/// IMPORTANT /////
// YOU MUST USE TEENSYDUINO 1.41 or greater
// YOU MUST COMPILE THIS DEMO USING Serial + Midi

#define MIDI_DEBUG // uncomment to see raw MIDI info in terminal

AudioEffectTremolo tremolo;

AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.

// Simply connect the input to the tremolo, and the output
// to both i2s channels
AudioConnection input(i2sIn,0, tremolo,0);
AudioConnection tremoloOut(tremolo, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);

int loopCount = 0;

void setup() {
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

  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
}

void OnControlChange(byte channel, byte control, byte value) {
  tremolo.processMidi(channel, control, value);
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

void loop() {
  // usbMIDI.read() needs to be called rapidly from loop().  When
  // each MIDI messages arrives, it return true.  The message must
  // be fully processed before usbMIDI.read() is called again.

  if (loopCount % 524288 == 0) {
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" tremolo: "); Serial.print(tremolo.processorUsage());
    Serial.println("%");
  }
  loopCount++;

  // check for new MIDI from USB
  if (usbMIDI.read()) {
    // this code entered only if new MIDI received
    byte type, channel, data1, data2, cable;
    type = usbMIDI.getType();       // which MIDI message, 128-255
    channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
    data1 = usbMIDI.getData1();     // first data byte of message, 0-127
    data2 = usbMIDI.getData2();     // second data byte of message, 0-127
    Serial.println(String("Received a MIDI message on channel ") + channel);
    
    if (type == MidiType::ControlChange) {
      // if type is 3, it's a CC MIDI Message
      // Note: the Arduino MIDI library encodes channels as 1-16 instead
      // of 0 to 15 as it should, so we must subtract one.
      OnControlChange(channel-1, data1, data2);
    }
  }

}
