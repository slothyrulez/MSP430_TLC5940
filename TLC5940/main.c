#include <msp430.h>
#include <msp430g2553.h>
#include <stdint.h>
#include "tlc5940.h"

// HEADERS
void MSP430_Init(void);
void MSP430_Ports(void);
void MSP430_Timer(void);



void MSP430_Init(void) {
	// INITIALIZE COMMON MSP430 BITS
	// DISABLE WDT
    WDTCTL = WDTPW + WDTHOLD;
    // 16MHz CLOCK
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;
}

void MSP430_Ports(void) {
	// CONFIGURE PORTS
	// PORT 1
	// VPRG | XLAT | BLANK
	P1DIR |= (VPRG_PIN | XLAT_PIN | BLANK_PIN);

	// GSCLK, pulsed by clock output
    P1DIR |= GSCLK_PIN; // port 1.4 configured as SMCLK out
    P1SEL |= GSCLK_PIN;

	// PORT 2
    // DCPRG P2.0
    // DCPRG OUTPUT - LOW
	P2OUT &= ~(DCPRG_PIN);
	P2DIR |= DCPRG_PIN;
}

void MSP430_Timer(void){
	// SETUP TIMER && UCSI

	// COUNTER = 0xFFF = 4095
    CCR0 = 0xFFF;
    // SMCLK, UP MODE 1:1
    // TASSEL_2	>	SMCLK AS SOURCE
    // MC_1		>	UP MODE UNTIL TACCR0
    // ID_0 	>	CLOCK DIVIDED BY 1
    TACTL = TASSEL_2 + MC_1 + ID_0;
    // CCR0 INTERRUPT ENABLED
    CCTL0 = CCIE;

    // SETUP UCB0
    // SCLK_PIN P1.5 (CLOCK INPUT/OUTPUY) UCB0CLK
    // MOSI_PIN P1.7 SIMO (SLAVE IN MASTER OUT) UCB0SIMO
    // ALTERNATE FUNCTION USCIB0

    // SCLK_PIN
    // MOSI_PIN
    P1SEL |= SCLK_PIN + MOSI_PIN;
    P1SEL2 |= SCLK_PIN + MOSI_PIN;

    // CONFIGURE USCIB0
    // 3-PIN, 8-BIT SPI master
    UCB0CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC;
    // SMCLK
    UCB0CTL1 |= UCSSEL_2;
    // 1:1
    UCB0BR0 |= 0x01;
    UCB0BR1 = 0;
    // CLEAR SW, SOFTWARE RESET
    UCB0CTL1 &= ~UCSWRST;
}

int main(void){
	MSP430_Init();
	MSP430_Ports();
	MSP430_Timer();
	TLC5940_Init();

	TLC5940_SetAllDC(63);
	TLC5940_ClockInDC();

	// Default all channels to off
	TLC5940_SetAllGS(0);


	channel_t ch = 0;
	uint16_t bright = 0;

	// GLOBAL INTERRUPTS ENABLED
	__bis_status_register(GIE);

	for(;;){
		while(uigsUpdateFlag);	// wait until we can modify gsData
		//TLC5940_SetAllGS(0);
		TLC5940_SetGS(ch, bright);
		TLC5940_SetGSUpdateFlag();
		__delay_cycles(10000);
		ch = (ch + 1) % NUM_CHANNELS;
		bright++;
		if (bright > 4095){
			bright = 0;
		}
	}
	return 0;
}
