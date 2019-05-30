#include "transmit.h"
/*
normal manchester
0      0b01010101
1      0b01010110
2      0b01011001
3      0b01011010
4      0b01100101
5      0b01100101
6      0b01101001
7      0b01101010
8      0b10010101
9      0b10010110
A      0b10011001 
B      0b10011010
C      0b10100101
D      0b10100110
E      0b10101001
F      0b10101010
*/

// Manchester per IEEE 802.3 - LSB shifted out first.
//
const uint8_t m0 =0b01010101; //0x55
const uint8_t m1 =0b01010110; //0x56
const uint8_t m2 =0b01011001; //0x59
const uint8_t m3 =0b01011010; //0x5A
const uint8_t m4 =0b01100101; //0x65
const uint8_t m5 =0b01100110; //0x66
const uint8_t m6 =0b01101001; //0x69
const uint8_t m7 =0b01101010; //0x6A // start seeing a pattern here...
const uint8_t m8 =0b10010101; //0x95
const uint8_t m9 =0b10010110; //0x96
const uint8_t mA =0b10011001; //0x99
const uint8_t mB =0b10011010; //0x9A
const uint8_t mC =0b10100101; //0xA5
const uint8_t mD =0b10100110; //0xA6
const uint8_t mE =0b10101001; //0xA9
const uint8_t mF =0b10101010; //0xAA

//const uint32_t ID = ((mB<<24)|(mA<<16|(mD<<8)|(m1)); // Unique ID. 0xBAD1 Pre-convert to manchester encoding (because why do that at runtime if it is a constant anyway) 
//const uint32_t ID = 0b10011010100110011010011001010110; // 0xBAD1 , apearently shifting as above is non-const?

//pick one or add more (As there is no EEPROM, ID is set at compile time)
//const uint32_t ID = 0x569A569A; //0x1B1B
//const uint32_t ID = 0x55555555; //0x0000
//const uint32_t ID = 0xA599AAA9; //0xCAFE
//const uint32_t ID = 0x9A555555; //0xB000
//const uint32_t ID = 0x9A999AA9; //0xBABE

/* IN USE: */
//const uint32_t ID = 0x9AA9A966; //0xBEE5 (BEES)
//const uint32_t ID = 0x9A55559A; //0xB00B
//const uint32_t ID = 0x95959595; //0x8888
//const uint32_t ID = 0x66666666; //0x5555 

void transmitframe(uint8_t HinBye){
//HinBye is used as bool, 0 means "Bye", all else means "Hi"
transmitmanch((mF<<8)|mF); // Bias RX/TX / preamble. All "ones" so no matter which "one" it sees as a startbit, things will be fine once synchronized
transmitmanch((mF<<8)|mF); 
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mA<<8)|m5); // sync word, pre converted to machester 0xA5 = 0b10100101 -manch-> 0b1001 1001 0110 0110 = 0x9966
transmitmanch((ID>>16)&0xFFFF); // MSB first
transmitmanch(ID&0xFFFF);
if(HinBye) transmitmanch((mF<<8)|mF); else transmitmanch((m0<<8)|m0); // Hi=0xFF, Bye=0x00. 
transmitend(); // always end with the pin LOW
}


