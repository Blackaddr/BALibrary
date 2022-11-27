#ifndef DEBUG_PRINTF_H_
#define DEBUG_PRINTF_H_

// Make a safe call to Serial where it checks if the object is valid first. This allows your
// program to work even when no USB serial is connected. Printing to the Serial object when
// no valid connection is present will crash the CPU.
#define DEBUG_PRINT(x) {if (Serial) {x;}}

#endif
