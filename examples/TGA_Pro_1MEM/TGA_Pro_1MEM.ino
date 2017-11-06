/*************************************************************************
 * This demo uses the BAGuitar library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BAGuitar
 * 
 * This demo will provide an audio passthrough, as well as exercise the
 * MIDI interface.
 * 
 * It will also perform a sweeo of the SPI MEM0 external RAM.
 * 
 */
#include <Wire.h>
#include <Audio.h>
#include <MIDI.h>
#include "BAGuitar.h"

MIDI_CREATE_DEFAULT_INSTANCE();
using namespace midi;


using namespace BAGuitar;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

// Audio Thru Connection
AudioConnection      patch0(i2sIn,0, i2sOut, 0);
AudioConnection      patch1(i2sIn,1, i2sOut, 1);

BAAudioControlWM8731      codecControl;
BAGpio                    gpio;  // access to User LED
BASpiMemory               spiMem0(SpiDeviceId::SPI_DEVICE0);
BASpiMemory               spiMem1(SpiDeviceId::SPI_DEVICE1);

unsigned long t=0;

// SPI stuff
int spiAddress0 = 0;
int spiData0 = 0xff;
int spiErrorCount0 = 0;
int spiAddress1 = 0;
int spiData1 = 0xff;
int spiErrorCount1 = 0;


void setup() {

  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(57600);
  delay(5);

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  AudioMemory(24);

  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(100);
  
}

void loop() {  

  //////////////////////////////////////////////////////////////////
  // Write test data to the SPI Memory 0
  //////////////////////////////////////////////////////////////////
  for (spiAddress0=0; spiAddress0 <= SPI_MAX_ADDR; spiAddress0++) {
    if ((spiAddress0 % 32768) == 0) {
      //Serial.print("Writing to ");
      //Serial.println(spiAddress0, HEX);
    }

    //mem0Write(spiAddress0, spiData0);
    spiMem0.write(spiAddress0, spiData0);
    spiData0 = (spiData0-1) & 0xff;
  }
  Serial.println("SPI0 writing DONE!");

  ///////////////////////////////////////////////////////////////////
  // Read back from the SPI Memory 0
  ///////////////////////////////////////////////////////////////////
  spiErrorCount0 = 0;
  spiAddress0 = 0;
  spiData0 = 0xff;

  for (spiAddress0=0; spiAddress0 <= SPI_MAX_ADDR; spiAddress0++) {
    if ((spiAddress0 % 32768) == 0) {
      //Serial.print("Reading ");
      //Serial.print(spiAddress0, HEX);
    }

    //int data = mem0Read(spiAddress0);
    int data = spiMem0.read(spiAddress0);
    if (data != spiData0) {
      spiErrorCount0++;
      Serial.println("");
      Serial.print("ERROR MEM0: (expected) (actual):");
      Serial.print(spiData0, HEX); Serial.print(":");
      Serial.println(data, HEX);
      delay(100);
    }

    if ((spiAddress0 % 32768) == 0) {
      //Serial.print(", data = ");
      //Serial.println(data, HEX);
    }

    spiData0 = (spiData0-1) & 0xff;

    // Break out of test once the error count reaches 10
    if (spiErrorCount0 > 10) { break; }
    
  }

  if (spiErrorCount0 == 0) { Serial.println("SPI0 TEST PASSED!!!"); }
  
  
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

