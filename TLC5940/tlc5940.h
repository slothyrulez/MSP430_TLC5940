#ifndef _TLC5940_H_
#define _TLC5940_H_

#include <msp430.h>
#include <msp430g2553.h>
#include <stdint.h>

// VPRG		>	P1.0
#define VPRG_PIN	BIT0
// XLAT		>	P1.1
#define XLAT_PIN 	BIT1
// BLANK	>	P1.2
#define BLANK_PIN 	BIT2
// SCLK		>	P1.5
#define SCLK_PIN 	BIT5
// MOSI		>	P1.7
#define MOSI_PIN 	BIT7
// GSCLK	>	P1.4
#define GSCLK_PIN 	BIT4
// DCPRG	>	P2.0
#define DCPRG_PIN 	BIT0

// HELPERS
#define setHigh(pin) 		( P1OUT |= pin )
#define setLow(pin)			( P1OUT &= ~pin )
#define toggle(pin)			( P1OUT ^= pin )
#define outputState(pin)	( P1OUT &= (1 << pin) )
#define pulse(pin)			do { setHigh(pin); setLow(pin); } while(0)

extern volatile uint8_t uigsUpdateFlag;
#define TLC5940_NUMBER 1
#define NUMBER_OF_LEDS 16

// COMPILE TIME TYPE DEFINITIONS
#if (12 * TLC5940_NUMBER > 255)
#define dcData_t uint16_t
#else
#define dcData_t uint8_t
#endif

#if (24 * TLC5940_NUMBER > 255)
#define gsData_t uint16_t
#else
#define gsData_t uint8_t
#endif

#if (16 * TLC5940_NUMBER > 255)
#define channel_t uint16_t
#else
#define channel_t uint8_t
#endif


#define DC_DATA_SIZE ((dcData_t)12 * TLC5940_NUMBER)
#define GS_DATA_SIZE ((gsData_t)24 * TLC5940_NUMBER)
#define NUM_CHANNELS ((channel_t)16 * TLC5940_NUMBER)

extern uint8_t dcData[DC_DATA_SIZE];
// MSB -------> LSB
// 3 ARRAYS =  4 LED
// Ex.	0b11111111,
//	    0b11111111,
//	    0b11111111,


extern uint8_t gsData[GS_DATA_SIZE];
// 12 - 0 BITS x CHANNEL
// MSB --------> LSB
// 3 ARRAYS = 2 LEDS
// Ex.	0b00000000,
//    	0b00010000,
//		0b00000001,

static inline void TLC5940_SetGSUpdateFlag(void) {
	// MEMORY BARRIER
	__asm__ volatile ("" ::: "memory");
	uigsUpdateFlag = 1;
}
void TLC5940_Init(void);
void TLC5940_ClockInDC(void);
void TLC5940_SetAllDC(uint8_t);
void TLC5940_SetDC(channel_t, uint8_t);
void TLC5940_SetGS(channel_t, uint16_t);
void TLC5940_SetAllGS(uint16_t);
#endif //_TLC5940_H_
