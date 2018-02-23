#include <MIDI.h>
#include "BAGuitar.h"

using namespace BAGuitar;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

/// IMPORTANT /////
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
AudioEffectAnalogDelay analogDelay(200.0f); // max delay of 200 ms.
#endif

AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.

// Record the audio to the PC
//AudioOutputUSB usb;

// Simply connect the input to the delay, and the output
// to both i2s channels
AudioConnection input(i2sIn,0, analogDelay,0);
AudioConnection delayOut(analogDelay, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);
//AudioConnection leftOutUSB(cabFilter,0, usb, 0);
//AudioConnection rightOutUSB(cabFilter,0, usb, 1);

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
    if (type == 3) {
      // if type is 3, it's a CC MIDI Message
      // Note: the Arduino MIDI library encodes channels as 1-16 instead
      // of 0 to 15 as it should, so we must subtract one.
      OnControlChange(channel-1, data1, data2);
    }
  }

}
