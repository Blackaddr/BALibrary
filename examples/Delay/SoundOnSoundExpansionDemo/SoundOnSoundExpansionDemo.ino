/*************************************************************************
 * This demo uses the BALibrary library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * THIS DEMO REQUIRES BOTH THE EXTERNAL SRAM AND EXPANSION BOARD ADD-ONS
 * 
 * This demo combines the Blackaddr Audio Expansion board with the BAAudioEffectSOS,
 * which provides sound-on-sound. The pushbuttons control the opening of the effect 
 * gate, as well as clearing the sound being held.
 * 
 * The pots control the feedback, as well as the gate opening and close times.
 * 
 * Afters startup, the effect will spend about 5 seconds clearing the audio delay buffer to prevent
 * any startup pops or clicks from propagating.
 * 
 * POT1 - Gate open time. Middle position (detent) is about 2100 ms.
 * POT2 - gate close time. Middle position (detent) is about 2100 ms.
 * POT3 - Effect volume. Controls the volume of the SOS effect separate from the normal volume
 * SW1 - Strum and hold a chord then push this button. Continue holding the button until the LED1 light goes out.
 * SW2 - Push this button to clear out the sound circulating in the delay.
 * 
 */
#include <Audio.h> 
#include "BALibrary.h"
#include "BAEffects.h"

using namespace BAEffects;
using namespace BALibrary;

//#define USE_CAB_FILTER // uncomment this line to add a simple low-pass filter to simulate a cabinet if you are going straight to headphones

AudioInputI2S i2sIn;
AudioOutputI2S i2sOut;
BAAudioControlWM8731 codec;

// External SRAM is required for this effect due to the very long
// delays required.
ExternalSramManager externalSram;
ExtMemSlot delaySlot; // Declare an external memory slot.

AudioEffectSOS sos(&delaySlot);

// Add some effects for our soloing channel
AudioEffectDelay         delayModule; // we'll add a little slapback echo
AudioMixer4              gainModule; // This will be used simply to reduce the gain before the reverb
AudioEffectReverb        reverb; // Add a bit of 'verb to our tone
AudioMixer4 mixer;

#if defined(USE_CAB_FILTER)
AudioFilterBiquad cabFilter; // We'll want something to cut out the highs and smooth the tone, just like a guitar cab.
#endif

// Connect the input
AudioConnection inputToSos(i2sIn, 0, sos, 0);
AudioConnection inputToSolo(i2sIn, 0, delayModule, 0);

// Patch cables for the SOLO channel
AudioConnection inputToGain(delayModule, 0, gainModule, 0);
AudioConnection inputToReverb(gainModule, 0, reverb, 0);

// Output Mixer
AudioConnection mixer0input(i2sIn, 0, mixer, 0);  // SOLO Dry Channel
AudioConnection mixer1input(reverb, 0, mixer, 1); // SOLO Wet Channel
AudioConnection mixer2input(sos, 0, mixer, 2); // SOS Channel

#if defined(USE_CAB_FILTER)
AudioConnection inputToCab(mixer, 0, cabFilter, 0);
AudioConnection outputLeft(cabFilter, 0, i2sOut, 0);
AudioConnection outputRight(cabFilter, 0, i2sOut, 1);
#else
AudioConnection outputLeft(mixer, 0, i2sOut, 0);
AudioConnection outputRight(mixer, 0, i2sOut, 1);
#endif

//////////////////////////////////////////
// SETUP PHYSICAL CONTROLS
// - POT1 (left) will control the GATE OPEN time
// - POT2 (right) will control the GATE CLOSE TIME
// - POT3 (centre) will control the EFFECT VOLUME
// - SW1  (left) will be used as the GATE TRIGGER
// - LED1 (left) will be illuminated while the GATE is open
// - SW2  (right) will be used as the CLEAR FEEDBACK TRIGGER
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
constexpr unsigned MAX_HEADPHONE_VOL = 10;
unsigned headphoneVolume = MAX_HEADPHONE_VOL; // control headphone volume from 0 to 10.
constexpr float MAX_GATE_TIME_MS = 4000.0f; // set maximum gate time of 4 seconds.

// BAPhysicalControls returns a handle when you register a new control. We'll uses these handles when working with the controls.
int gateHandle, clearHandle, openHandle, closeHandle, volumeHandle, led1Handle, led2Handle; // Handles for the various controls


void setup() {

  TGA_PRO_MKII_REV1(); // Declare the version of the TGA Pro you are using.
  //TGA_PRO_REVB(x);
  //TGA_PRO_REVA(x);

  SPI_MEM0_4M();
  //SPI_MEM0_1M(); // use this line instead of you have the older 1Mbit memory

  delay(100);
  delay(100); // wait a bit for serial to be available
  Serial.begin(57600); // Start the serial port
  delay(100); // wait a bit for serial to be available

  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  // pushbuttons
  gateHandle  = controls.addSwitch(BA_EXPAND_SW1_PIN); // will be used for bypass control
  clearHandle = controls.addSwitch(BA_EXPAND_SW2_PIN); // will be used for stepping through filters
  // pots
  openHandle   = controls.addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection); // control the amount of delay
  closeHandle  = controls.addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  volumeHandle = controls.addPot(BA_EXPAND_POT3_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  // leds
  led1Handle = controls.addOutput(BA_EXPAND_LED1_PIN);
  led2Handle = controls.addOutput(BA_EXPAND_LED2_PIN); // will illuminate when pressing SW2

  // Disable the audio codec first
  codec.disable();
  AudioMemory(128);

  // Enable the codec
  Serial.println("Enabling codec...\n");
  codec.enable();
  codec.setHeadphoneVolume(1.0f); // Max headphone volume

  // We have to request memory be allocated to our slot.
  externalSram.requestMemory(&delaySlot, BAHardwareConfig.getSpiMemSizeBytes(MemSelect::MEM0), MemSelect::MEM0, true);

  // Configure the LED to indicate the gate status, this is controlled directly by SOS effect, not by
  // by BAPhysicalControls
  sos.setGateLedGpio(BA_EXPAND_LED1_PIN);

  // Besure to enable the SOS. When disabled, audio is is completely blocked
  // to minimize resources to nearly zero.
  sos.enable(); 

  // Set some default values.
  // These can be changed by sending MIDI CC messages over the USB using
  // the BAMidiTester application.
  sos.bypass(false);
  sos.gateOpenTime(3000.0f);
  sos.gateCloseTime(1000.0f);
  sos.feedback(0.9f);

  // Setup effects on the SOLO channel
  gainModule.gain(0, 0.25); // the reverb unit clips easily if the input is too high
  delayModule.delay(0, 50.0f); // 50 ms slapback delay

#if defined(USE_CAB_FILTER)
  // Setup 2-stages of LPF, cutoff 4500 Hz, Q-factor 0.7071 (a 'normal' Q-factor)
  cabFilter.setLowpass(0, 4500, .7071);
  cabFilter.setLowpass(1, 4500, .7071);
#endif

  // Setup the Mixer
  mixer.gain(0, 0.5f); // SOLO Dry gain
  mixer.gain(1, 0.5f); // SOLO Wet gain
  mixer.gain(1, 1.0f); // SOS gain

  delay(1000);
  sos.clear();
  
}

void loop() {

  float potValue;

  // Check if SW1 has been toggled (pushed) and trigger the gate
  // LED1 will be directly control by the SOS effect, not by BAPhysicalControls
  if (controls.isSwitchToggled(gateHandle)) {
    sos.trigger(); 
    Serial.println("GATE OPEN is triggered");
  }

  // Use SW2 to clear out the SOS delayline
  controls.setOutput(led2Handle, controls.getSwitchValue(led2Handle));
  if (controls.isSwitchToggled(clearHandle)) {
    sos.clear();
    Serial.println("GATE CLEAR is triggered");
  }

  // Use POT1 (left) to control the OPEN GATE time
  if (controls.checkPotValue(openHandle, potValue)) {
    // Pot has changed
    sos.gateOpenTime(potValue * MAX_GATE_TIME_MS);
    Serial.println(String("New OPEN GATE setting (ms): ") + (potValue * MAX_GATE_TIME_MS));
  }

  // Use POT2 (right) to control the CLOSE GATE time
  if (controls.checkPotValue(closeHandle, potValue)) {
    // Pot has changed
    sos.gateCloseTime(potValue * MAX_GATE_TIME_MS);
    Serial.println(String("New CLOSE GATE setting (ms): ") + (potValue * MAX_GATE_TIME_MS));
  }

  // Use POT3 (centre) to control the sos effect volume
  if (controls.checkPotValue(volumeHandle, potValue)) {
    // Pot has changed
    Serial.println(String("New SOS VOLUME setting: ") + potValue);
    sos.volume(potValue);
  }

  // Use the 'u' and 'd' keys to adjust headphone volume across ten levels.
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
    Serial.print(" SOS: "); Serial.print(sos.processorUsage());
    Serial.println("%");
  }
}
