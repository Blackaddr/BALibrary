#ifndef PHYSICAL_CONTROLS_H_
#define PHYSICAL_CONTROLS_H_

#include "BALibrary.h"

#define USE_OLED  // Comment this line if you don't want to use the OLED or it's dependencies.

void configPhysicalControls(BALibrary::BAPhysicalControls* controls, BALibrary::BAAudioControlWM8731master* codec);
void checkPot(unsigned id);
int  checkSwitch(unsigned id, bool getValueOnly=false);
void checkEncoder(unsigned id);

#endif
