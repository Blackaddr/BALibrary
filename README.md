## BAGuitar Library
This library is under active development as of 2018.

**REQUIREMENTS**
This library uses a forked (modified) version of [DmaSpi](https://github.com/Blackaddr/DmaSpi) for Teensy, a library that extends SPI functionality to support DMA. This allows audio programs to use external SPI RAM with very little processor overhead.

Last tested with:
Arduino IDE: v1.8.4
Teensyduino: v1.39

**INTRODUCTION**
This open-source library is designed to extend the capabilities of Teensyduino, a collection of Arudino libraries  ported to the Teensy microcontroller platform by Paul at PJRC.com.

Teensyduino provides a realtime Audio library that makes it very easy for musicians and audio enthusiasts to start experimenting with and writing their own audio processors and effects, without the need for a lot of programming experience. This is accomplished by handling the complex topics (like using DMA to move audio blocks around) so you can focus on how you want to process the audio content itself.

BAGuitar adds to this by providing features and building blocks that are of particular interest to guitarists. In a nutshell, guitarists want digital audio to work similar to their guitar pedals and amps. Basically, a chain of self-contained audio processors with switches and knobs to control them. The good news is we can do this digitally with virtual patch cables (called AudioConnections) and MIDI control.

**INSTALLATION**
In order to use BAGuitar, you should:

 1. Install the Arduino IDE. This is where you write and compile your software, called 'sketches'. See [here](https://www.arduino.cc/en/Main/Software).
 2. Install the Teensyduino plugin for the Arduino IDE. This provides support for programming Teensy boards over USB, as well as access to the plethora of helpful libraries and examples it provides. See [here](https://www.pjrc.com/teensy/td_download.html).
 3. Download the BAGuitar library, and use the Library Manager in the Arduino IDE to install it. See [here](https://www.arduino.cc/en/Guide/Libraries) for details..
 4. Include "BAGuitar.h" in your sketch.

**HARDWARE**
The audio primitives and effects provided in the BAGuitar library require no special hardware other than a Teensy 3.x series board. However, in order to use the external RAM features provided in some effects, the SPI pins used must be the same as those used on the Blackaddr [TGA-Pro audio shield](http://blackaddr.com/products/).

**BAGUITAR CONTENTS**
 - WM871 advanced codec control
 - analog delay modelling effect
 - digital delay effect
 - external SRAM manager
 - more on the way!

