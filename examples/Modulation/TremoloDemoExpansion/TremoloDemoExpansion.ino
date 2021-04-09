/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This example demonstrates teh BAAudioEffectsTremolo effect. It can
 * be controlled using the Blackaddr Audio "Expansion Control Board".
 * 
 * POT1 (left) controls the tremolo RATE
 * POT2 (right) controls the tremolo DEPTH
 * POT3 (center) controls the output VOLUME
 * SW1 will enable/bypass the audio effect. LED1 will be on when effect is enabled.
 * SW2 will cycle through the 3 pre-programmed waveforms. NOTE: any waveform other than Sine will cause
 *     pops/clicks since the waveforms are not bandlimited.
 * 
 * 
 * Using the Serial Montitor, send 'u' and 'd' characters to increase or decrease
 * the headphone volume between values of 0 and 9.
 */

#include "BALibrary.h"
#include "BAEffects.h"

using namespace BAEffects;
using namespace BALibrary;

//#define USE_CAB_FILTER // uncomment this line to add a simple low-pass filter to simulate a cabinet if you are going straight to headphones

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

AudioEffectTremolo tremolo;

#if defined(USE_CAB_FILTER)
AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.
#endif

AudioConnection input(i2sIn,0, tremolo,0);
#if defined(USE_CAB_FILTER)
AudioConnection tremOut(tremolo, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);
#else
AudioConnection leftOut(tremolo,0, i2sOut, 0);
AudioConnection rightOut(tremolo,0, i2sOut, 1);
#endif


//////////////////////////////////////////
// SETUP PHYSICAL CONTROLS
// - POT1 (left) will control the rate
// - POT2 (right) will control the depth
// - POT3 (centre) will control the volume
// - SW1  (left) will be used as a bypass control
// - LED1 (left) will be illuminated when the effect is ON (not bypass)
// - SW2  (right) will be used to cycle through the the waveforms
// - LED2 (right) will illuminate when pressing SW2.
//////////////////////////////////////////
// To get the calibration values for your particular board, first run the
// BAExpansionCalibrate.ino example and 
constexpr int  potCalibMin = 1;
constexpr int  potCalibMax = 1018;
constexpr bool potSwapDirection = true;

// Create a control object using the number of switches, pots, encoders and outputs on the
// Blackaddr Audio Expansion Board.
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, BA_EXPAND_NUM_ENC, BA_EXPAND_NUM_LED);

elapsedMillis timer;

unsigned waveformIndex = 0; // variable for storing which analog filter we're currently using.
constexpr unsigned MAX_HEADPHONE_VOL = 10;
unsigned headphoneVolume = 8; // control headphone volume from 0 to 10.

// BAPhysicalControls returns a handle when you register a new control. We'll uses these handles when working with the controls.
int bypassHandle, waveformHandle, rateHandle, depthHandle, volumeHandle, led1Handle, led2Handle; // Handles for the various controls

void setup() {
  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);
  
  delay(100); // wait a bit for serial to be available
  Serial.begin(57600); // Start the serial port
  delay(100);

  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  // pushbuttons
  bypassHandle = controls.addSwitch(BA_EXPAND_SW1_PIN); // will be used for bypass control
  waveformHandle = controls.addSwitch(BA_EXPAND_SW2_PIN); // will be used for stepping through filters
  // pots
  rateHandle   = controls.addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection); // control the amount of delay
  depthHandle  = controls.addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  volumeHandle = controls.addPot(BA_EXPAND_POT3_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  // leds
  led1Handle = controls.addOutput(BA_EXPAND_LED1_PIN);
  led2Handle = controls.addOutput(BA_EXPAND_LED2_PIN); // will illuminate when pressing SW2

  // Disable the audio codec first
  codec.disable();
  AudioMemory(128);

  // Enable and configure the codec
  Serial.println("Enabling codec...\n");
  codec.enable();
  codec.setHeadphoneVolume(1.0f); // Max headphone volume
  
  // Besure to enable the tremolo. When disabled, audio is is completely blocked by the effect
  // to minimize resource usage to nearly to nearly zero.
  tremolo.enable(); 

  // Set some default values.
  // These can be changed using the controls on the Blackaddr Audio Expansion Board
  tremolo.bypass(false);
  controls.setOutput(led1Handle, !tremolo.isBypass()); // Set the LED when NOT bypassed  
  tremolo.rate(0.0f);
  tremolo.depth(1.0f);

  //////////////////////////////////
  // Waveform selection //
  // These are commented out, in this example we'll use SW2 to cycle through the different filters
  //tremolo.setWaveform(Waveform::SINE); // The default waveform

#if defined(USE_CAB_FILTER)
  // Guitar cabinet: Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
#endif
}

void loop() {

  float potValue;

  // Check if SW1 has been toggled (pushed)
  if (controls.isSwitchToggled(bypassHandle)) {
    bool bypass = tremolo.isBypass(); // get the current state
    bypass = !bypass; // change it
    tremolo.bypass(bypass); // set the new state
    controls.setOutput(led1Handle, !bypass); // Set the LED when NOT bypassed  
    Serial.println(String("BYPASS is ") + bypass);
  }

  // Use SW2 to cycle through the waveforms
  controls.setOutput(led2Handle, controls.getSwitchValue(led2Handle));
  if (controls.isSwitchToggled(waveformHandle)) {
    waveformIndex = (waveformIndex + 1) % static_cast<unsigned>(Waveform::NUM_WAVEFORMS);
    // cast the index
    tremolo.setWaveform(static_cast<Waveform>(waveformIndex));
    Serial.println(String("Waveform set to ") + waveformIndex);
  }

  // Use POT1 (left) to control the rate setting
  if (controls.checkPotValue(rateHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New RATE setting: ") + potValue);
    tremolo.rate(potValue);
  }

  // Use POT2 (right) to control the depth setting
  if (controls.checkPotValue(depthHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New DEPTH setting: ") + potValue);
    tremolo.depth(potValue);
  }

  // Use POT3 (centre) to control the volume setting
  if (controls.checkPotValue(volumeHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New VOLUME setting: ") + potValue);
    tremolo.volume(potValue);
  }

  // Use the 'u' and 'd' keys to adjust volume across ten levels.
  if (Serial) {
    if (Serial.available() > 0) {
      while (Serial.available()) {
        char key = Serial.read();
        if (key == 'u') { 
          headphoneVolume = (headphoneVolume + 1) % MAX_HEADPHONE_VOL;
          Serial.println(String("Increasing HEADPHONE volume to ") + headphoneVolume);
        }
        else if (key == 'd') { 
          headphoneVolume = (headphoneVolume - 1) % MAX_HEADPHONE_VOL;
          Serial.println(String("Decreasing HEADPHONE volume to ") + headphoneVolume);
        }
        codec.setHeadphoneVolume(static_cast<float>(headphoneVolume) / static_cast<float>(MAX_HEADPHONE_VOL));
      }
    }
  }

  delay(20); // Without some minimal delay here it will be difficult for the pots/switch changes to be detected.
  
  if (timer > 1000) {
    timer = 0;
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" tremolo: "); Serial.print(tremolo.processorUsage());
    Serial.println("%");
  }

}
