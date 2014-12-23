#include "tlc5940.h"

uint8_t dcData[DC_DATA_SIZE] = {0,};
uint8_t gsData[GS_DATA_SIZE] = {0,};
volatile uint8_t uigsUpdateFlag;

void TLC5940_Init(void) {
	// SET INITIAL PIN STATUS
	setLow(SCLK_PIN);
	setHigh(VPRG_PIN);
	setLow(XLAT_PIN);
	setHigh(BLANK_PIN);
	// LOW
	P2OUT &= ~DCPRG_PIN;

	uigsUpdateFlag = 1;
}

void TLC5940_ClockInDC(void) {
	// SET DOT CORRCTION INITIAL DATA

	// VPRG & DCPRG HIGH
	P2OUT |= DCPRG_PIN;
	setHigh(VPRG_PIN);

	for (dcData_t led=0; led < DC_DATA_SIZE; led++) {
		// TRANSMIT DATA
        UCB0TXBUF = dcData[led];
        while (!(IFG2 & UCB0TXIFG)); // TX buffer ready?
		// TRANSMISSION COMPLETED
	}
	pulse(XLAT_PIN);
}

void TLC5940_SetAllDC(uint8_t value) {
	// SET DOT CORRECTION VALUE OF ALL CHANNELS

	uint8_t tmp1 = (uint8_t)(value << 2);
	uint8_t tmp2 = (uint8_t)(tmp1 << 2);
	uint8_t tmp3 = (uint8_t)(tmp2 << 2);
	tmp1 |= (value >> 4);
	tmp2 |= (value >> 2);
	tmp3 |= value;
	uint8_t i = 0;
	do {
		dcData[i++] = tmp1;				// bits: 05 04 03 02 01 00 05 04
		dcData[i++] = tmp2;				// bits: 03 02 01 00 05 04 03 02
		dcData[i++] = tmp3;				// bits: 01 00 05 04 03 02 01 00
	} while (i < DC_DATA_SIZE);
}

void TLC5940_SetDC(channel_t channel, uint8_t value) {
	// SET DOT CORRECTION VALUE FOR AN INDIVIDUAL CHANNEL
	// DOT CORRECTION VALUE IS 6 BITS
	// PACKED ON 8 BIT - 3/4 RATIO

	channel = NUM_CHANNELS - 1 - channel;
	uint16_t i = (uint16_t)channel * 3 / 4;
	switch (channel % 4) {
		case 0:
			dcData[i] = (dcData[i] & 0x03) | (uint8_t)(value << 2);
			break;
		case 1:
			dcData[i] = (dcData[i] & 0xFC) | (value >> 4);
			i++;
			dcData[i] = (dcData[i] & 0x0F) | (uint8_t)(value << 4);
			break;
		case 2:
			dcData[i] = (dcData[i] & 0xF0) | (value >> 2);
			i++;
			dcData[i] = (dcData[i] & 0x3F) | (uint8_t)(value << 6);
			break;
		default: // case 3:
			dcData[i] = (dcData[i] & 0xC0) | (value);
			break;
	}
}

void TLC5940_SetAllGS(uint16_t value) {
	// SET GRAYSCALE VALUE OF ALL CHANNELS

	uint8_t tmp1 = (value >> 4);
	uint8_t tmp2 = (uint8_t)(value << 4) | (tmp1 >> 4);
	uint8_t i = 0;
	do {
		gsData[i++] = tmp1; 			// bits: 11 10 09 08 07 06 05 04
		gsData[i++] = tmp2; 			// bits: 03 02 01 00 11 10 09 08
		gsData[i++] = (uint8_t)value;	// bits: 07 06 05 04 03 02 01 00
	} while (i < GS_DATA_SIZE);
}

void TLC5940_SetGS(channel_t channel, uint16_t value) {
	// SET  GRAYSCALE DATA FOR AN INDIVIDUAL CHANNEL
	// CHANNELS NEED TO BE SENT TO TLC5940 IN REVERSE ORDER
	// GREYSCALE DATA IS 12 BIT
	// PACKED ON 8 BIT - 3/2 RATIO

	channel = NUM_CHANNELS - 1 - channel;
	uint16_t i = (uint16_t)channel * 3 / 2;
	switch (channel % 2) {
		case 0:
			gsData[i] = (value >> 4);
			i++;
			gsData[i] = (gsData[i] & 0x0F) | (uint8_t)(value << 4);
			break;
		default: // case 1:
			gsData[i] = (gsData[i] & 0xF0) | (value >> 8);
			i++;
			gsData[i] = (uint8_t)value;
			break;
	}
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
		pulse(SCLK_PIN);
	} else if (xlatNeedsPulse){
		pulse(XLAT_PIN);
		xlatNeedsPulse = 0;
	}

	// BLANK LOW
	setLow(BLANK_PIN);

	// Below this we have 4096 cycles to shift in the data for the next cycle
	if (uigsUpdateFlag) {
		for (gsData_t i = 0; i < GS_DATA_SIZE; i++) {
			// TRANSMIT DATA
			UCB0TXBUF = gsData[i];
			while (!(IFG2 & UCB0TXIFG)); // TX buffer ready?
			// TRANSMISSION COMPLETED
		}
		xlatNeedsPulse = 1;
		uigsUpdateFlag = 0;
	}
}

