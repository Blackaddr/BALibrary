/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This example demonstrates teh BAAudioEffectsAnalogDelay effect. It can
 * be controlled using the Blackaddr Audio "Expansion Control Board".
 * 
 * POT1 (left) controls amount of delay
 * POT2 (right) controls amount of feedback
 * POT3 (center) controls the wet/dry mix
 * SW1 will enable/bypass the audio effect. LED1 will be on when effect is enabled.
 * SW2 will cycle through the 3 pre-programmed analog filters. LED2 will be on when SW2 is pressed.
 * 
 * !!! SET POTS TO REASONABLE VALUES BEFORE STARTING TO AVOID SCREECHING FEEDBACK!!!!
 * - set POT1 (delay) fully counter-clockwise then increase it slowly.
 * - set POT2 (feedback) fully counter-clockwise, then increase it slowly
 * - set POT3 (wet/dry mix) to half-way at the detent.
 * 
 * Using the Serial Montitor, send 'u' and 'd' characters to increase or decrease
 * the headphone volume between values of 0 and 9.
 */
#include "BALibrary.h"
#include "BAEffects.h"

using namespace BAEffects;
using namespace BALibrary;

//#define USE_CAB_FILTER // uncomment this line to add a simple low-pass filter to simulate a cabinet if you are going straight to headphones
//#define USE_EXT // uncomment this line to use External MEM0

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

#ifdef USE_EXT
// If using external SPI memory, we will instantiate a SRAM
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
AudioEffectAnalogDelay analogDelay(200.0f); // set the max delay of 200 ms.
// If you use external SPI memory you can get up to 1485.0f ms of delay!
#endif

#if defined(USE_CAB_FILTER)
AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.
#endif

AudioConnection input(i2sIn,0, analogDelay,0);
#if defined(USE_CAB_FILTER)
AudioConnection delayOut(analogDelay, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);
#else
AudioConnection leftOut(analogDelay,0, i2sOut, 0);
AudioConnection rightOut(analogDelay,0, i2sOut, 1);
#endif

//////////////////////////////////////////
// SETUP PHYSICAL CONTROLS
// - POT1 (left) will control the amount of delay
// - POT2 (right) will control the amount of feedback
// - POT3 (centre) will control the wet/dry mix.
// - SW1  (left) will be used as a bypass control
// - LED1 (left) will be illuminated when the effect is ON (not bypass)
// - SW2  (right) will be used to cycle through the three built in analog filter styles available.
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
unsigned filterIndex = 0; // variable for storing which analog filter we're currently using.
constexpr unsigned MAX_HEADPHONE_VOL = 10;
unsigned headphoneVolume = 8; // control headphone volume from 0 to 10.

// BAPhysicalControls returns a handle when you register a new control. We'll uses these handles when working with the controls.
int bypassHandle, filterHandle, delayHandle, feedbackHandle, mixHandle, led1Handle, led2Handle; // Handles for the various controls

void setup() {
  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);
  
  #ifdef USE_EXT
    SPI_MEM0_4M();
  //SPI_MEM0_1M(); // use this line instead of you have the older 1Mbit memory
  #endif
  
  delay(100); // wait a bit for serial to be available
  Serial.begin(57600); // Start the serial port
  delay(100);

  // Configure the hardware
  
  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  // pushbuttons
  bypassHandle = controls.addSwitch(BA_EXPAND_SW1_PIN); // will be used for bypass control
  filterHandle = controls.addSwitch(BA_EXPAND_SW2_PIN); // will be used for stepping through filters
  // pots
  delayHandle    = controls.addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection); // control the amount of delay
  feedbackHandle = controls.addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  mixHandle      = controls.addPot(BA_EXPAND_POT3_PIN, potCalibMin, potCalibMax, potSwapDirection); 
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

  // If using external memory request request memory from the manager
  // for the slot
  #ifdef USE_EXT
  Serial.println("Using EXTERNAL memory");
  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, 500.0f, MemSelect::MEM0, true);
  #else
  Serial.println("Using INTERNAL memory");
  #endif

  // Besure to enable the delay. When disabled, audio is is completely blocked by the effect
  // to minimize resource usage to nearly to nearly zero.
  analogDelay.enable(); 

  // Set some default values.
  // These can be changed using the controls on the Blackaddr Audio Expansion Board
  analogDelay.bypass(false);
  controls.setOutput(led1Handle, !analogDelay.isBypass()); // Set the LED when NOT bypassed  
  analogDelay.mix(0.5f);
  analogDelay.feedback(0.0f);

  //////////////////////////////////
  // AnalogDelay filter selection //
  // These are commented out, in this example we'll use SW2 to cycle through the different filters
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DM3); // The default filter. Naturally bright echo (highs stay, lows fade away)
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::WARM); // A warm filter with a smooth frequency rolloff above 2Khz
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DARK); // A very dark filter, with a sharp rolloff above 1Khz

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
    bool bypass = analogDelay.isBypass(); // get the current state
    bypass = !bypass; // change it
    analogDelay.bypass(bypass); // set the new state
    controls.setOutput(led1Handle, !bypass); // Set the LED when NOT bypassed  
    Serial.println(String("BYPASS is ") + bypass);
  }

  // Use SW2 to cycle through the filters
  controls.setOutput(led2Handle, controls.getSwitchValue(led2Handle));
  if (controls.isSwitchToggled(filterHandle)) {
    filterIndex = (filterIndex + 1) % 3; // update and potentionall roll the counter 0, 1, 2, 0, 1, 2, ...
    // cast the index between 0 to 2 to the enum class AudioEffectAnalogDelay::Filter
    analogDelay.setFilter(static_cast<AudioEffectAnalogDelay::Filter>(filterIndex)); // will cycle through 0 to 2
    Serial.println(String("Filter set to ") + filterIndex);
  }

  // Use POT1 (left) to control the delay setting
  if (controls.checkPotValue(delayHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New DELAY setting: ") + potValue);
    analogDelay.delayFractionMax(potValue);
  }

  // Use POT2 (right) to control the feedback setting
  if (controls.checkPotValue(feedbackHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New FEEDBACK setting: ") + potValue);
    analogDelay.feedback(potValue);
  }

  // Use POT3 (centre) to control the mix setting
  if (controls.checkPotValue(mixHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New MIX setting: ") + potValue);
    analogDelay.mix(potValue);
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
    Serial.print(" analogDelay: "); Serial.print(analogDelay.processorUsage());
    Serial.println("%");
  }

}
