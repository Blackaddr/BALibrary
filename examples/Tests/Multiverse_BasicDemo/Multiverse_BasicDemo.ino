/***********************************************************************************
 * MULTIVERSE DEMO
 * 
 * This demo program shows how to use BALibrary to access the hardware
 * features of the Aviate Audio Multiverse effects processor.
 * 
 * The following are demonstrated in this programming using BALibrary:
 * - WM8731 stereo audio codec in master mode (NOTE: not slave mode like TGA Pro)
 * - Interact with all physical controls
 * - Control the 128x64 pixel OLED display (connected to SPI0)
 * - Use the 8MB external SRAM (simple memory test)
 * 
 * REQUIREMENTS:
 * This demo for Multiverse uses its OLED display which requires several Arduino
 * libraries be downloaded first. The SH1106 library is modifed to work with Teensy
 * and must be downloaded from the AviateAudio github.
 * 
 * Adafruit_BusIO       : https://github.com/adafruit/Adafruit_BusIO
 * Adafruit_GFX_Library : https://github.com/adafruit/Adafruit-GFX-Library
 * Adafruit_SH1106      : https://github.com/AviateAudio/Adafruit_SH1106
 * 
 * 
 * USAGE INSTRUCTIONS
 * - Use the 'Gain' knob to control the input gain on the codec. See checkPot().
 * - Use the 'Level' knob to control output volume with an AudioMixer4 object.
 * - Stomp switches S1 and S2 will write status to display, and turn on LED
 * - Encoder push-button switches will write status to display when pressed/released
 * - Encoder rotary control will adjust a positive/negative count and update display
 */
#include <Audio.h>
#include <SPI.h>
#include "BALibrary.h"

#include "DebugPrintf.h"
#include "PhysicalControls.h"

using namespace BALibrary;

// OLED display stuff
#include "Adafruit_SH1106.h"
#include "Adafruit_GFX.h"
#include "Fonts/FreeSansBold9pt7b.h"
constexpr unsigned SCREEN_WIDTH = 128; // OLED display width, in pixels
constexpr unsigned SCREEN_HEIGHT = 64; // OLED display height, in pixels
Adafruit_SH1106 display(37, 35, 10);

// External SPI RAM
ExternalSramManager sramManager;
ExtMemSlot memSlot;
BASpiMemory spiMem1(SpiDeviceId::SPI_DEVICE1);
unsigned spiAddress = 0;
unsigned spiAddressMax;
unsigned sramStage = 0; // stage 0 is zero, 1 is write, 2 is read
volatile float sramCompletion = 0.0f;
volatile unsigned errorCount = 0;

AudioInputI2Sslave       i2sIn;
AudioOutputI2Sslave      i2sOut;
AudioMixer4              volumeOut;

// i2sIn --> volumeOut(Mixer) --> i2sOut
AudioConnection patchIn0(i2sIn, 0, volumeOut, 0);
AudioConnection patchIn1(i2sIn, 1, volumeOut, 1);
AudioConnection patchOut0(volumeOut,0, i2sOut, 0);
AudioConnection patchOut1(volumeOut,0, i2sOut, 1);

BAAudioControlWM8731master codec;
elapsedMillis timer;

// Create a control object using the number of switches, pots, encoders and outputs on the
// Multiverse pedal
BAPhysicalControls controls(6, 4, 4, 2);  // (SW, POT, ENC, LED)

unsigned loopCounter = 0;

void drawProgressBar(float completion);  // declaration

void drawBlackaddrAudio() {
  display.setCursor(0, 24); // (x,y)
  display.printf("    Blackaddr");
  display.setCursor(0, 40); // (x,y)
  display.printf("        Audio");
}

void setup() {

  codec.disable(); // this will reset the codec

  // wait up for the serial to appear for up to 1 second, then continue
  Serial.begin(57600);
  unsigned serialLoopCount = 10;
  while (!Serial && (serialLoopCount > 0)) {
    delay(100);
    serialLoopCount--;
  }

  MULTIVERSE_REV1(); // constants defined in BALibrary become valid only after this call
  SPI_MEM1_64M(); // Declare the correct memory size

  // Init the display
  display.begin(SH1106_SWITCHCAPVCC, SH1106_I2C_ADDRESS, true);
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE); // Draw white text
  display.setFont(&FreeSansBold9pt7b);
  drawBlackaddrAudio();
  display.display();

  configPhysicalControls(&controls, &codec);  

  // Request a memory slot from the external RAM
  size_t numBytes = BAHardwareConfig.getSpiMemSizeBytes(MemSelect::MEM1);
  spiAddressMax = BAHardwareConfig.getSpiMemMaxAddr(1)/4;  // test the first 25% of memory
  bool success = sramManager.requestMemory(&memSlot, numBytes, MemSelect::MEM1, /* no DMA */ false);
  if (!success && Serial) { printf("Request for memory slot failed\n\r"); }  

  // Allocated audio buffers and enable codec
  AudioMemory(64);
  codec.enable();
  delay(100);

  // Mixer at full volume
  volumeOut.gain(0,1.0f);
  volumeOut.gain(1,1.0f);

  // flush the pot filters. The analog measurement of the analog pots is averaged (filtered)
  // over time, so at startup you will see a bunch of false changes detected as the filter
  // settles. We can force this with a few dozen repeated calls.
  for (unsigned i=0; i < 50; i++) {
    float potValue;
    for (unsigned j=0; j < BA_EXPAND_NUM_POT; j++) {
      controls.checkPotValue(j, potValue);      
    }
    delay(10);
  }
}

void loop() {

  // Check all the physical controls for updates
  checkPot(0);
  checkPot(1);
  checkPot(2);
  checkPot(3);
  
  checkSwitch(0);
  checkSwitch(1);
  checkSwitch(2);
  checkSwitch(3);
  checkSwitch(4);
  checkSwitch(5);
  
  checkEncoder(0);
  checkEncoder(1);
  checkEncoder(2);
  checkEncoder(3);    

  // If the SRAM test is not complete, run the next block
  if (sramCompletion < 1.0f) {
    nextSpiMemTestBlock();        
  }

  // Adjusting one of the knobs/switches will result in its value being display for
  // 2 seconds in the check*() functions.
  if (timer > 2000) {
    loopCounter++; 
    display.clearDisplay();
    drawBlackaddrAudio();
    drawSramProgress(sramCompletion);  
    display.display(); 
  }
}

// This function will draw on the display which stage the memory test is in, and
// the percentage complete for that stage.
void drawSramProgress(float completion)
{
  if (errorCount > 0) { // If errors, print the error count
    display.setCursor(0, SCREEN_HEIGHT-1);
    display.printf("Errors: %d", errorCount);    
    return;
  }

  // Draw the SRAM test progress at the bottom of the screen
  display.setCursor(0, SCREEN_HEIGHT-1);
  switch(sramStage) {    
    case 0 : display.printf("0 mem:"); break;
    case 1 : display.printf("0 chk:"); break;
    case 2 : display.printf("wr mem:"); break;
    case 3 : display.printf("rd mem:"); break;
    case 4 : // same as default
    default: display.printf("Done"); break;
  }
  display.setCursor(SCREEN_WIDTH*0.63f, SCREEN_HEIGHT-1);  // position to lower right corner
  display.printf("%0.f%%", 100.0f * completion);
}

// Create a predictable data pattern based on address.
constexpr int mask0 = 0x5555;
constexpr int mask1 = 0xaaaa;
int calcNextData(int spiAddress, int loopPhase, int maskPhase)
{
  int data;

  int phase = ((loopPhase << 1) + maskPhase) & 0x3;
  switch(phase)
  {
    case 0 :
    data = spiAddress ^ mask0;
    break;
    case 1:
    data = spiAddress ^ mask1;
    break;
    case 2:
    data = ~spiAddress ^ mask0;
    break;
    case 3:
    data = ~spiAddress ^ mask1;
    
  }
  return (data & 0xffff);
}

// Process the next block of data in the memory test
void nextSpiMemTestBlock()
{
  constexpr unsigned BLOCK_SIZE_BYTES = 256;  // transfer 256 bytes (arbitrary) per transaction
  constexpr unsigned NUM_BLOCK_WORDS = BLOCK_SIZE_BYTES;
  static uint8_t  buffer[BLOCK_SIZE_BYTES];
  static int16_t buffer16a[NUM_BLOCK_WORDS];
  static int16_t buffer16b[NUM_BLOCK_WORDS];
  static int maskPhase = 0;

  if (sramStage == 0) { // Zero write
    // zero the memory
    while (spiMem1.isWriteBusy()) {} // wait for DMA write to complete
    memSlot.zero(spiAddress, BLOCK_SIZE_BYTES);
    spiAddress += BLOCK_SIZE_BYTES;   
    
  } else if (sramStage == 1) { // Zero check
    memSlot.read(spiAddress, buffer, BLOCK_SIZE_BYTES);
    while (spiMem1.isReadBusy()) {} // wait for DMA read results
    for (unsigned i=0; i < BLOCK_SIZE_BYTES; i++) {
      if (buffer[i] != 0) { errorCount++; }
    }
    spiAddress += BLOCK_SIZE_BYTES;
  
  }
  else if (sramStage == 2) { // write test
    // Calculate the data for a block
    for (unsigned i=0; i<NUM_BLOCK_WORDS; i++) {
      buffer16a[i] = calcNextData(spiAddress+i, 0, 0);
      maskPhase = (maskPhase+1) % 2;
    }
    memSlot.write16(spiAddress, buffer16a, NUM_BLOCK_WORDS);
    while (memSlot.isWriteBusy()) {} // wait for DMA write to complete
    spiAddress += BLOCK_SIZE_BYTES;
    
  }
  else if (sramStage == 3) { // read test
    // Calculate the data for a block
    for (unsigned i=0; i<NUM_BLOCK_WORDS; i++) {
      buffer16a[i] = calcNextData(spiAddress+i, 0, 0);
      maskPhase = (maskPhase+1) % 2;
    }
    memSlot.read16(spiAddress, buffer16b, NUM_BLOCK_WORDS);
    while (memSlot.isReadBusy()) {} // wait for DMA read results
    for (unsigned i=0; i < NUM_BLOCK_WORDS; i++) {
      if (buffer16a[i] != buffer16b[i]) { errorCount++; }
    }    
    spiAddress += BLOCK_SIZE_BYTES;
  }

  else if (sramStage == 4) {
    sramCompletion = 1.0f;
    return;
  }

  if (spiAddress > spiAddressMax && sramStage < 4) { 
    spiAddress = 0; sramStage++; sramCompletion = 0.0f;
    return;
  }

  sramCompletion = (float)spiAddress / (float)spiAddressMax ;
}
