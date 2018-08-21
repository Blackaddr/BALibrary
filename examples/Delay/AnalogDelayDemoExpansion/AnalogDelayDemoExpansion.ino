#include <MIDI.h>

#define TGA_PRO_REVB
#define TGA_PRO_EXPAND_REV2

#include "BAGuitar.h"

using namespace midi;
using namespace BAEffects;
using namespace BALibrary;

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

//#define USE_EXT // uncomment this line to use External MEM0

#ifdef USE_EXT
// If using external SPI memory, we will instantiance an SRAM
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
AudioEffectAnalogDelay analogDelay(200.0f); // max delay of 200 ms.
#endif

AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.

// Simply connect the input to the delay, and the output
// to both i2s channels
AudioConnection input(i2sIn,0, analogDelay,0);
AudioConnection delayOut(analogDelay, 0, cabFilter, 0);
AudioConnection leftOut(cabFilter,0, i2sOut, 0);
AudioConnection rightOut(cabFilter,0, i2sOut, 1);

//////////////////////////////////////////
// SETUP PHYSICAL CONTROLS
//////////////////////////////////////////
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, 0);

int loopCount = 0;

void setup() {
  delay(100);
  Serial.begin(57600); // Start the serial port
  delay(100);

  // Setup the controls
  controls.addSwitch(BA_EXPAND_SW1_PIN);
  controls.addSwitch(BA_EXPAND_SW2_PIN);
  controls.addPot(BA_EXPAND_POT1_PIN, 

  // Disable the codec first
  codec.disable();
  AudioMemory(128);

  // Enable the codec
  Serial.println("Enabling codec...\n");
  codec.enable();

  // If using external memory request request memory from the manager
  // for the slot
  #ifdef USE_EXT
  Serial.println("Using EXTERNAL memory");
  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, 500.0f, MemSelect::MEM0, true);
  #else
  Serial.println("Using INTERNAL memory");
  #endif

  // Besure to enable the delay. When disabled, audio is is completely blocked
  // to minimize resources to nearly zero.
  analogDelay.enable(); 

  // Set some default values.
  // These can be changed using the controls on the Blackaddr Audio Expansion Board
  analogDelay.bypass(false);
  analogDelay.mix(0.5f);
  analogDelay.feedback(0.0f);

  //////////////////////////////////
  // AnalogDelay filter selection //
  // Uncomment to tryout the 3 different built-in filters.
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DM3); // The default filter. Naturally bright echo (highs stay, lows fade away)
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::WARM); // A warm filter with a smooth frequency rolloff above 2Khz
  //analogDelay.setFilter(AudioEffectAnalogDelay::Filter::DARK); // A very dark filter, with a sharp rolloff above 1Khz

  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
}

void loop() {

  if (loopCount % 524288 == 0) {
    Serial.print("Processor Usage, Total: "); Serial.print(AudioProcessorUsage());
    Serial.print("% ");
    Serial.print(" analogDelay: "); Serial.print(analogDelay.processorUsage());
    Serial.println("%");
  }
  loopCount++;


}
