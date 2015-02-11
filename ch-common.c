/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011-2012 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ColorHug.h"

#include <delays.h>

#include "ch-common.h"
#include "ch-flash.h"

/**
 * CHugGetColorSelect:
 **/
ChColorSelect
CHugGetColorSelect(void)
{
	return (PORTCbits.RC2 << 1) + PORTCbits.RC4;
}

/**
 * CHugSetColorSelect:
 **/
void
CHugSetColorSelect(ChColorSelect color_select)
{
	PORTCbits.RC4 = (color_select & 0x01);
	PORTCbits.RC2 = (color_select & 0x02) >> 1;
}

/**
 * CHugGetMultiplier:
 **/
ChFreqScale
CHugGetMultiplier(void)
{
	return (PORTAbits.RA5 << 1) + PORTCbits.RC3;
}

/**
 * CHugSetMultiplier:
 **/
void
CHugSetMultiplier(ChFreqScale multiplier)
{
	PORTCbits.RC3 = (multiplier & 0x01);
	PORTAbits.RA5 = (multiplier & 0x02) >> 1;
}

/**
 * CHugGetLEDs:
 **/
uint8_t
CHugGetLEDs(void)
{
	return PORTCbits.RC5;
}

/**
 * CHugSetLEDs:
 **/
void
CHugSetLEDs(uint8_t leds)
{
	PORTCbits.RC5 = leds;
}

/**
 * CHugFatalError:
 **/
void
CHugFatalError (ChError error)
{
	uint8_t i;

	/* turn off watchdog */
	WDTCONbits.SWDTEN = 0;
	TRISC = 0xdf;

	while (1) {
		for (i = 0; i < error; i++) {
			CHugSetLEDs(1);
			Delay10KTCYx(0xff);
			CHugSetLEDs(0);
			Delay10KTCYx(0xff);
		}
		Delay10KTCYx(0xff);
		Delay10KTCYx(0xff);
	}
}
