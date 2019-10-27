/*************************************************************************
 * This demo uses the BALibrary to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BALibrary can be obtained from
 * https://github.com/Blackaddr/BALibrary
 * 
 * This demo will perform a DMA memory test on MEM1 attached
 * to SPI1.
 * 
 * NOTE: SPI MEM1 can be used by a Teensy 3.5/3.6.
 * pins.
 * 
 */
#include <Wire.h>
#include "BALibrary.h"

using namespace BALibrary;

//#define SANITY
#define DMA_SIZE 256

#define SPI_WRITE_CMD 0x2
#define SPI_READ_CMD 0x3
#define SPI_ADDR_2_MASK 0xFF0000
#define SPI_ADDR_2_SHIFT 16
#define SPI_ADDR_1_MASK 0x00FF00
#define SPI_ADDR_1_SHIFT 8
#define SPI_ADDR_0_MASK 0x0000FF
SPISettings memSettings(20000000, MSBFIRST, SPI_MODE0);
const int cs0pin = 15;

BAGpio           gpio;  // access to User LED
BASpiMemoryDMA   spiMem1(SpiDeviceId::SPI_DEVICE1);

bool compareBuffers(uint8_t *a, uint8_t *b, size_t numBytes)
{
  bool pass=true;
  int errorCount = 0;
  for (size_t i=0; i<numBytes; i++) {
    if (a[i] != b[i]) {
      if (errorCount < 10) {
        Serial.print(i); Serial.print(":: a:"); Serial.print(a[i]); Serial.print(" does not match b:"); Serial.println(b[i]);
        pass=false;
        errorCount++;
      } else { return false; }
    }
  }
  return pass;
}

bool compareBuffers16(uint16_t *a, uint16_t *b, size_t numWords)
{
  return compareBuffers(reinterpret_cast<uint8_t *>(a), reinterpret_cast<uint8_t*>(b), sizeof(uint16_t)*numWords);
}

size_t SPI_MAX_ADDR = 0;

void setup() {

  Serial.begin(57600);
  while (!Serial) { yield(); }
  delay(5);

  SPI_MEM1_1M();
  SPI_MAX_ADDR = BAHardwareConfig.getSpiMemMaxAddr(MemSelect::MEM1);

  Serial.println("Enabling SPI, testing MEM1");
  spiMem1.begin();
}


bool spi8BitTest(void) {
  
  size_t spiAddress = 0;
  int spiPhase = 0;
  constexpr uint8_t MASK80 = 0xaa;
  constexpr uint8_t MASK81 = 0x55;
  
  uint8_t src8[DMA_SIZE];
  uint8_t dest8[DMA_SIZE];
  
  // Write to the memory using 8-bit transfers
  Serial.println("\nStarting 8-bit test Write/Read");
  while (spiPhase < 4) {
    spiAddress = 0;
    while (spiAddress <= SPI_MAX_ADDR) {
      
      // fill the write data buffer
      switch (spiPhase) {
        case 0 :
          for (int i=0; i<DMA_SIZE; i++) {
            src8[i] = ( (spiAddress+i) & 0xff) ^ MASK80;
          }
          break;
        case 1 :
          for (int i=0; i<DMA_SIZE; i++) {
            src8[i] = ((spiAddress+i) & 0xff) ^ MASK81;
          }
          break;
        case 2 :
          for (int i=0; i<DMA_SIZE; i++) {
            src8[i] = ((spiAddress+i) & 0xff) ^ (!MASK80);
          }
          break;
        case 3 :
          for (int i=0; i<DMA_SIZE; i++) {
            src8[i] = ((spiAddress+i) & 0xff) ^ (!MASK81);
          }
          break;
      }      
  
      // Send the DMA transfer
      spiMem1.write(spiAddress, src8, DMA_SIZE);
      // wait until write is done
      while (spiMem1.isWriteBusy()) {}
      spiAddress += DMA_SIZE;    
    }
  
    // Read back the data using 8-bit transfers
    spiAddress = 0;
    while (spiAddress <= SPI_MAX_ADDR) {
      // generate the golden data
      switch (spiPhase) {
      case 0 :
        for (int i=0; i<DMA_SIZE; i++) {
          src8[i] = ( (spiAddress+i) & 0xff) ^ MASK80;
        }
        break;
      case 1 :
        for (int i=0; i<DMA_SIZE; i++) {
          src8[i] = ((spiAddress+i) & 0xff) ^ MASK81;
        }
        break;
      case 2 :
        for (int i=0; i<DMA_SIZE; i++) {
          src8[i] = ((spiAddress+i) & 0xff) ^ (!MASK80);
        }
        break;
      case 3 :
        for (int i=0; i<DMA_SIZE; i++) {
          src8[i] = ((spiAddress+i) & 0xff) ^ (!MASK81);
        }
        break;
      }
  
      // Read back the DMA data
      spiMem1.read(spiAddress, dest8, DMA_SIZE);
      // wait until read is done
      while (spiMem1.isReadBusy()) {}
      if (!compareBuffers(src8, dest8, DMA_SIZE)) { 
        Serial.println(String("ERROR @") + spiAddress);
        return false; } // stop the test    
      spiAddress += DMA_SIZE; 
    }
    Serial.println(String("Phase ") + spiPhase + String(": 8-bit Write/Read test passed!"));
    spiPhase++;
  }

#ifdef SANITY
  for (int i=0; i<DMA_SIZE; i++) {
    Serial.print(src8[i], HEX); Serial.print("="); Serial.println(dest8[i],HEX);
  }
#endif

  Serial.println("\nStarting 8-bit test Zero/Read");
  // Clear the memory
  spiAddress = 0;
  memset(src8, 0, DMA_SIZE);
  while (spiAddress <= SPI_MAX_ADDR) {
      // Send the DMA transfer
      spiMem1.zero(spiAddress, DMA_SIZE);
      // wait until write is done
      while (spiMem1.isWriteBusy()) {}
      spiAddress += DMA_SIZE;     
  }

  // Check the memory is clear
  spiAddress = 0;
  while (spiAddress <= SPI_MAX_ADDR) {
    // Read back the DMA data
    spiMem1.read(spiAddress, dest8, DMA_SIZE);
    // wait until read is done
    while (spiMem1.isReadBusy()) {}
    if (!compareBuffers(src8, dest8, DMA_SIZE)) { return false; } // stop the test    
    spiAddress += DMA_SIZE; 
  }
  Serial.println(String(": 8-bit Zero/Read test passed!"));
  return true;
}

bool spi16BitTest(void) {
  size_t spiAddress = 0;
  int spiPhase = 0;
  constexpr uint16_t MASK160 = 0xaaaa;
  constexpr uint16_t MASK161 = 0x5555;
  constexpr int NUM_DATA = DMA_SIZE / sizeof(uint16_t);
  
  uint16_t src16[NUM_DATA];
  uint16_t dest16[NUM_DATA];

  // Write to the memory using 16-bit transfers
  Serial.println("\nStarting 16-bit test");
  while (spiPhase < 4) {
    spiAddress = 0;
    while (spiAddress <= SPI_MAX_ADDR) {
      
      // fill the write data buffer
      switch (spiPhase) {
        case 0 :
          for (int i=0; i<NUM_DATA; i++) {
            src16[i] = ( (spiAddress+i) & 0xffff) ^ MASK160;
          }
          break;
        case 1 :
          for (int i=0; i<NUM_DATA; i++) {
            src16[i] = ((spiAddress+i) & 0xffff) ^ MASK161;
          }
          break;
        case 2 :
          for (int i=0; i<NUM_DATA; i++) {
            src16[i] = ((spiAddress+i) & 0xffff) ^ (!MASK160);
          }
          break;
        case 3 :
          for (int i=0; i<NUM_DATA; i++) {
            src16[i] = ((spiAddress+i) & 0xffff) ^ (!MASK161);
          }
          break;
      }
  
      // Send the DMA transfer
      spiMem1.write16(spiAddress, src16, NUM_DATA);
      // wait until write is done
      while (spiMem1.isWriteBusy()) {}
      spiAddress += DMA_SIZE;    
    }
  
    // Read back the data using 8-bit transfers
    spiAddress = 0;
    while (spiAddress <= SPI_MAX_ADDR) {
      // generate the golden data
      switch (spiPhase) {
      case 0 :
        for (int i=0; i<NUM_DATA; i++) {
          src16[i] = ( (spiAddress+i) & 0xffff) ^ MASK160;
        }
        break;
      case 1 :
        for (int i=0; i<NUM_DATA; i++) {
          src16[i] = ((spiAddress+i) & 0xffff) ^ MASK161;
        }
        break;
      case 2 :
        for (int i=0; i<NUM_DATA; i++) {
          src16[i] = ((spiAddress+i) & 0xffff) ^ (!MASK160);
        }
        break;
      case 3 :
        for (int i=0; i<NUM_DATA; i++) {
          src16[i] = ((spiAddress+i) & 0xffff) ^ (!MASK161);
        }
        break;
      }
  
      // Read back the DMA data
      spiMem1.read16(spiAddress, dest16, NUM_DATA);
      // wait until read is done
      while (spiMem1.isReadBusy()) {}
      if (!compareBuffers16(src16, dest16, NUM_DATA)) { 
        Serial.print("ERROR @"); Serial.println(spiAddress, HEX);
        return false; } // stop the test    
      spiAddress += DMA_SIZE; 
    }
    Serial.println(String("Phase ") + spiPhase + String(": 16-bit test passed!"));
    spiPhase++;
  }
#ifdef SANITY
  for (int i=0; i<NUM_DATA; i++) {
   Serial.print(src16[i], HEX); Serial.print("="); Serial.println(dest16[i],HEX);
  }
#endif

  Serial.println("\nStarting 16-bit test Zero/Read");
  // Clear the memory
  spiAddress = 0;
  memset(src16, 0, DMA_SIZE);
  while (spiAddress <= SPI_MAX_ADDR) {
      // Send the DMA transfer
      spiMem1.zero16(spiAddress, NUM_DATA);
      // wait until write is done
      while (spiMem1.isWriteBusy()) {}
      spiAddress += DMA_SIZE;     
  }

  // Check the memory is clear
  spiAddress = 0;
  while (spiAddress <= SPI_MAX_ADDR) {
    // Read back the DMA data
    spiMem1.read16(spiAddress, dest16, NUM_DATA);
    // wait until read is done
    while (spiMem1.isReadBusy()) {}
    if (!compareBuffers16(src16, dest16, NUM_DATA)) { return false; } // stop the test    
    spiAddress += DMA_SIZE; 
  }
  Serial.println(String(": 16-bit Zero/Read test passed!"));
  return true;
}

void loop() {

  spi8BitTest();
  spi16BitTest();
  Serial.println("\nTEST DONE!");
  while(true) {}

}
