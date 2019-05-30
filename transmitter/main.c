// MainsSensor.
// doel: op zuinige attiny (attiny10? attiny45?) bij opstarten en elke paar minuten op 433Mhz eigen ID zenden, en bij wegvallen van de voeding een afmeldbericht sturen (Met de energie uit een elco) met datzelfde ID.

// PORTB1 data output
// PORTB2 = INT0 watch powerloss (same pin on both t45 and t10)

//#define t45
#define t10

// TODO: Please note: do not forget to edit the makefile as well when changing MCU between attiny45 and attiny10 (in 2 places), and when using t45, set its fuses correctly.

#define F_CPU 128000UL // 128 Khz internal osc. (t45 lfuse:E4, rest default. 0x64 for ckdiv8 16 kHz clock, 0x62 default 8/8=1Mhz. t10 clock can be changed at runtime, t45 clock can not be changed at runtime.)

// NOTE: It will be real slow then, so limit bitclock for programming: avrdude -p t45 -c dragon_isp -t -B 50 (400 at 16Khz)
// NOTE: Do NOT enable debugwire at this slow clock. It will make reprogramming impossible (debug won't work either so it bricks the chip, btdt)

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "transmit.h"

const uint32_t ID = 0x6A6A6A6A; //0x7777

void transmitend() {
  PORTB=0;
};

void transmitmanch(uint16_t tx){ 
  
uint8_t i=0;
  do{
     if(tx&0x8000) PORTB=(1<<PORTB1); else PORTB=0; // MSB first, then.
     i++;    
     tx=tx<<1;//MSB first	
     _delay_us(180); // at 128Khz clock delay is needed. At 16Khz clock a negative delay would be nice...
    }while(i<16);

}

int main(void){
DDRB|=(1<<PORTB1); // PORTB1 output
PORTB=0; // start with the pin LOW

#ifdef t45
PRR = 0x0F; // disable powerhungry peripherals
//enable INT0
//MCUCR=0x00; //0x00 is the default and means "low level on PB2 triggers INT0"
GIMSK = (1<<6); // enable INT0
#endif

#ifdef t10
PRR = 0x03;// disable powerhungry peripherals
CCP = 0xD8; // signature for changing protected registers such as CLKMSR
CLKMSR = 0x01; // select 128Khz internal oscilator as main clock. Default prescaler is 8, so 16 Khz main clock.
CCP = 0xD8; // signature for changing protected registers such as CLKMSR
CLKPSR = 0x00; // set prescaler to 1, so 128 kHz main clock.
//EICRA=0x00; //0x00 is the default and means "low level on PB2 triggers INT0"
EIMSK = 0x01; // enable INT0
#endif


/* do the waiting after the right clock is selected! */
while((PINB&(1<<PINB2))==0); // wait untill bulk cap is charged
_delay_ms(5000); // and slightly longer, because input flips before it is full enough.

sei(); // Enable interupts after PB2 is high


while(1){
    transmitframe(1); // on powerup and every minute or so, transmit powerup msg
    //_delay_ms(60000);
    _delay_ms(30000); // or half a minute.
    //_delay_ms(10000); // so let's test with 10s...
    }
}


ISR(BADISR_vect)
{
    // just reset, but have this here so I could in theory add a handler
}

ISR(INT0_vect){ //note INT0 is on PB2 

// if PB2 is low, power failed / is going down
    do{
        transmitframe(0); // transmit goodbye
    }while((PINB&(1<<PINB2))==0); // until power goes out or returns.
}
