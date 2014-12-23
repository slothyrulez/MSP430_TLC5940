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

#define TLC5940_NUMBER 1
#define NUMBER_OF_LEDS 16

#define dcDataSize ((uint8_t)12 * TLC5940_NUMBER)
#define gsDataSize ((uint8_t)24 * TLC5940_NUMBER)

uint8_t dcData[ 12 * TLC5940_NUMBER ] = {
    // MSB	    LSB
    // 3 ARRAYS =  4 LED
    0b11111111,
    0b11111111,
    0b11111111,

    0b11111111,
    0b11111111,
    0b11111111,

    0b11111111,
    0b11111111,
    0b11111111,

    0b11111111,
    0b11111111,
    0b11111111,
};

uint8_t gsData[24 * TLC5940_NUMBER] = {
    // 12 - 0
	// IGNORED | MSB ----------------------------------->LSB
    // 3 ARRAYS = 2 LEDS

    0b00000000,
    0b00000000,
    0b00000001,

    0b00000000,
    0b00100000,
    0b00000100,

    0b00000000,
    0b10000000,
    0b00010000,

    0b00000010,
    0b00000000,
    0b01000000,

    0b00001000,
    0b00000001,
    0b00000000,

    0b00100000,
    0b00000100,
    0b00000000,

    0b10000000,
    0b00001000,
    0b00000000,

    0b11000000,
    0b00001110,
    0b00000000,
};

typedef unsigned char u_char;

void TLC5940_Init(void);
void MSP430_Init(void);
void MSP430_Timer(void);
void TLC5940_ClockInDC(void);
//void TLC5940_SetGS_And_GS_PWM(void);

void TLC5940_Init(void){
	// PORT 1
	// VPRG | XLAT | BLANK
	P1DIR |= (VPRG_PIN | XLAT_PIN | BLANK_PIN);
	setHigh(VPRG_PIN);
	setLow(XLAT_PIN);
	setHigh(BLANK_PIN);

	// GSCLK, pulsed by clock output
    P1DIR |= GSCLK_PIN; // port 1.4 configured as SMCLK out
    P1SEL |= GSCLK_PIN;

	// PORT 2
    // DCPRG P2.0
    // DCPRG OUTPUT - LOW
	P2OUT &= ~(DCPRG_PIN);
	P2DIR |= DCPRG_PIN;
}

void MSP430_Init(void) {
	// DISABLE WDT
    WDTCTL = WDTPW + WDTHOLD;
    // 16MHz CLOCK
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    // LED PIN P1.6
    P1OUT &= ~(BIT6);
    P1DIR |= BIT6;
}

void MSP430_Timer(void){
	// SETUP TIMER

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

void TLC5940_ClockInDC(void) {
	// VPRG & DCPRG HIGH
	P2OUT |= DCPRG_PIN;
	setHigh(VPRG_PIN);

	for (uint8_t led=0; led < dcDataSize; led++) {
		// TRANSMIT DATA
        UCB0TXBUF = dcData[led];
        while (!(IFG2 & UCB0TXIFG)); // TX buffer ready?
		// TRANSMISSION COMPLETED
	}
	pulse(XLAT_PIN);
}


int main(void){
	MSP430_Init();
	TLC5940_Init();
	MSP430_Timer();
	TLC5940_ClockInDC();
	// GLOBAL INTERRUPTS ENABLED
	__bis_status_register(GIE + LPM0_bits);
	while(1){
		P1OUT ^= BIT6;     /* Toggle Pin P1.6 */
	}
	return 0;
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A0(void) {
	static uint8_t xlatNeedsPulse = 0;

	setHigh(BLANK_PIN);

	if (outputState(VPRG_PIN)) {
		setLow(VPRG_PIN);
		if (xlatNeedsPulse) {
			pulse(XLAT_PIN);
			xlatNeedsPulse = 0;
		}
		//pulse(SCLK_PIN);
	} else if (xlatNeedsPulse){
		pulse(XLAT_PIN);
		xlatNeedsPulse = 0;
	}

	// BLANK LOW
	setLow(BLANK_PIN);

	// Below this we have 4096 cycles to shift in the data for the next cycle
	for (uint8_t led = 0; led < gsDataSize; led++) {
		// TRANSMIT DATA
        UCB0TXBUF = gsData[led];
        while (!(IFG2 & UCB0TXIFG)); // TX buffer ready?
		// TRANSMISSION COMPLETED
	}
	xlatNeedsPulse = 1;
}
