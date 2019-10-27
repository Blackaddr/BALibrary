/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo will provide an audio passthrough, as well as exercise the
 * MIDI interface.
 * 
 */
#include <Wire.h>
#include <Audio.h>
#include <MIDI.h>
#include "BALibrary.h"

MIDI_CREATE_DEFAULT_INSTANCE();
using namespace midi;
using namespace BALibrary;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

// Audio Thru Connection
AudioConnection      patch0(i2sIn,0, i2sOut, 0);
AudioConnection      patch1(i2sIn,1, i2sOut, 1);

BAAudioControlWM8731      codecControl;
BAGpio                    gpio;  // access to User LED

unsigned long t=0;

void setup() {

  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(57600);
  delay(5);
  while (!Serial) { yield(); }

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  AudioMemory(24);

  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(100);
  
}

void loop() {  

  ///////////////////////////////////////////////////////////////////////
  // MIDI TESTING
  // Connect a loopback cable between the MIDI IN and MIDI OUT on the
  // GTA Pro.  This test code will periodically send MIDI events which
  // will loop back and get printed in the Serial Monitor.
  ///////////////////////////////////////////////////////////////////////
  DataByte note, velocity, channel, d1, d2;

  // Send MIDI OUT
  int cc, val=0xA, channelSend = 1;
  for (cc=32; cc<40; cc++) {
    MIDI.sendControlChange(cc, val, channelSend); val++; channelSend++;
    delay(100);
    MIDI.sendNoteOn(10, 100, channelSend);
    delay(100);
  }
  
  if (MIDI.read()) {                    // Is there a MIDI message incoming ?
    MidiType type = MIDI.getType();
    Serial.println(String("MIDI IS WORKING!!!"));
    switch (type) {
      case NoteOn:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        if (velocity > 0) {
          Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        } else {
          Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
        }
        break;
      case NoteOff:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        break;
      default:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
        Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
    }
    t = millis();
  }
  
  if (millis() - t > 10000) {
    t += 10000;
    Serial.println("(no MIDI activity, check cables)");
  }

  // Toggle the USR LED state
  gpio.toggleLed();

}
