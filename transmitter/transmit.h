#ifndef _H_TRANSMIT
#define _H_TRANSMIT

#ifdef TEST
#include <stdio.h>
#include <sys/kernel_types.h>
#else
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#endif

//const uint32_t ID = 0x66666666; //0x5555 
extern const uint32_t ID;
void transmitmanch(uint16_t tx);
void transmitend();

void transmitframe(uint8_t HinBye);
#endif
