#include "DmaSpi.h"

#if defined(__IMXRT1062__) // T4.X
    DmaSpi0 DMASPI0;
    #if defined(ARDUINO_TEENSY_MICROMOD) || defined (ARDUINO_TEENSY41)
        DmaSpi1 DMASPI1;
    #endif

#elif defined(KINETISK)
    DmaSpi0 DMASPI0;
    #if defined(__MK66FX1M0__)
        DmaSpi1 DMASPI1;
        //DmaSpi2 DMASPI2;
    #endif
#elif defined (KINETISL)
    DmaSpi0 DMASPI0;
    DmaSpi1 DMASPI1;
#else
#endif // defined
