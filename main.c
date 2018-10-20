/*
*	WS2812 & Acrylic Crystal Lamp
*	main.c
*
*	Created: 2018-09-26 오후 6:12:27
*	Author: Cakeng (PARK JONG SEOK)
*
*	NO LICENCE INCLUDED
*	Contact cakeng@naver.com to
*	use, modify, or share the software for any purpose
*	other than personal use.
*
*/

#define F_CPU 9600000
#include <avr/io.h>
#include <util/delay.h>
#define ledNum	3

uint8_t colorData[ledNum][3];
uint16_t colorHue = 0;
uint8_t colorSat = 0;
uint8_t SatUp = 1;

void ws2812Out(uint8_t * ptr, uint8_t count)
{
	/*
	1/9600000 = 104ns.
	WS2812 Requires 400ns High, 850ns Low for 0, 800ns High, 450ns Low for 1 with +-150ns tolerance.
	r18 : byte data stored.
	r19 : Bit counter.
	r20 : High value for PORTB
	r21 : Low value for PORTB
	portb : PORTB address.
	%a0 : ptr address - Ram address of data to send.
	cnt : count integer - Number of bytes to send.
	*/
	asm(
	"ldi	r20,	16	""\n\t"
	"ldi	r21,	0	""\n\t"
	"ldi	r19,	7	""\n\t"
	"ld		r18,	%a0+""\n\t"
	"LOOP:				""\n\t"
	"out	%[ptb],	r20	""\n\t"
	"nop				""\n\t"
	"lsl	r18			""\n\t"
	"brcs	ONE			""\n\t"	//Branch to ONE if MSB from r18 is 1. (Two clocks if branched)
	"out	%[ptb],	r21	""\n\t"	//4 Clocks High (MSB low - Send 0)
	"nop				""\n\t"
	"nop				""\n\t"
	"nop				""\n\t"
	"subi	r19,	1	""\n\t"
	"breq	BIT8		""\n\t"	//Branch if r19 is zero (last bit)
	"rjmp	LOOP		""\n\t"	//2 clocks for jump - 8 Clocks Low including next out instruction.
	"ONE:				""\n\t"
	"nop				""\n\t"
	"nop				""\n\t"
	"nop				""\n\t"
	"out	%[ptb],	r21	""\n\t"	//8 Clocks High (MSB High - Send 1)
	"subi	r19,	1	""\n\t"
	"brne	LOOP		""\n\t"	//Branch if r19 is not zero (Not last bit). 2 clocks for jump - 4 Clocks Low including next out.
	"BIT8:				""\n\t"		
	"nop				""\n\t"
	"out	%[ptb],	r20	""\n\t"
	"ldi	r19,	7	""\n\t"
	"lsl	r18			""\n\t"
	"brcs	BIT8ONE		""\n\t"	//Branch to 8THONE if MSB from r18 is 1. (Two clocks if branched)
	"out	%[ptb],	r21	""\n\t"	//4 Clocks High (MSB low - Send 0)
	"ld		r18,	%a0+""\n\t"	//Load next byte.
	"subi	%[cnt],	1	""\n\t"	//Subtract from number of bytes left.
	"nop				""\n\t"
	"nop				""\n\t"
	"brne	LOOP		""\n\t"	//If Count isn't 0, Loop back. 2 clocks for jump - 8 Clocks Low including next out instruction.
	"rjmp	EXIT		""\n\t"	//Exit asm.
	"BIT8ONE:			""\n\t"
	"ld		r18,	%a0+""\n\t"	//Load next byte.
	"subi	%[cnt],	1	""\n\t"	//Subtract from number of bytes left.
	"out	%[ptb],	r21	""\n\t"	//8 Clocks High (MSB High - Send 1)
	"nop				""\n\t"
	"brne	LOOP		""\n\t"	//If Count isn't 0, Loop back. 2 clocks for jump - 4 Clocks Low including next out instruction.
	"EXIT:"
	:
	: "e" (ptr), [cnt] "r" (count), [ptb] "I" (_SFR_IO_ADDR(PORTB))
	: "r18", "r19", "r20", "r21"
	);
}



uint32_t rgbTo32Bit(uint8_t r, uint8_t g, uint8_t b)
{
	return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

uint32_t hsvToRgb(uint8_t colorHue, uint8_t sat) //Value of the color is always maxed. (No Grey)
{
	uint8_t satDiff = 255 - sat;
	if(colorHue < 85)
	{
		uint16_t satVals[3] = {(uint16_t)(colorHue * 3), 0, (uint16_t)(255-colorHue * 3)};
		for (uint8_t i = 0; i < 3; i++)
		{
			satVals[i] *= satDiff;
			satVals[i] /= 256;
			satVals[i] += sat;
		}
		return rgbTo32Bit(satVals[0], satVals[1], satVals[2]);
	}
	if(colorHue < 170)
	{
		colorHue -= 85;
		uint16_t satVals[3] = {(uint16_t)(colorHue * 3), 0, (uint16_t)(255-colorHue * 3)};
		for (uint8_t i = 0; i < 3; i++)
		{
			satVals[i] *= satDiff;
			satVals[i] /= 256;
			satVals[i] += sat;
		}
		return rgbTo32Bit(satVals[2], satVals[0], satVals[1]);
	}
	else
	{
		colorHue -= 170;
		uint16_t satVals[3] = {(uint16_t)(colorHue * 3), 0, (uint16_t)(255-colorHue * 3)};
		for (uint8_t i = 0; i < 3; i++)
		{
			satVals[i] *= satDiff;
			satVals[i] /= 256;
			satVals[i] += sat;
		}
		return rgbTo32Bit(satVals[1], satVals[2], satVals[0]);
	}
}

void setPixelColorRgb(uint8_t * ptr, uint8_t num, uint32_t color)
{
	uint8_t	r = (uint8_t)(color >> 16);
	uint8_t g = (uint8_t)(color >>  8);
	uint8_t b = (uint8_t)color;
	ptr[num * 3 + 1] = r;
	ptr[num * 3 + 0] = g;
	ptr[num * 3 + 2] = b;
}

void colorCycle()
{
	for (uint16_t i = 0; i < ledNum; i++)
	{
		setPixelColorRgb(colorData[0], i, hsvToRgb(colorHue,colorSat));
		ws2812Out(colorData[0], ledNum*3);
	}
	if (SatUp)
	{
		colorSat++;
		if (colorSat > 60)
		{
			SatUp = 0;
			colorHue += 3;
			if (colorHue > 255)
			{
				colorHue -= 255;
			}
			for (uint16_t i = 0; i < ledNum; i++)
			{
				setPixelColorRgb(colorData[0], i, hsvToRgb(colorHue,colorSat));
			}
			ws2812Out(colorData[0], ledNum*3);
			_delay_ms(3000);
		}
	}
	else
	{
		colorSat--;
		if (colorSat == 1)
		{
			SatUp = 1;
			_delay_ms(3000);
		}
	}
}

//uint8_t testData[3] = {255,255,255};
int main(void)
{
	DDRB = 0b00011000;
	while (1)
	{
		//ws2812Out(testData, 3);
		colorCycle();
		_delay_ms(180);
	}
}

