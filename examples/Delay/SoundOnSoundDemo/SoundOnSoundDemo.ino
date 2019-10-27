/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * THIS DEMO REQUIRES THE EXTERNAL SRAM MEM0
 * 
 * This demo combines MIDI control with the BAAudioEffectSoundOnSound. You can use
 * the BAMidiTester to control the effect but it's best to use external MIDI footswitch
 * or the Blackaddr Audio Expansion Control Board.
 * 
 * Use must set the Arduino IDE USB-Type to "Serial + MIDI" in the Tools menu.
 * 
 */
#include <Wire.h>
#include <Audio.h>
#include <MIDI.h>
#include <SPI.h>
#include "BALibrary.h"
#include "BAEffects.h"

#include <midi_UsbTransport.h>
static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, uMIDI);


MIDI_CREATE_DEFAULT_INSTANCE();
using namespace midi;

using namespace BAEffects;

#define MIDI_DEBUG

using namespace BALibrary;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

// External SRAM is required for this effect due to the very long
// delays required.
ExternalSramManager externalSram;
ExtMemSlot delaySlot; // Declare an external memory slot.

AudioEffectSOS sos(&delaySlot);

// Add some effects for our soloing channel
AudioEffectDelay         delayModule; // we'll add a little slapback echo
AudioMixer4              gainModule; // This will be used simply to reduce the gain before the reverb
AudioEffectReverb        reverb; // Add a bit of 'verb to our tone
AudioFilterBiquad        cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.
AudioMixer4 mixer;

// Connect the input
AudioConnection inputToSos(i2sIn, 0, sos, 0);
AudioConnection inputToSolo(i2sIn, 0, delayModule, 0);

// Patch cables for the SOLO channel
AudioConnection inputToGain(delayModule, 0, gainModule, 0);
AudioConnection inputToReverb(gainModule, 0, reverb, 0);

// Output Mixer
AudioConnection mixer0input(i2sIn, 0, mixer, 0);  // SOLO Dry Channel
AudioConnection mixer1input(reverb, 0, mixer, 1); // SOLO Wet Channel
AudioConnection mixer2input(sos, 0, mixer, 2); // SOS Channel
AudioConnection inputToCab(mixer, 0, cabFilter, 0);

// CODEC Outputs
AudioConnection outputLeft(cabFilter, 0, i2sOut, 0);
AudioConnection outputRight(cabFilter, 0, i2sOut, 1);

int loopCount = 0;

void OnControlChange(byte channel, byte control, byte value) {
  sos.processMidi(channel-1, control, value);
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

delay(100);
  Serial.begin(57600); // Start the serial port

  // Disable the codec first
  codec.disable();
  delay(100);
  AudioMemory(128);
  delay(5);

  SPI_MEM0_1M(); // Configure the SPI memory size

  // Enable the codec
  Serial.println("Enabling codec...\n");
  codec.enable();
  delay(100);

  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, BAHardwareConfig.getSpiMemSizeBytes(MemSelect::MEM0), MemSelect::MEM0, true);
  //externalSram.requestMemory(&delaySlot, 50.0f, MemSelect::MEM0, true);

  // Setup MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleControlChange(OnControlChange);
  uMIDI.begin(MIDI_CHANNEL_OMNI);
  uMIDI.setHandleControlChange(OnControlChange);

  // Configure the LED to indicate the gate status
  sos.setGateLedGpio(USR_LED_ID);
  
  // Configure which MIDI CC's will control the effect parameters
  sos.mapMidiControl(AudioEffectSOS::BYPASS,16);
  sos.mapMidiControl(AudioEffectSOS::CLEAR_FEEDBACK_TRIGGER,22);
  sos.mapMidiControl(AudioEffectSOS::GATE_TRIGGER,23);
  sos.mapMidiControl(AudioEffectSOS::GATE_OPEN_TIME,20);
  sos.mapMidiControl(AudioEffectSOS::GATE_CLOSE_TIME,21);
  sos.mapMidiControl(AudioEffectSOS::FEEDBACK,24);
  sos.mapMidiControl(AudioEffectSOS::VOLUME,17);

  // Besure to enable the delay. When disabled, audio is is completely blocked
  // to minimize resources to nearly zero.
  sos.enable(); 

  // Set some default values.
  // These can be changed by sending MIDI CC messages over the USB using
  // the BAMidiTester application.
  sos.bypass(false);
  sos.gateOpenTime(3000.0f);
  sos.gateCloseTime(1000.0f);
  sos.feedback(0.9f);

  // Setup effects on the SOLO channel
  gainModule.gain(0, 0.25); // the reverb unit clips easily if the input is too high
  delayModule.delay(0, 50.0f); // 50 ms slapback delay

  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);

  // Setup the Mixer
  mixer.gain(0, 0.5f); // SOLO Dry gain
  mixer.gain(1, 0.5f); // SOLO Wet gain
  mixer.gain(1, 1.0f); // SOS gain
  
}



void loop() {
  // usbMIDI.read() needs to be called rapidly from loop().  When
  // each MIDI messages arrives, it return true.  The message must
  // be fully processed before usbMIDI.read() is called again.

  if (loopCount % 524288 == 0) {
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" sos: "); Serial.print(sos.processorUsage());
    Serial.println("%");
  }
  loopCount++;

  MIDI.read();
  uMIDI.read();

//  // check for new MIDI from USB
//  if (usbMIDI.read()) {
//    // this code entered only if new MIDI received
//    byte type, channel, data1, data2, cable;
//    type = usbMIDI.getType();       // which MIDI message, 128-255
//    channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
//    data1 = usbMIDI.getData1();     // first data byte of message, 0-127
//    data2 = usbMIDI.getData2();     // second data byte of message, 0-127
//    Serial.println(String("Received a MIDI message on channel ") + channel);
//    
//    if (type == MidiType::ControlChange) {
//      // if type is 3, it's a CC MIDI Message
//      // Note: the Arduino MIDI library encodes channels as 1-16 instead
//      // of 0 to 15 as it should, so we must subtract one.
//      OnControlChange(channel-1, data1, data2);
//    }
//  }

}
