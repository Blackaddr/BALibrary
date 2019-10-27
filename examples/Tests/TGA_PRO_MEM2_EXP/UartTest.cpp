#include "BAHardware.h"
#include "BASpiMemory.h"

using namespace BALibrary;

constexpr unsigned MIDI_RATE = 31250;
constexpr unsigned HIGH_RATE = 230400; 
constexpr unsigned TEST_TIME = 5; // 5 second test each

static unsigned baudRate = MIDI_RATE; // start with low speed
static uint8_t writeData = 0;
static unsigned loopCounter = 0;
static unsigned errorCount = 0;
static bool testFailed = false;
static bool testDone = false;
static unsigned testPhase = 0; // 0 for MIDI speed, 1 for high speed.

bool uartTest(void)
{
  Serial1.begin(baudRate, SERIAL_8N1);
  delay(100);
  while(!Serial) {}
  Serial.println(String("\nRunning MIDI Port speed test at ") + baudRate);

  // write the first data
  Serial1.write(writeData);

  while(!testFailed && !testDone) {
    
    if (loopCounter >= (baudRate/4)) { // the divisor determines how long the test runs for
      // next test
      switch (testPhase) {
        case 0 :
          baudRate = HIGH_RATE;
          break;
        case 1 :
          testDone = true;
      }

      if (errorCount == 0) { Serial.println("TEST PASSED!"); }
      else { 
        Serial.println("MIDI PORT TEST FAILED!");
      }
      errorCount = 0;
      testPhase++;
      loopCounter = 0;
      
      if (!testDone) {
        Serial.println(String("\nRunning MIDI Port speed test at ") + baudRate);
        Serial1.begin(baudRate, SERIAL_8N1);
        while (!Serial1) {} // wait for serial to be ready
      } else {
        Serial.println("MIDI PORT TEST DONE!\n");
      }
    }
    
    // Wait for read data
    if (Serial1.available()) {  
      uint8_t readData= Serial1.read();
      if (readData != writeData) {
        Serial.println(String("MIDI ERROR: readData = ") + readData + String(" writeData = ") + writeData);
        errorCount++;
      }
    
      if ((loopCounter % (baudRate/64)) == 0) { // the divisor determines how often the period is printed
        Serial.print("."); Serial.flush();
      }
    
      if (errorCount > 16) {
        Serial.println("Halting test");
        testFailed = true;
      }
    
      loopCounter++;
      writeData++;
      Serial1.write(writeData);
    }
  }

  return testFailed;
}
