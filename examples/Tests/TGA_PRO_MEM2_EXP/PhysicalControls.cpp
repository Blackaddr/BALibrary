#include "BALibrary.h"
using namespace BALibrary;

constexpr int  potCalibMin = 8;
constexpr int  potCalibMax = 1016;
constexpr bool potSwapDirection = true;
int pot1Handle, pot2Handle, pot3Handle, sw1Handle, sw2Handle, led1Handle, led2Handle;
bool mute = false;
BAAudioControlWM8731 *codecPtr = nullptr;
BAPhysicalControls *controlPtr = nullptr;

void configPhysicalControls(BAPhysicalControls &controls, BAAudioControlWM8731 &codec)
{
  // Setup the controls. The return value is the handle to use when checking for control changes, etc.
  
  // pushbuttons
  sw1Handle = controls.addSwitch(BA_EXPAND_SW1_PIN);
  sw2Handle = controls.addSwitch(BA_EXPAND_SW2_PIN);
  // pots
  pot1Handle = controls.addPot(BA_EXPAND_POT1_PIN, potCalibMin, potCalibMax, potSwapDirection);
  pot2Handle = controls.addPot(BA_EXPAND_POT2_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  pot3Handle = controls.addPot(BA_EXPAND_POT3_PIN, potCalibMin, potCalibMax, potSwapDirection); 
  // leds
  led1Handle = controls.addOutput(BA_EXPAND_LED1_PIN);
  led2Handle = controls.addOutput(BA_EXPAND_LED2_PIN); // will illuminate when pressing SW2

  controlPtr = &controls;
  codecPtr = &codec;
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
    default :
      handle = pot1Handle;
  }
  
  if (controlPtr->checkPotValue(handle, potValue)) {
    // Pot has changed
    codecPtr->setHeadphoneVolume(potValue);
    Serial.println(String("POT") + id + String(" value: ") + potValue);
  } 
}

void checkSwitch(unsigned id)
{
  unsigned swHandle;
  unsigned ledHandle;
  switch(id) {
    case 0 :
      swHandle = sw1Handle;
      ledHandle = led1Handle;
      break;
    case 1 :
      swHandle = sw2Handle;
      ledHandle = led2Handle;
      break;
    default :
      swHandle = sw1Handle;
      ledHandle = led1Handle;
  }

  if (controlPtr->isSwitchToggled(swHandle)) {
    Serial.println(String("Button ") + id + String(" pressed"));
  }

  bool pressed = controlPtr->isSwitchHeld(swHandle);
  controlPtr->setOutput(ledHandle, pressed);
}
