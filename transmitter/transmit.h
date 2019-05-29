#ifndef _H_TRANSMIT
#define _H_TRANSMIT

#ifndef AVR
#include <stdio.h>
#include <sys/kernel_types.h>
#endif

//const uint32_t ID = 0x66666666; //0x5555 
extern const uint32_t ID;
extern void transmitmanch(uint16_t tx);
extern void transmitend();

void transmitframe(uint8_t HinBye);
#endif
