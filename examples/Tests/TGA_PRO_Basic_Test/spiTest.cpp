#include <cstddef>
#include <cstdint>
#include "BAHardware.h"
#include "BASpiMemory.h"

constexpr int NUM_TESTS = 12;
constexpr int NUM_BLOCK_WORDS = 128;
constexpr int mask0 = 0x5555;
constexpr int mask1 = 0xaaaa;

//#define SANITY_CHECK

using namespace BALibrary;

int SPI_MAX_ADDR = 0;

int calcData(int spiAddress, int loopPhase, int maskPhase)
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

bool spiTest(BASpiMemory *mem, int id)
{
  int spiAddress = 0;
  int spiErrorCount = 0;

  int maskPhase = 0;
  int loopPhase = 0;
  uint16_t memBlock[NUM_BLOCK_WORDS];
  uint16_t goldData[NUM_BLOCK_WORDS];

  SPI_MAX_ADDR = BAHardwareConfig.getSpiMemMaxAddr(0); // assume for this test both memories are the same size so use MEM0
  const size_t SPI_MEM_SIZE_BYTES = BAHardwareConfig.getSpiMemSizeBytes(id);

  Serial.println(String("Starting SPI MEM Test of ") + SPI_MEM_SIZE_BYTES + String(" bytes"));

  for (int cnt = 0; cnt < NUM_TESTS; cnt++) {

    // Zero check
    mem->zero16(0, SPI_MEM_SIZE_BYTES / sizeof(uint16_t));    
    while (mem->isWriteBusy()) {}
    
    for (spiAddress = 0; spiAddress <= SPI_MAX_ADDR; spiAddress += NUM_BLOCK_WORDS*sizeof(uint16_t)) {
      mem->read16(spiAddress, memBlock, NUM_BLOCK_WORDS);
      while (mem->isReadBusy()) {}
      for (int i=0; i<NUM_BLOCK_WORDS; i++) {
        if (memBlock[i] != 0) {
          spiErrorCount++;
          if (spiErrorCount >= 10) break;
        }
      }
      if (spiErrorCount >= 10) break;
    }
  
    //if (spiErrorCount == 0) { Serial.println(String("SPI MEMORY(") + cnt + String("): Zero test PASSED!")); }
    if (spiErrorCount == 0) { Serial.print("."); Serial.flush(); }
    if (spiErrorCount > 0) { Serial.println(String("SPI MEMORY(") + cnt + String("): Zero test FAILED, error count = ") + spiErrorCount); return false;}
    

    // Write all test data to the memory
    maskPhase = 0;
    for (spiAddress = 0; spiAddress <= SPI_MAX_ADDR; spiAddress += NUM_BLOCK_WORDS*sizeof(uint16_t)) {      
      // Calculate the data for a block
      for (int i=0; i<NUM_BLOCK_WORDS; i++) {
        memBlock[i] = calcData(spiAddress+i, loopPhase, maskPhase);
        maskPhase = (maskPhase+1) % 2;
      }
      mem->write16(spiAddress, memBlock, NUM_BLOCK_WORDS);
      while (mem->isWriteBusy()) {}
    }

    // Read back the test data
    spiErrorCount = 0;
    spiAddress = 0;
    maskPhase = 0;

    for (spiAddress = 0; spiAddress <= SPI_MAX_ADDR; spiAddress += NUM_BLOCK_WORDS*sizeof(uint16_t)) {
            
      mem->read16(spiAddress, memBlock, NUM_BLOCK_WORDS);        
      // Calculate the golden data for a block
      for (int i=0; i<NUM_BLOCK_WORDS; i++) {
        goldData[i] = calcData(spiAddress+i, loopPhase, maskPhase);
        maskPhase = (maskPhase+1) % 2;
      }
      while (mem->isReadBusy()) {}  // wait for the read to finish

      for (int i=0; i<NUM_BLOCK_WORDS; i++) {
        if (goldData[i] != memBlock[i]) {
          Serial.println(String("ERROR@ ") + i + String(": ") + goldData[i] + String("!=") + memBlock[i]);
          spiErrorCount++;
          if (spiErrorCount >= 10) break;
        } 
        #ifdef SANITY_CHECK
        else {
          if ((spiAddress == 0) && (i<10)  && (cnt == 0) ){
            Serial.println(String("SHOW@ ") + i + String(": ") + goldData[i] + String("==") + memBlock[i]);
          }
        }
        #endif
      }
      if (spiErrorCount >= 10) break;
    }

    //if (spiErrorCount == 0) { Serial.println(String("SPI MEMORY(") + cnt + String("): Data test PASSED!")); }
    if (spiErrorCount == 0) { Serial.print("."); Serial.flush(); }
    if (spiErrorCount > 0)  { Serial.println(String("SPI MEMORY(") + cnt + String("): Data test FAILED, error count = ") + spiErrorCount); return false;}

    loopPhase = (loopPhase+1) % 2;
  }
  return true;
}
