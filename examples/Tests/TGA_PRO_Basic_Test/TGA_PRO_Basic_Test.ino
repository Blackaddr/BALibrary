/*************************************************************************
 * This demo is used for manufacturing testing on the TGA Pro Expansion
 * Control Board.
 * 
 * This will test the following on the TGA Pro:
 * 
 * - Audio INPUT and OUTPUT JACKS
 * - Midi INPUT and Midi OUTPUT jacks
 * - MEM0 (if installed)
 * - User LED
 * 
 * This will also test the Expansion Control Board (if installed):
 * 
 * - three POT knobs
 * - two pushbutton SWitches
 * - two LEDs
 * - headphone output
 * 
 * SETUP INSTRUCTIONS:
 * 
 * 1) Connect an audio source to AUDIO INPUT.
 * 2) Connect AUDIO OUTPUT to amp, stereo, headphone amplifier, etc.
 * 3) if testing the MIDI ports, connect a MIDI cable between MIDI INPUT and MIDI OUTPUT
 * 4) comment out any tests you want to skip
 * 5) Compile and run the demo on your Teensy with TGA Pro.
 * 6) Launch the Arduino Serial Monitor to see results.
 * 
 * TESTING INSTRUCTIONS:
 * 
 * 1) Check the Serial Monitor for the results of the MIDI testing, and external memory testing.
 * 2) Confirm that the audio sent to the INPUT is coming out the OUTPUT.
 * 3) Confirm the User LED is blinking every 1 or 2 seconds
 * 
 * If using the Expansion Control Board:
 * 
 * 1) Try pushing the pushbuttons. When pushed, they should turn on their corresponding LED.
 * 2) Try turn each of the knobs one at a time. They should adjust the volume.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 */

//#define RUN_MIDI_TEST // Uncomment this line to run a MIDI test. You must connect a MIDI cable as a loopback between the MIDI input and output jacks.
//#define RUN_MEMO_TEST // Uncomment this line if you purchased the option SPI RAM.
//#define RUN_EXP_TEST  // Uncomment if you purchased the Expansion Control Board

#include <Audio.h>
#include "BALibrary.h"

using namespace BALibrary;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

// Audio Thru Connection
AudioConnection      patch0(i2sIn,0, i2sOut, 0);
AudioConnection      patch1(i2sIn,1, i2sOut, 1);

BAAudioControlWM8731      codec;
BAGpio                    gpio;  // access to User LED

elapsedMillis timer;

#if defined(RUN_MEMO_TEST)
BASpiMemoryDMA spiMem0(SpiDeviceId::SPI_DEVICE0);
#endif

#if defined(RUN_MEM1_TEST) && !defined(__IMXRT1062__) // SPI1 not supported on T4.0
BASpiMemoryDMA spiMem1(SpiDeviceId::SPI_DEVICE1);
#endif

// Create a control object using the number of switches, pots, encoders and outputs on the
// Blackaddr Audio Expansion Board.
#ifdef RUN_EXP_TEST
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, BA_EXPAND_NUM_ENC, BA_EXPAND_NUM_LED);
void configPhysicalControls(BAPhysicalControls &controls, BAAudioControlWM8731 &codec);
void checkPot(unsigned id);
void checkSwitch(unsigned id);
#endif

bool spiTest(BASpiMemory *mem, int id); // returns true if passed
bool uartTest();                   // returns true if passed

unsigned loopCounter = 0;

void setup() {

  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);

  Serial.begin(57600);
  //while (!Serial) { yield(); }
  delay(500);

  // Disable the audio codec first
  codec.disable();
  delay(100);
  AudioMemory(64);
  codec.enable();
  codec.setHeadphoneVolume(0.8f); // Set headphone volume
#if defined(RUN_EXP_TEST)
  configPhysicalControls(controls, codec);
  Serial.println("Now monitoring for input from Expansion Control Board");
#endif

  // Run the initial Midi connectivity and SPI memory tests.
#if defined(RUN_MIDI_TEST)
  if (uartTest()) { Serial.println("MIDI Ports testing PASSED!"); }
#endif

#if defined(RUN_MEMO_TEST)
  SPI_MEM0_4M(); // Declare the correct memory size
  // SPI_MEM0_1M(); // older boards only had 1M memories 
  spiMem0.begin(); delay(10);
  if (spiTest(&spiMem0, 0)) { Serial.println("SPI0 testing PASSED!");}
#endif
}

void loop() {

  #if defined(RUN_EXP_TEST)
    checkPot(0);
    checkPot(1);
    checkPot(2);
    checkSwitch(0);
    checkSwitch(1);
  #endif

  delay(20); // Without some minimal delay here it will be difficult for the pots/switch changes to be detected.
  
  loopCounter++;
  if (timer > 1000) {
    timer = 0;
    gpio.toggleLed(); // toggle the user LED every 1 second
  }
}
