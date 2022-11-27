#include <cmath>
#include "Adafruit_SH1106.h"
#include "BALibrary.h"
#include "DebugPrintf.h"

using namespace BALibrary;

// Declare the externally shared variables from the main .ino
extern Adafruit_SH1106            display;
extern BAAudioControlWM8731master codec;
extern AudioMixer4                volumeOut;
extern elapsedMillis              timer;

constexpr int  displayRow = 36;  // Row to start OLED display updates on
constexpr int  potCalibMin = 8;
constexpr int  potCalibMax = 1016;
constexpr bool potSwapDirection = true;
constexpr bool encSwapDirection = true;
int pot1Handle= -1, pot2Handle = -1, pot3Handle = -1, pot4Handle = -1;
int sw1Handle = -1, sw2Handle = -1, sw3Handle = -1, sw4Handle = -1, sw5Handle = -1, sw6Handle = -1;
int enc1Handle = -1, enc2Handle = -1, enc3Handle = -1, enc4Handle = -1;
int led1Handle = -1, led2Handle = -1;

BAAudioControlWM8731master *codecPtr = nullptr;
BAPhysicalControls *controlPtr       = nullptr;

// Configure and setup the physical controls
void configPhysicalControls(BAPhysicalControls* controls, BAAudioControlWM8731master* codec)
{
  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  controlPtr = controls;
  codecPtr   = codec;

  if (!controlPtr) { DEBUG_PRINT(Serial.printf("ERROR: controlPtr is invalid\n\r")); return; }
  if (!codecPtr)   { DEBUG_PRINT(Serial.printf("ERROR: codecPtr is invalid\n\r")); return; }
  
  // pushbuttons
  sw1Handle = controlPtr->addSwitch(BA_EXPAND_SW1_PIN);
  sw2Handle = controlPtr->addSwitch(BA_EXPAND_SW2_PIN);
  sw3Handle = controlPtr->addSwitch(BA_EXPAND_SW3_PIN);
  sw4Handle = controlPtr->addSwitch(BA_EXPAND_SW4_PIN);
  sw5Handle = controlPtr->addSwitch(BA_EXPAND_SW5_PIN);
  sw6Handle = controlPtr->addSwitch(BA_EXPAND_SW6_PIN);
  // pots
  pot1Handle = controlPtr->addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection);
  pot2Handle = controlPtr->addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  pot3Handle = controlPtr->addPot(BA_EXPAND_POT3_PIN, potCalibMin, potCalibMax, potSwapDirection);
  pot4Handle = controlPtr->addPot(BA_EXPAND_POT4_PIN, potCalibMin, potCalibMax, potSwapDirection);

  // encoders
  enc1Handle = controlPtr->addRotary(BA_EXPAND_ENC1_A_PIN, BA_EXPAND_ENC1_B_PIN, encSwapDirection);
  enc2Handle = controlPtr->addRotary(BA_EXPAND_ENC2_A_PIN, BA_EXPAND_ENC2_B_PIN, encSwapDirection); 
  enc3Handle = controlPtr->addRotary(BA_EXPAND_ENC3_A_PIN, BA_EXPAND_ENC3_B_PIN, encSwapDirection);
  enc4Handle = controlPtr->addRotary(BA_EXPAND_ENC4_A_PIN, BA_EXPAND_ENC4_B_PIN, encSwapDirection);
  
  // leds
  led1Handle = controlPtr->addOutput(BA_EXPAND_LED1_PIN);
  led2Handle = controlPtr->addOutput(BA_EXPAND_LED2_PIN); // will illuminate when pressing SW2

}

void checkPot(unsigned id)
{
  float potValue;
  unsigned handle;
  switch(id) {
    case 0 :
      handle = pot1Handle;
      break;
    case 1 :
      handle = pot2Handle;
      break;
    case 2 :
      handle = pot3Handle;
      break;
    case 3 :
      handle = pot4Handle;
      break;
    default :
      handle = pot1Handle;
  }

  if ((handle < 0) || (handle >= controlPtr->getNumPots())) {
    DEBUG_PRINT(Serial.printf("ILLEGAL POT HANDLE: %d for id %d\n\r", handle, id));
    return;
  }
  
  if (controlPtr->checkPotValue(handle, potValue)) {
    // Pot has changed
    DEBUG_PRINT(Serial.println(String("POT") + id + String(" value: ") + potValue));

    timer = 0;
    display.clearDisplay();
    display.setCursor(0,displayRow);
    switch(id) {
      case 0 :
      {
        display.printf("Gain: %0.f\n", potValue * 100.0f);
        int gain = static_cast<int>(std::roundf(31.0f * potValue));
        codecPtr->setLeftInputGain(gain);
        codecPtr->setRightInputGain(gain);
        yield(); // give time for i2C transfers to complete
        break;
      }
      case 1 : 
      {
        display.printf("Level: %0.f\n", potValue * 100.0f);
        volumeOut.gain(0, potValue);
        volumeOut.gain(1, potValue);
        break;
      }
      case 2 : display.printf("Exp T: %0.f\n", potValue * 100.0f); break;
      case 3 : display.printf("Exp R: %0.f\n", potValue * 100.0f); break;
    }
    display.display();
  }

}

int checkSwitch(unsigned id, bool getValueOnly=false)
{
  unsigned swHandle  = -1;
  unsigned ledHandle = -1;
  switch(id) {
    case 0 :
      swHandle = sw1Handle;
      ledHandle = led1Handle;
      break;
    case 1 :
      swHandle = sw2Handle;
      ledHandle = led2Handle;
      break;
    case 2 :
      swHandle = sw3Handle;
      break;
    case 3 :
      swHandle = sw4Handle;
      break;
    case 4 :
      swHandle = sw5Handle;
      break;
    case 5 :
      swHandle = sw6Handle;
      break;
    default :
      swHandle = sw1Handle;
      ledHandle = led1Handle;
  }

  if ((swHandle < 0) || (swHandle >= controlPtr->getNumSwitches())) {
    DEBUG_PRINT(Serial.printf("ILLEGAL SWITCH HANDLE: %d for id %d\n\r", swHandle, id); Serial.flush());
    return -1;
  }

  bool switchValue;
  bool changed = controlPtr->hasSwitchChanged(swHandle, switchValue);
  if (getValueOnly) { return controlPtr->getSwitchValue(swHandle); }
  
  if (changed) {
    DEBUG_PRINT(Serial.println(String("Button ") + id + String(" pressed")));
    timer = 0;
    display.clearDisplay();
    display.setCursor(0, displayRow);
    switch(id) {
      case 0 : display.printf("S1: %d\n", switchValue); break;
      case 1 : display.printf("S2: %d\n", switchValue); break;
      case 2 : display.printf("EncSw A: %d\n", switchValue); break;
      case 3 : display.printf("EncSw B: %d\n", switchValue); break;
      case 4 : display.printf("EncSw C: %d\n", switchValue); break;
      case 5 : display.printf("EncSw D: %d\n", switchValue); break;
    }
    display.display();
  }

  if (swHandle < 2) { // these SWs map to LEDs
    bool pressed = controlPtr->isSwitchHeld(swHandle);
    controlPtr->setOutput(ledHandle, pressed);
  }
  return controlPtr->getSwitchValue(swHandle);
}

void checkEncoder(unsigned id)
{
  unsigned encHandle;
  static int enc1 = 0, enc2 = 0, enc3 = 0, enc4 = 0;
  switch(id) {
    case 0 :
      encHandle = enc1Handle;
      break;
    case 1 :
      encHandle = enc2Handle;
      break;
    case 2 :
      encHandle = enc3Handle;
      break;
    case 3 :
      encHandle = enc4Handle;
      break;
    default :
      encHandle = enc1Handle;
  }

  if ((encHandle < 0) || (encHandle >= controlPtr->getNumRotary())) {
    DEBUG_PRINT(Serial.printf("ILLEGAL ENCODER HANDLE: %d for id %d\n\r", encHandle, id); Serial.flush());
    return;
  }

  int adj= controlPtr->getRotaryAdjustUnit(encHandle);
  if (adj != 0) {    
    DEBUG_PRINT(Serial.printf("Enc %d: %d\n\r", id, adj); Serial.flush());    
    display.clearDisplay();
    display.setCursor(0, displayRow);
    switch(id) {
      case 0 : enc1 += adj; display.printf("Enc A: %d", enc1); break;
      case 1 : enc2 += adj; display.printf("Enc B: %d", enc2); break;
      case 2 : enc3 += adj; display.printf("Enc C: %d", enc3); break;
      case 3 : enc4 += adj; display.printf("Enc D: %d", enc4); break;
    }
    display.display();
    timer = 0;
  }
}
