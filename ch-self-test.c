/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011-2015 Richard Hughes <richard@hughsie.com>
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

#include "ch-common.h"
#include "ch-flash.h"
#include "ch-self-test.h"

/**
 * CHugSelfTestSensor:
 **/
static uint8_t
CHugSelfTestSensor(uint8_t min_pulses)
{
	uint16_t i;
	uint8_t pulses = 0;
	uint8_t ra_tmp = PORTA;

	/* check sensor reports some values */
	for (i = 0; i < 0xffff && pulses < min_pulses; i++) {
		if (ra_tmp != PORTA)
			pulses++;
	}
	return pulses;
}

/**
 * CHugSelfTest:
 **/
uint8_t
CHugSelfTest(void)
{
	const uint8_t min_pulses = 3;
	uint8_t pulses[3];
	uint8_t rc;

	/* check multiplier can be set and read */
	CHugSetMultiplier(CH_FREQ_SCALE_0);
	if (CHugGetMultiplier() != CH_FREQ_SCALE_0)
		return CH_ERROR_SELF_TEST_MULTIPLIER;
	CHugSetMultiplier(CH_FREQ_SCALE_100);
	if (CHugGetMultiplier() != CH_FREQ_SCALE_100)
		return CH_ERROR_SELF_TEST_MULTIPLIER;

	/* check color select can be set and read */
	CHugSetColorSelect(CH_COLOR_SELECT_RED);
	if (CHugGetColorSelect() != CH_COLOR_SELECT_RED)
		return CH_ERROR_SELF_TEST_COLOR_SELECT;
	CHugSetColorSelect(CH_COLOR_SELECT_GREEN);
	if (CHugGetColorSelect() != CH_COLOR_SELECT_GREEN)
		return CH_ERROR_SELF_TEST_COLOR_SELECT;

	/* check red, green and blue */
	CHugSetColorSelect(CH_COLOR_SELECT_RED);
	pulses[CH_COLOR_OFFSET_RED] = CHugSelfTestSensor(min_pulses);
	CHugSetColorSelect(CH_COLOR_SELECT_GREEN);
	pulses[CH_COLOR_OFFSET_GREEN] = CHugSelfTestSensor(min_pulses);
	CHugSetColorSelect(CH_COLOR_SELECT_BLUE);
	pulses[CH_COLOR_OFFSET_BLUE] = CHugSelfTestSensor(min_pulses);

	/* are all the results invalid? */
	if (pulses[CH_COLOR_OFFSET_RED] != min_pulses &&
	    pulses[CH_COLOR_OFFSET_GREEN] != min_pulses &&
	    pulses[CH_COLOR_OFFSET_BLUE] != min_pulses)
		return CH_ERROR_SELF_TEST_SENSOR;

	/* one sensor color invalid */
	if (pulses[CH_COLOR_OFFSET_RED] != min_pulses)
		return CH_ERROR_SELF_TEST_RED;
	if (pulses[CH_COLOR_OFFSET_GREEN] != min_pulses)
		return CH_ERROR_SELF_TEST_GREEN;
	if (pulses[CH_COLOR_OFFSET_BLUE] != min_pulses)
		return CH_ERROR_SELF_TEST_BLUE;

	/* success */
	return CH_ERROR_NONE;
}
