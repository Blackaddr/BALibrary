/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This example demonstrates teh BAAudioEffectsAnalogDelay effect. It can
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

//#define USE_EXT // uncomment this line to use External MEM0
#define MIDI_DEBUG // uncomment to see raw MIDI info in terminal

#ifdef USE_EXT
// If using external SPI memory, we will instantiance an SRAM
// manager and create an external memory slot to use as the memory
// for our audio delay
ExternalSramManager externalSram;
ExtMemSlot delaySlot; // Declare an external memory slot.

// Instantiate the AudioEffectAnalogDelay to use external memory by
/// passing it the delay slot.
AudioEffectAnalogDelay analogDelay(&delaySlot);
#else
// If using internal memory, we will instantiate the AudioEffectAnalogDelay
// by passing it the maximum amount of delay we will use in millseconds. Note that
// audio delay lengths are very limited when using internal memory due to limited
// internal RAM size.
AudioEffectAnalogDelay analogDelay(200.0f); // max delay of 200 ms or internal.
// If you use external SPI memory you can get up to 1485.0f ms of delay!
#endif

AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.

// Simply connect the input to the delay, and the output
// to both i2s channels
AudioConnection input(i2sIn,0, analogDelay,0);
AudioConnection delayOut(analogDelay, 0, cabFilter, 0);
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

  // If using external memory request request memory from the manager
  // for the slot
  #ifdef USE_EXT
  SPI_MEM0_1M();
  Serial.println("Using EXTERNAL memory");
  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, 500.0f, MemSelect::MEM0, true);
  #else
  Serial.println("Using INTERNAL memory");
  #endif

  // Configure which MIDI CC's will control the effect parameters
  analogDelay.mapMidiControl(AudioEffectAnalogDelay::BYPASS,16);
  analogDelay.mapMidiControl(AudioEffectAnalogDelay::DELAY,20);
  analogDelay.mapMidiControl(AudioEffectAnalogDelay::FEEDBACK,21);
  analogDelay.mapMidiControl(AudioEffectAnalogDelay::MIX,22);
  analogDelay.mapMidiControl(AudioEffectAnalogDelay::VOLUME,23);

  // Besure to enable the delay. When disabled, audio is is completely blocked
  // to minimize resources to nearly zero.
  analogDelay.enable(); 

  // Set some default values.
  // These can be changed by sending MIDI CC messages over the USB using
  // the BAMidiTester application.
  analogDelay.delay(200.0f); // initial delay of 200 ms
  analogDelay.bypass(false);
  analogDelay.mix(0.5f);
  analogDelay.feedback(0.0f);

  //////////////////////////////////
  // AnalogDelay filter selection //
  // Uncomment to tryout the 3 different built-in filters.
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DM3); // The default filter. Naturally bright echo (highs stay, lows fade away)
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::WARM); // A warm filter with a smooth frequency rolloff above 2Khz
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DARK); // A very dark filter, with a sharp rolloff above 1Khz

  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
}

void OnControlChange(byte channel, byte control, byte value) {
  analogDelay.processMidi(channel, control, value);
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
    Serial.print(" analogDelay: "); Serial.print(analogDelay.processorUsage());
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
