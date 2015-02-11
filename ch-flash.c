/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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

#include "ch-flash.h"

/**
 * CHugFlashErase:
 **/
uint8_t
CHugFlashErase(uint16_t addr, uint16_t len)
{
	uint16_t i;

	/* can only erase one block */
	if (addr >= CH_EEPROM_ADDR_MAX)
		return CH_ERROR_INVALID_ADDRESS;
	if (addr % CH_FLASH_ERASE_BLOCK_SIZE > 0)
		return CH_ERROR_INVALID_ADDRESS;
	if (len % CH_FLASH_ERASE_BLOCK_SIZE > 0)
		return CH_ERROR_INVALID_LENGTH;

	/* PMADR is addressed as words, not as bytes */
	addr /= 2;

	/* erase in chunks of 32 words */
	for (i = 0; i < len; i += 32) {
		PMCON1bits.CFGS = 0;
		PMCON1bits.FREE = 1;
		PMCON1bits.WREN = 1;
		PMADR = addr + i;
		PMCON2 = 0x55;
		PMCON2 = 0xAA;
		PMCON1bits.WR = 1;
		asm("nop");
		asm("nop");
	}
	PMCON1bits.WREN = 0;
	return CH_ERROR_NONE;
}

/**
 * CHugFlashWrite:
 **/
uint8_t
CHugFlashWrite(uint16_t addr, uint16_t len, const uint8_t *data)
{
	uint16_t i;

	/* can only write aligned to block */
	if (addr >= CH_EEPROM_ADDR_MAX)
		return CH_ERROR_INVALID_ADDRESS;

	/* PMADR is addressed as words, not as bytes */
	addr /= 2;

	/* write in chunks of 2 bytes */
	for (i = 0; i < len; i += 2) {
		PMCON1bits.WREN = 1;
		PMCON1bits.CFGS = 0;
		PMCON1bits.LWLO = 0;
		PMCON1bits.FREE = 0;
		PMADR = addr++;
		PMDATL = data[i];
		if (i + 1 < len)
			PMDATH = data[i + 1];
		PMCON2 = 0x55;
		PMCON2 = 0xAA;
		PMCON1bits.WR = 1;
		asm("nop");
		asm("nop");
	}
	PMCON1bits.WREN = 0;

	return CH_ERROR_NONE;
}

/**
 * CHugFlashRead:
 **/
uint8_t
CHugFlashRead(uint16_t addr, uint16_t len, uint8_t *data)
{
	uint16_t i;

	/* validate */
	if (addr >= CH_EEPROM_ADDR_MAX)
		return CH_ERROR_INVALID_ADDRESS;
	if (addr % 2 > 0)
		return CH_ERROR_INVALID_ADDRESS;

	/* clear memory */
	for (i = 0; i < len; i++)
		data[i] = 0xff;

	/* PMADR is addressed as words, not as bytes */
	addr /= 2;

	/* read in chunks of 2 bytes */
	for (i = 0; i < len; i += 2) {
		PMADR = addr++;
		PMCON1bits.CFGS = 0;
		PMCON1bits.RD = 1;
		asm("nop");
		asm("nop");
		data[i] = PMDATL;
		/* odd number of bytes to read */
		if (i + 1 >= len)
			break;
		data[i + 1] = PMDATH;
	}
	return CH_ERROR_NONE;
}
