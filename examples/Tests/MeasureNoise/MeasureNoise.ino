/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo measures the input noise of the TGA Pro. Make sure nothing is physically plugged
 * into the INPUT jack when you run this program. This allows the switching-input jack to ground
 * the input to zero signal.
 * 
 * The test will measure RMS noise and average sample value while toggling the CODEC HPF
 * every few seconds. the CODEC HPF attempts to maximize headroom by modulating it's digital
 * HPF filter offset. This results in near-zero low-frequncy (DC and sub-sonic) content which can
 * be useful for frequency detection but is not appropriate for when sound quality is desired as
 * the modulation of the filter will result in audible artifacts.
 * 
 * ALLOW THE TEST TO RUN THROUGH SEVERAL CYCLE of toggling the HPF so the CODEC can calibrate correctly.
 * 
 */
#include <Wire.h>
#include <Audio.h>
#include "BALibrary.h"
#include "BAEffects.h"

using namespace BALibrary;
using namespace BAEffects;

#define MEASURE_CODEC_PERFORMANCE // uncomment this line to measure internal codec performance, comment the line to measure TGA analog input circuitry performance.

BAAudioControlWM8731     codecControl;
AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;
AudioEffectRmsMeasure    rmsModule;

// Audio Connections
AudioConnection      patchInL(i2sIn,0, rmsModule, 0); // route the input to the delay
AudioConnection      patchInR(i2sIn,1, rmsModule, 1); // route the input to the delay

AudioConnection      patchOutL(rmsModule, 0, i2sOut, 0); // connect the cab filter to the output.
AudioConnection      patchOutR(rmsModule, 0, i2sOut, 1); // connect the cab filter to the output.

void setup() {
  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);

  delay(5); // wait a few ms to make sure the GTA Pro is fully powered up
  AudioMemory(48);

  // If the codec was already powered up (due to reboot) power itd own first
  codecControl.disable();
  delay(100);
  codecControl.enable();
  delay(100);

#if defined(MEASURE_CODEC_PERFORMANCE)
  // Measure TGA board performance with input unplugged, and 0 gain. Please set
  // the gain switch on the TGA Pro to 0 dB.
  Serial.println("Measuring CODEC internal performance");
  codecControl.setLeftInMute(true); // mute the input signal completely
  codecControl.setRightInMute(true);
  codecControl.setHPFDisable(false); // Start with the HPF enabled
#else
  // Measure TGA board performance with input unplugged, and 0 gain. Please set
  // the gain switch on the TGA Pro to 0 dB.
  Serial.println("Measuring TGA Pro analog performance");
  codecControl.setLeftInputGain(23); // 23 = 10111 = 0 dB of CODEC analog gain
  codecControl.setRightInputGain(23);
  codecControl.setLeftInMute(false);
  codecControl.setRightInMute(false);
  codecControl.setHPFDisable(false); // Start with the HPF enabled.
#endif

  rmsModule.enable();
  rmsModule.bypass(false);

  codecControl.recalibrateDcOffset();
  
  
}

unsigned loopCount = 0;
bool isHpfEnabled = true;

void loop() {  

  // The audio flows automatically through the Teensy Audio Library

  if (loopCount > 100000000) {
    if (isHpfEnabled) {
        Serial.println("Setting HPF disable to true");
        codecControl.setHPFDisable(true);

        isHpfEnabled = false;
    } else {
        Serial.println("Setting HPF disable to false");
        codecControl.setHPFDisable(false);
        isHpfEnabled = true;
    }
    loopCount = 0;
  }

  loopCount++;

}
