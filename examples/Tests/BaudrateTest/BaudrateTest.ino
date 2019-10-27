
/*************************************************************************
 * This demo does high speed testing of the Serial1 hardware port. On
 * The TGA Pro, this is used for MIDI. MIDI is essentially a Serial protocol
 * running 31250 baud with port configuration 8N1.
 * 
 * Despite there being MIDI physical circuitry on the TGA Pro, we can still
 * run a serial protocol at different rates to ensure a robust design.
 * 
 * CONNECT A MIDI CABLE FROM MIDI_INPUT TO MIDI_OUTPUT AS A LOOPBACK.
 */
constexpr unsigned LOW_RATE = 2400;
constexpr unsigned MIDI_RATE = 31250;
constexpr unsigned HIGH_RATE = 230400; 
constexpr unsigned TEST_TIME = 5; // 5 second test each

unsigned baudRate = LOW_RATE; // start with low speed
uint8_t writeData = 0;
unsigned loopCounter = 0;
unsigned errorCount = 0;
bool testFailed = false;
bool testDone = false;
unsigned testPhase = 0; // 0 for low speed, 1 for MIDI speed, 2 for high speed.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  Serial1.begin(baudRate, SERIAL_8N1);
  delay(100);
  while(!Serial) {}
  Serial.println(String("\nRunning speed test at ") + baudRate);

  // write the first data
  Serial1.write(writeData);
}


void loop() {

  if ((!testFailed) && (!testDone)) {

    if (loopCounter >= (baudRate/4)) { // the divisor determines how long the test runs for
      // next test
      switch (testPhase) {
        case 0 :
          baudRate = MIDI_RATE;
          break;
        case 1 :
          baudRate = HIGH_RATE;
          break;
        case 2 :
          testDone = true;
      }

      if (errorCount == 0) { Serial.println("TEST PASSED!"); }
      else { 
        Serial.println("TEST FAILED!");
      }
      errorCount = 0;
      testPhase++;
      loopCounter = 0;
      
      if (!testDone) {
        Serial.println(String("\nRunning speed test at ") + baudRate);
        Serial1.begin(baudRate, SERIAL_8N1);
        while (!Serial1) {} // wait for serial to be ready
      } else {
        Serial.println("\nTEST DONE!");
      }
    }
    
    // Wait for read data
    if (Serial1.available()) {  
      uint8_t readData= Serial1.read();
      if (readData != writeData) {
        Serial.println(String("ERROR: readData = ") + readData + String(" writeData = ") + writeData);
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
}
