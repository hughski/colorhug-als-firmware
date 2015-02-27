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
 *
 * Additionally, some constants and code snippets have been taken from
 * freely available datasheets which are:
 *
 * Copyright (C) Microchip Technology, Inc.
 */

#include "ColorHug.h"
#include "HardwareProfile.h"
#include "usb_config.h"
#include "ch-common.h"
#include "ch-flash.h"

#include <delays.h>
#include <USB/usb.h>
#include <USB/usb_common.h>
#include <USB/usb_device.h>
#include <USB/usb_function_hid.h>

/**
 * ISRCode:
 **/
void interrupt
ISRCode(void)
{
}

/* ensure this is incremented on each released build */
static uint16_t		FirmwareVersion[3] = { 3, 0, 2 };

static uint16_t		SensorIntegralTime = 0xffff;
static ChFreqScale	multiplier_old = CH_FREQ_SCALE_0;

/* this is used to map the firmware to a hardware version */
static const char flash_id[] = CH_FIRMWARE_ID_TOKEN;

/* USB idle support */
static uint8_t		idle_command = 0x00;
static uint8_t		idle_counter = 0x00;

/* USB buffers */
static uint8_t RxBuffer[CH_USB_HID_EP_SIZE];
static uint8_t TxBuffer[CH_USB_HID_EP_SIZE];
USB_HANDLE		USBOutHandle = 0;
USB_HANDLE		USBInHandle = 0;

/**
 * CHugTakeReadingRaw:
 *
 * The TAOS3200 sensor with the external IR filter gives the following rough
 * outputs with red selected at 100%:
 *
 *   Frequency   |   Condition
 * --------------|-------------------
 *    10Hz       | Aperture covered
 *    160Hz      | TFT backlight on, but masked to black
 *    1.24KHz    | 100 white at 170cd/m2
 **/
static uint32_t
CHugTakeReadingRaw (uint32_t integral_time)
{
	const uint8_t abs_scale[] = {  5, 5, 7, 6 }; /* red, white, blue, green */
	uint32_t i;
	uint32_t number_edges = 0;
	uint8_t ra_tmp = PORTA;

	/* wait for the output to change so we start on a new pulse
	 * rising edge, which means more accurate black readings */
	for (i = 0; i < integral_time; i++) {
		if (ra_tmp != PORTA) {
			/* ___      ____
			 *    |____|    |___
			 *
			 *         ^- START HERE
			 */
			if (PORTAbits.RA4 == 1)
				break;
			ra_tmp = PORTA;
		}
	}

	/* we got no change */
	if (i == integral_time)
		return 0;

	/* count how many times we get a rising edge */
	for (i = 0; i < integral_time; i++) {
		if (ra_tmp != PORTA) {
			if (PORTAbits.RA4 == 1)
				number_edges++;
			ra_tmp = PORTA;
		}
	}

	/* scale it according to the datasheet */
	ra_tmp = CHugGetColorSelect();
	number_edges *= abs_scale[ra_tmp];

	return number_edges;
}

/**
 * CHugDeviceIdle:
 **/
static void
CHugDeviceIdle(void)
{
	switch (idle_command) {
	case CH_CMD_RESET:
		RESET();
		break;
	}
	idle_command = 0x00;
}

/**
 * ProcessIO:
 **/
static void
ProcessIO(void)
{
	uint16_t address;
	uint16_t calibration_index;
	uint32_t reading;
	uint8_t checksum;
	uint8_t i;
	uint8_t length;
	uint8_t cmd;
	uint8_t rc = CH_ERROR_NONE;

	/* User Application USB tasks */
	if ((USBDeviceState < CONFIGURED_STATE) ||
	    (USBSuspendControl == 1))
		return;

	/* no data was received */
	if (HIDRxHandleBusy(USBOutHandle)) {
		if (idle_counter++ == 0xff &&
		    idle_command != 0x00)
			CHugDeviceIdle();
		return;
	}

	/* we're waiting for a read from the host */
	if (HIDTxHandleBusy(USBInHandle)) {
		/* hijack the pending read with the new error */
		TxBuffer[CH_BUFFER_OUTPUT_RETVAL] = CH_ERROR_INCOMPLETE_REQUEST;
		goto re_arm_rx;
	}

	/* got data, reset idle counter */
	idle_counter = 0;

	/* clear for debugging */
	memset (TxBuffer, 0xff, sizeof (TxBuffer));

	cmd = RxBuffer[CH_BUFFER_INPUT_CMD];
	switch(cmd) {
	case CH_CMD_GET_HARDWARE_VERSION:
		TxBuffer[CH_BUFFER_OUTPUT_DATA] = 0x04;
		break;
	case CH_CMD_GET_COLOR_SELECT:
		TxBuffer[CH_BUFFER_OUTPUT_DATA] = CHugGetColorSelect();
		break;
	case CH_CMD_SET_COLOR_SELECT:
		CHugSetColorSelect(RxBuffer[CH_BUFFER_INPUT_DATA]);
		break;
	case CH_CMD_GET_LEDS:
		TxBuffer[CH_BUFFER_OUTPUT_DATA] = CHugGetLEDs();
		break;
	case CH_CMD_SET_LEDS:
		CHugSetLEDs(RxBuffer[CH_BUFFER_INPUT_DATA + 0]);
		break;
	case CH_CMD_GET_MULTIPLIER:
		TxBuffer[CH_BUFFER_OUTPUT_DATA] = CHugGetMultiplier();
		break;
	case CH_CMD_SET_MULTIPLIER:
		CHugSetMultiplier(RxBuffer[CH_BUFFER_INPUT_DATA]);
		break;
	case CH_CMD_GET_INTEGRAL_TIME:
		memcpy (&TxBuffer[CH_BUFFER_OUTPUT_DATA],
			(void *) &SensorIntegralTime,
			2);
		break;
	case CH_CMD_SET_INTEGRAL_TIME:
		memcpy (&SensorIntegralTime,
			(const void *) &RxBuffer[CH_BUFFER_INPUT_DATA],
			2);
		break;
	case CH_CMD_GET_FIRMWARE_VERSION:
		memcpy (&TxBuffer[CH_BUFFER_OUTPUT_DATA],
			&FirmwareVersion,
			2 * 3);
		break;
	case CH_CMD_GET_SERIAL_NUMBER:
		reading = 0x0;
		memcpy (&TxBuffer[CH_BUFFER_OUTPUT_DATA],
			(const void *) &reading,
			4);
		break;
	case CH_CMD_TAKE_READING_RAW:
		/* take a single reading */
		reading = CHugTakeReadingRaw(SensorIntegralTime);
		memcpy (&TxBuffer[CH_BUFFER_OUTPUT_DATA],
			(const void *) &reading,
			sizeof(uint32_t));
		break;
	case CH_CMD_RESET:
		/* only reset when USB stack is not busy */
		idle_command = CH_CMD_RESET;
		break;
	case CH_CMD_SET_FLASH_SUCCESS:
		if (RxBuffer[CH_BUFFER_INPUT_DATA] != 0x01 &&
		    RxBuffer[CH_BUFFER_INPUT_DATA] != 0xff) {
			rc = CH_ERROR_INVALID_VALUE;
			break;
		}
		rc = CHugFlashErase(CH_EEPROM_ADDR_FLASH_SUCCESS,
				    CH_FLASH_ERASE_BLOCK_SIZE);
		if (rc != CH_ERROR_NONE)
			break;
		rc = CHugFlashWrite(CH_EEPROM_ADDR_FLASH_SUCCESS, 1,
				    &RxBuffer[CH_BUFFER_INPUT_DATA]);
		break;
	default:
		rc = CH_ERROR_UNKNOWN_CMD;
		break;
	}

	/* always send return code */
	if(!HIDTxHandleBusy(USBInHandle)) {
		TxBuffer[CH_BUFFER_OUTPUT_RETVAL] = rc;
		TxBuffer[CH_BUFFER_OUTPUT_CMD] = cmd;
		USBInHandle = HIDTxPacket(HID_EP,
					  (BYTE*)&TxBuffer[0],
					  CH_USB_HID_EP_SIZE);
	}
re_arm_rx:
	/* re-arm the OUT endpoint for the next packet */
	USBOutHandle = HIDRxPacket(HID_EP,
				   (BYTE*)&RxBuffer,
				   CH_USB_HID_EP_SIZE);
}

/**
 * USER_USB_CALLBACK_EVENT_HANDLER:
 * @event: the type of event
 * @pdata: pointer to the event data
 * @size: size of the event data
 *
 * This function is called from the USB stack to
 * notify a user application that a USB event
 * occured.  This callback is in interrupt context
 * when the USB_INTERRUPT option is selected.
 **/
BOOL
USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
	switch(event) {
	case EVENT_TRANSFER:
		break;
	case EVENT_SUSPEND:
		/* need to reduce power to < 2.5mA, so power down sensor */
		multiplier_old = CHugGetMultiplier();
		CHugSetMultiplier(CH_FREQ_SCALE_0);

		/* power down LEDs */
		CHugSetLEDs(0);
		break;
	case EVENT_RESUME:
		/* restore full power mode */
		CHugSetMultiplier(multiplier_old);
		break;
	case EVENT_CONFIGURED:
		/* enable the HID endpoint */
		USBEnableEndpoint(HID_EP,
				  USB_IN_ENABLED|
				  USB_OUT_ENABLED|
				  USB_HANDSHAKE_ENABLED|
				  USB_DISALLOW_SETUP);

		/* re-arm the OUT endpoint for the next packet */
		USBOutHandle = HIDRxPacket(HID_EP,
					   (BYTE*)&RxBuffer,
					   CH_USB_HID_EP_SIZE);
		break;
	case EVENT_EP0_REQUEST:
		USBCheckHIDRequest();
		break;
	case EVENT_TRANSFER_TERMINATED:
		break;
	default:
		break;
	}
	return TRUE;
}

/**
 * main:
 **/
void
main(void)
{
	uint8_t i;

	/* The USB module will be enabled if the bootloader has booted,
	 * so we soft-detach from the host. */
	if(UCONbits.USBEN == 1) {
		UCONbits.SUSPND = 0;
		UCON = 0;
		Delay10KTCYx(0xff);
	}

	/* set some defaults to power down the sensor */
	CHugSetLEDs(0);
	CHugSetColorSelect(CH_COLOR_SELECT_WHITE);
	CHugSetMultiplier(CH_FREQ_SCALE_0);

	/* Initializes USB module SFRs and firmware variables to known states */
	USBDeviceInit();
	USBDeviceAttach();

	/* do the welcome flash */
	for (i = 0; i < 3; i++) {
		CHugSetLEDs(1);
		Delay10KTCYx(0x5f);
		CHugSetLEDs(0);
		Delay10KTCYx(0x5f);
	}

	while(1) {

		/* clear watchdog */
		CLRWDT();

		/* check bus status and service USB interrupts */
		USBDeviceTasks();

		ProcessIO();
	}
}
