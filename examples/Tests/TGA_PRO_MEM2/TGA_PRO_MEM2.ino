#include <Wire.h>
#include <Audio.h>
#include <SPI.h>

#define TGA_PRO_EXPAND_REV2
#include "BALibrary.h"

using namespace BALibrary;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

// Audio Thru Connection
AudioConnection      patch0(i2sIn,0, i2sOut, 0);
AudioConnection      patch1(i2sIn,1, i2sOut, 1);

BAAudioControlWM8731      codec;
BAGpio                    gpio;  // access to User LED

BASpiMemoryDMA spiMem0(SpiDeviceId::SPI_DEVICE0);
BASpiMemoryDMA spiMem1(SpiDeviceId::SPI_DEVICE1);

// Create a control object using the number of switches, pots, encoders and outputs on the
// Blackaddr Audio Expansion Board.
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, BA_EXPAND_NUM_ENC, BA_EXPAND_NUM_LED);

void configPhysicalControls(BAPhysicalControls &controls, BAAudioControlWM8731 &codec);
void checkPot(unsigned id);
void checkSwitch(unsigned id);
bool spiTest(BASpiMemoryDMA *mem); // returns true if passed
bool uartTest();                   // returns true if passed

void setup() {
  Serial.begin(57600);
  
  spiMem0.begin();
  spiMem1.begin();

  // Disable the audio codec first
  codec.disable();
  AudioMemory(128);
  codec.enable();
  codec.setHeadphoneVolume(0.8f); // Set headphone volume
  configPhysicalControls(controls, codec);

//  if (uartTest()) { Serial.println("MIDI Ports testing PASSED!"); }
//  if (spiTest(&spiMem0)) { Serial.println("SPI0 testing PASSED!");}
//  if (spiTest(&spiMem1)) { Serial.println("SPI1 testing PASSED!");}
}

void loop() {
  // put your main code here, to run repeatedly:
  checkPot(0);
  checkPot(1);
  checkPot(2);
  checkSwitch(0);
  checkSwitch(1);
  delay(10);
}
