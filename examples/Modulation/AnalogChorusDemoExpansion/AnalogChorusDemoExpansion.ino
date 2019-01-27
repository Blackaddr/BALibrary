/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This example demonstrates teh BAAudioEffectsAnalogChorus effect. It can
 * be controlled using the Blackaddr Audio "Expansion Control Board".
 * 
 * POT1 (left) controls the modulation rate
 * POT2 (right) controls the modulation depth
 * POT3 (center) controls the wet/dry mix
 * SW1 will enable/bypass the audio effect. LED1 will be on when effect is enabled.
 * SW2 will cycle through the 3 pre-programmed analog filters. LED2 will be on when SW2 is pressed.
 * 
 * Using the Serial Montitor, send 'u' and 'd' characters to increase or decrease
 * the headphone volume between values of 0 and 9.
 */
#define TGA_PRO_REVB // Set which hardware revision of the TGA Pro we're using
#define TGA_PRO_EXPAND_REV2 // pull in the pin definitions for the Blackaddr Audio Expansion Board.

#include "BALibrary.h"
#include "BAEffects.h"

using namespace BAEffects;
using namespace BALibrary;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

//#define USE_EXT // uncomment this line to use External MEM0

#ifdef USE_EXT
// If using external SPI memory, we will instantiate a SRAM
// manager and create an external memory slot to use as the memory
// for our audio delay
ExternalSramManager externalSram;
ExtMemSlot delaySlot; // Declare an external memory slot.

// Instantiate the AudioEffectAnalogChorus to use external memory by
/// passing it the delay slot.
AudioEffectAnalogChorus analogChorus(&delaySlot);
#else
AudioEffectAnalogChorus analogChorus; // default chorus delays
#endif

AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.

// Simply connect the input to the delay, and the output
// to both i2s channels
AudioConnection input(i2sIn,0, analogChorus,0);
AudioConnection chorusOut(analogChorus, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);


//////////////////////////////////////////
// SETUP PHYSICAL CONTROLS
// - POT1 (left) will control the rate
// - POT2 (right) will control the depth
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

int loopCount = 0;
unsigned filterIndex = 0; // variable for storing which analog filter we're currently using.
constexpr unsigned MAX_HEADPHONE_VOL = 10;
unsigned headphoneVolume = 8; // control headphone volume from 0 to 10.

// BAPhysicalControls returns a handle when you register a new control. We'll uses these handles when working with the controls.
int bypassHandle, filterHandle, rateHandle, depthHandle, mixHandle, led1Handle, led2Handle; // Handles for the various controls

void setup() {
  delay(100); // wait a bit for serial to be available
  Serial.begin(57600); // Start the serial port
  delay(100);

  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  // pushbuttons
  bypassHandle = controls.addSwitch(BA_EXPAND_SW1_PIN); // will be used for bypass control
  filterHandle = controls.addSwitch(BA_EXPAND_SW2_PIN); // will be used for stepping through filters
  // pots
  rateHandle    = controls.addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection); // control the amount of delay
  depthHandle = controls.addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
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
  externalSram.requestMemory(&delaySlot, 40.0f, MemSelect::MEM0, true); // 40 ms is enough to handle the full range of the chorus delay
  #else
  Serial.println("Using INTERNAL memory");
  #endif

  // Besure to enable the delay. When disabled, audio is is completely blocked by the effect
  // to minimize resource usage to nearly to nearly zero.
  analogChorus.enable(); 

  // Set some default values.
  // These can be changed using the controls on the Blackaddr Audio Expansion Board
  analogChorus.bypass(false);
  analogChorus.rate(0.5f);
  analogChorus.mix(0.5f);
  analogChorus.depth(1.0f);

  //////////////////////////////////
  // AnalogChorus filter selection //
  // These are commented out, in this example we'll use SW2 to cycle through the different filters
  //analogChorus.setFilter(AudioEffectAnalogChorus::Filter::CE2); // The default filter. Naturally bright echo (highs stay, lows fade away)
  //analogChorus.setFilter(AudioEffectAnalogChorus::Filter::WARM); // A warm filter with a smooth frequency rolloff above 2Khz
  //analogChorus.setFilter(AudioEffectAnalogChorus::Filter::DARK); // A very dark filter, with a sharp rolloff above 1Khz

  // Guitar cabinet: Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
}

void loop() {

  float potValue;

  // Check if SW1 has been toggled (pushed)
  if (controls.isSwitchToggled(bypassHandle)) {
    bool bypass = analogChorus.isBypass(); // get the current state
    bypass = !bypass; // change it
    analogChorus.bypass(bypass); // set the new state
    controls.setOutput(led1Handle, !bypass); // Set the LED when NOT bypassed  
    Serial.println(String("BYPASS is ") + bypass);
  }

  // Use SW2 to cycle through the filters
  controls.setOutput(led2Handle, controls.getSwitchValue(led2Handle));
  if (controls.isSwitchToggled(filterHandle)) {
    filterIndex = (filterIndex + 1) % 3; // update and potentionall roll the counter 0, 1, 2, 0, 1, 2, ...
    // cast the index between 0 to 2 to the enum class AudioEffectAnalogChorus::Filter
    analogChorus.setFilter(static_cast<AudioEffectAnalogChorus::Filter>(filterIndex)); // will cycle through 0 to 2
    Serial.println(String("Filter set to ") + filterIndex);
  }

  // Use POT1 (left) to control the rate setting
  if (controls.checkPotValue(rateHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New RATE setting: ") + potValue);
    analogChorus.rate(potValue);
  }

  // Use POT2 (right) to control the depth setting
  if (controls.checkPotValue(depthHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New DEPTH setting: ") + potValue);
    analogChorus.depth(potValue);
  }

  // Use POT3 (centre) to control the mix setting
  if (controls.checkPotValue(mixHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New MIX setting: ") + potValue);
    analogChorus.mix(potValue);
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

  // Use the loopCounter to roughly measure human timescales. Every few seconds, print the CPU usage
  // to the serial port. About 500,000 loops!
  if (loopCount % 524288 == 0) {
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" AnalogChorus: "); Serial.print(analogChorus.processorUsage());
    Serial.println("%");
  }
  loopCount++;

}
