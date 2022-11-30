#ifndef PHYSICAL_CONTROLS_H_
#define PHYSICAL_CONTROLS_H_

#include "BALibrary.h"

void configPhysicalControls(BALibrary::BAPhysicalControls* controls, BALibrary::BAAudioControlWM8731master* codec);
void checkPot(unsigned id);
int  checkSwitch(unsigned id, bool getValueOnly=false);
void checkEncoder(unsigned id);

#endif
