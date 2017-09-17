/*************************************************************************
 * This demo uses only the built-in libraries provided with the Arudino
 * Teensy libraries. 
 * 
 * IT IS STRONGLY RECOMMENDED to use the BAGuitar Library located at
 * https://github.com/Blackaddr/BAGuitar
 * The BAGuitar library will made it easier to use the features of your
 * GTA Pro board, as well and configure it correctly to eliminate
 * unneccesary noise.
 * 
 * The built-in support for the WM8731 codec in the Teensy Library is very
 * limited. Without proper configuration, the codec will produce a noisy
 * output. This is caused by the default configuration the WM8731 powers
 * up in. This can easily be corrected by installing the BAGuitar library
 * 
 * For instructions on installing additional libraries follow the
 * instructions at https://www.arduino.cc/en/Guide/Libraries after downloading
 * the BAGuitar repo from github as a zip file.
 * 
 */

#include <Wire.h>
#include <Audio.h>
#include <MIDI.h>

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

// Audio connections, just connect the I2S input directly to the I2S output.
AudioConnection      patch0(i2sIn,0, i2sOut, 0);
AudioConnection      patch1(i2sIn,1, i2sOut, 1);

AudioControlWM8731      codecControl; // needed to enable the codec

unsigned long t=0;
const int usrLED = 16; // the location of the GTA Pro user LED (not the LED on the Teensy itself)
int usrLEDState = 0;

void setup() {

  // Configure the user LED pin as output
  pinMode(usrLED, OUTPUT);
  digitalWrite(usrLED, usrLEDState);

  // Enable the MIDI to listen on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);  
  Serial.begin(57600); // Enable the serial monitor
  Wire.begin(); // Enable the I2C bus for controlling the codec
  delay(5);
  
  delay(100);
  codecControl.disable(); // Turn off the codec first (in case it was in an unknown state)
  delay(100);
  AudioMemory(24);

  Serial.println("Enabling codec...\n");
  codecControl.enable(); // Enable the codec
  delay(100);
  

}

void loop() {  
  
  ///////////////////////////////////////////////////////////////////////
  // MIDI TESTING
  // Connect a loopback cable between the MIDI IN and MIDI OUT on the
  // GTA Pro.  This test code will periodically send MIDI events which
  // will loop back and get printed in the Serial Monitor.
  ///////////////////////////////////////////////////////////////////////
  int type, note, velocity, channel, d1, d2;

  // Send MIDI OUT
  int cc, val=0xA, channelSend = 1;
  for (cc=32; cc<40; cc++) {
    MIDI.sendControlChange(cc, val, channelSend); val++; channelSend++;
    delay(100);
    MIDI.sendNoteOn(10, 100, channelSend);
    delay(100);
  }
  
  if (MIDI.read()) {                    // Is there a MIDI message incoming ?
    byte type = MIDI.getType();
    Serial.println(String("MIDI IS WORKING!!!"));
    switch (type) {
      case NoteOn:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        if (velocity > 0) {
          Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        } else {
          Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
        }
        break;
      case NoteOff:
        note = MIDI.getData1();
        velocity = MIDI.getData2();
        channel = MIDI.getChannel();
        Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
        break;
      default:
        d1 = MIDI.getData1();
        d2 = MIDI.getData2();
        Serial.println(String("Message, type=") + type + ", data = " + d1 + " " + d2);
    }
    t = millis();
  }

  // If no MIDI IN activity, print a message every 10 seconds
  if (millis() - t > 10000) {
    t += 10000;
    Serial.println("(no MIDI activity, check cables)");
  }

  // Toggle the USR LED state
  usrLEDState = ~usrLEDState;
  digitalWrite(usrLED, usrLEDState);

}

