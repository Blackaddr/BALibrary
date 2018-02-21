#include <MIDI.h>
#include "BAGuitar.h"

using namespace BAGuitar;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

#define USE_EXT

#ifdef USE_EXT
// If using external SPI memory, we will instantiance an SRAM
// manager and create an external memory slot to use as the memory
// for our audio delay
ExternalSramManager externalSram(1); // Manage only one SRAM.
ExtMemSlot delaySlot; // Declare an external memory slot.

// Instantiate the AudioEffectAnalogDelay to use external memory by
/// passing it the delay slot.
AudioEffectAnalogDelay myDelay(&delaySlot);
#else
// If using internal memory, we will instantiate the AudioEffectAnalogDelay
// by passing it the maximum amount of delay we will use in millseconds. Note that
// audio delay lengths are very limited when using internal memory due to limited
// internal RAM size.
AudioEffectAnalogDelay myDelay(200.0f); // max delay of 200 ms.
#endif

//AudioMixer4              mixer; // Used to mix the original dry with the wet (effects) path.
//
//AudioConnection patch0(i2sIn,0, myDelay,0);
//AudioConnection mixerDry(i2sIn,0, mixer,0);
//AudioConnection mixerWet(myDelay,0, mixer,1);

AudioConnection input(i2sIn,0, myDelay,0);
AudioConnection leftOut(myDelay,0, i2sOut, 0);

int loopCount = 0;

AudioConnection rightOut(myDelay,0, i2sOut, 1);

void setup() {
  delay(100);
  Serial.begin(57600);
  codec.disable();
  delay(100);
  AudioMemory(128);
  delay(5);

  // Setup MIDI
  //usbMIDI.setHandleControlChange(OnControlChange);
  Serial.println("Enabling codec...\n");
  codec.enable();
  delay(100);

  
  #ifdef USE_EXT
  Serial.println("Using EXTERNAL memory");
  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, 1400.0f, MemSelect::MEM1, true);
  #else
  Serial.println("Using INTERNAL memory");
  #endif

  // Configure which MIDI CC's will control the effects
  myDelay.mapMidiControl(AudioEffectAnalogDelay::BYPASS,16);
  myDelay.mapMidiControl(AudioEffectAnalogDelay::DELAY,20);
  myDelay.mapMidiControl(AudioEffectAnalogDelay::FEEDBACK,21);
  myDelay.mapMidiControl(AudioEffectAnalogDelay::MIX,22);
  myDelay.mapMidiControl(AudioEffectAnalogDelay::VOLUME,23);

  // Besure to enable the delay, by default it's processing is off.
  myDelay.enable(); 

  // Set some default values. They can be changed by sending MIDI CC messages
  // over the USB.
  myDelay.delay(200.0f);
  myDelay.bypass(false);
  myDelay.mix(1.0f);
  myDelay.feedback(0.0f);
  
//  mixer.gain(0, 0.0f); // unity gain on the dry
//  mixer.gain(1, 1.0f); // unity gain on the wet

}

void OnControlChange(byte channel, byte control, byte value) {
  myDelay.processMidi(channel, control, value);
  Serial.print("Control Change, ch=");
  Serial.print(channel, DEC);
  Serial.print(", control=");
  Serial.print(control, DEC);
  Serial.print(", value=");
  Serial.print(value, DEC);
  Serial.println();

  
}

void loop() {
  // usbMIDI.read() needs to be called rapidly from loop().  When
  // each MIDI messages arrives, it return true.  The message must
  // be fully processed before usbMIDI.read() is called again.
  //Serial.println(".");

  if (loopCount % 262144 == 0) {
  Serial.print("Processor Usage: "); Serial.print(AudioProcessorUsage());
  Serial.print(" "); Serial.print(AudioProcessorUsageMax());
  Serial.print(" Delay: "); Serial.print(myDelay.processorUsage());
  Serial.print(" "); Serial.println(myDelay.processorUsageMax());
  }
  loopCount++;
  
  if (usbMIDI.read()) {
    byte type, channel, data1, data2, cable;
    type = usbMIDI.getType();       // which MIDI message, 128-255
      channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
      data1 = usbMIDI.getData1();     // first data byte of message, 0-127
      data2 = usbMIDI.getData2();     // second data byte of message, 0-127
      //cable = usbMIDI.getCable();     // which virtual cable with MIDIx8, 0-7
    if (type == 3) {
      OnControlChange(channel-1, data1, data2);
    }
  }

}
