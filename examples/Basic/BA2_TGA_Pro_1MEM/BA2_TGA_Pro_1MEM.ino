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
 * It will also peform a sweep of SPI MEM0.
 * 
 * NOTE: SPI MEM0 can be used by a Teensy 3.1/3.2/3.5/3.6.
 * pins.
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
BASpiMemory               spiMem0(SpiDeviceId::SPI_DEVICE0);

unsigned long t=0;

// SPI stuff
unsigned spiAddress0 = 0;
uint16_t spiData0 = 0xABCD;
int spiErrorCount0 = 0;

constexpr int mask0 = 0x5555;
constexpr int mask1 = 0xaaaa;

int maskPhase = 0;
int loopPhase = 0;

void setup() {

  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(57600);
  while (!Serial) { yield(); }
  delay(5);

  SPI_MEM0_1M(); // Set the Spi memory size to 1Mbit

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  AudioMemory(24);

  Serial.println("Sketch: Enabling codec...\n");
  codecControl.enable();
  delay(100);

  Serial.println("Enabling SPI");
  spiMem0.begin();
  
}

int calcData(int spiAddress, int loopPhase, int maskPhase)
{
  int data;

  int phase = ((loopPhase << 1) + maskPhase) & 0x3;
  switch(phase)
  {
    case 0 :
    data = spiAddress ^ mask0;
    break;
    case 1:
    data = spiAddress ^ mask1;
    break;
    case 2:
    data = ~spiAddress ^ mask0;
    break;
    case 3:
    data = ~spiAddress ^ mask1;
    
  }
  return (data & 0xffff);
}

void loop() {

  //////////////////////////////////////////////////////////////////
  // Write test data to the SPI Memory 0
  //////////////////////////////////////////////////////////////////
  maskPhase = 0;
  for (spiAddress0=0; spiAddress0 <= BAHardwareConfig.getSpiMemMaxAddr(0); spiAddress0=spiAddress0+2) {
    if ((spiAddress0 % 32768) == 0) {
      //Serial.print("Writing to ");
      //Serial.println(spiAddress0, HEX);
    }

    spiData0 = calcData(spiAddress0, loopPhase, maskPhase);
    spiMem0.write16(spiAddress0, spiData0);
    maskPhase = (maskPhase+1) % 2;
  }
  Serial.println("SPI0 writing DONE!");

  ///////////////////////////////////////////////////////////////////
  // Read back from the SPI Memory 0
  ///////////////////////////////////////////////////////////////////
  spiErrorCount0 = 0;
  spiAddress0 = 0;
  maskPhase = 0;

  for (spiAddress0=0; spiAddress0 <= BAHardwareConfig.getSpiMemMaxAddr(0); spiAddress0=spiAddress0+2) {
    if ((spiAddress0 % 32768) == 0) {
//      Serial.print("Reading ");
//      Serial.print(spiAddress0, HEX);
    }

    spiData0 = calcData(spiAddress0, loopPhase, maskPhase);
    int data = spiMem0.read16(spiAddress0);
    if (data != spiData0) {
      spiErrorCount0++;
      Serial.println("");
      Serial.print("ERROR MEM0: (expected) (actual):");
      Serial.print(spiData0, HEX); Serial.print(":");
      Serial.print(data, HEX);
      
      delay(100);
    }
    maskPhase = (maskPhase+1) % 2;

    if ((spiAddress0 % 32768) == 0) {
//      Serial.print(", data = ");
//      Serial.println(data, HEX);
//      Serial.println(String(" loopPhase: ") + loopPhase + String(" maskPhase: ") + maskPhase);
    }

    // Break out of test once the error count reaches 10
    if (spiErrorCount0 > 10) { break; }
    
  }

  if (spiErrorCount0 == 0) { Serial.println("SPI0 TEST PASSED!!!"); }

  loopPhase = (loopPhase+1) % 2;
  
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
