/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011-2015 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2015 Benjamin Tissoires <benjamin.tissoires@gmail.com>
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

#define CH_REPORT_HID_SENSOR			0x01
#define CH_REPORT_SENSOR_SETTINGS		0x02
#define CH_REPORT_SYSTEM_SETTINGS		0x03

typedef struct {
	uint8_t			 report_id;
	uint8_t			 sensor_state;
	uint8_t			 sensor_event;
	uint32_t		 illuminance;
} ChInputReport;

typedef struct {
	uint8_t			 report_id;
	uint8_t			 connection_type;
	uint8_t			 reporting_state;
	uint8_t			 power_state;
	uint8_t			 sensor_state;
	uint32_t		 report_interval;
} ChHidSensorFeature;

typedef struct {
	uint8_t			 report_id;
	uint8_t	 		 color_select;
	uint8_t			 LEDs_state;
	uint8_t			 multiplier;
	uint16_t		 integral_time;
} ChSensorSettings;

typedef struct {
	uint8_t			 report_id;
	uint8_t			 hardware_version;
	uint16_t		 version_major;
	uint16_t		 version_minor;
	uint16_t		 version_micro;
	uint32_t		 serial;
	uint8_t			 reset;
	uint8_t			 flash_flag;
} ChSystemSettings;

union ChFeatureReport {
	ChHidSensorFeature	 hid;
	ChSensorSettings	 sensor;
	ChSystemSettings	 system;
};

/**
 * ISRCode:
 **/
void interrupt
ISRCode(void)
{
}

static uint16_t		SensorIntegralTime = 0xffff;
static ChFreqScale	multiplier = CH_FREQ_SCALE_100;

/* this is used to map the firmware to a hardware version */
static const char flash_id[] = CH_FIRMWARE_ID_TOKEN;

/* USB idle support */
static uint8_t		idle_command = 0x00;
static uint16_t		idle_counter = 0x00;

/* USB buffers */
static ChInputReport TxBuffer;
static union ChFeatureReport TxFeature;
static union ChFeatureReport RxFeature;
USB_HANDLE USBInHandle = 0;

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
	uint32_t value;
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
			if (PORTAbits.RA4 == 1) {
				number_edges++;
				/* overflow */
				if (number_edges == 0)
					return UINT32_MAX;
			}
			ra_tmp = PORTA;
		}
	}

	/* scale it according to the datasheet */
	ra_tmp = CHugGetColorSelect();
	value = number_edges * abs_scale[ra_tmp];

	/* overflow */
	if (value < number_edges)
		return UINT32_MAX;

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
	/* User Application USB tasks */
	if ((USBDeviceState < CONFIGURED_STATE) ||
	    (USBSuspendControl == 1))
		return;

	/* prevent to send too many readings */
	if (idle_counter++ != 0x00)
		return;

	if(!HIDTxHandleBusy(USBInHandle)) {
		CHugSetMultiplier(multiplier);

		TxBuffer.report_id = CH_REPORT_HID_SENSOR;
		TxBuffer.sensor_state = CH_SENSOR_STATE_READY; // multiplier == CH_FREQ_SCALE_100 ? CH_SENSOR_STATE_READY : CH_SENSOR_STATE_NO_DATA_SEL;
		TxBuffer.sensor_event = CH_SENSOR_EVENT_DATA_UPDATED;
		TxBuffer.illuminance = CHugTakeReadingRaw(SensorIntegralTime);

		USBInHandle = HIDTxPacket(HID_EP,
					  (BYTE*)&TxBuffer,
					  sizeof(TxBuffer));
		CHugSetMultiplier(CH_FREQ_SCALE_0);
	} else {
		/* nobody reads */
		if (idle_command != 0x00) {
			CHugDeviceIdle();
			return;
		}
	}
}

/**
 * USER_USB_CALLBACK_EVENT_HANDLER:
 **/
BOOL
USER_USB_CALLBACK_EVENT_HANDLER(int event, void *pdata, WORD size)
{
	switch(event) {
	case EVENT_TRANSFER:
		break;
	case EVENT_SUSPEND:
		/* need to reduce power to < 2.5mA, so power down sensor */
		CHugSetMultiplier(CH_FREQ_SCALE_0);

		/* power down LEDs */
		CHugSetLEDs(0);
		break;
	case EVENT_RESUME:
		/* restore full power mode */
		break;
	case EVENT_CONFIGURED:
		/* enable the HID endpoint */
		USBEnableEndpoint(HID_EP,
				  USB_IN_ENABLED|
				  USB_HANDSHAKE_ENABLED|
				  USB_DISALLOW_SETUP);
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
 * CHugGetReportHandler:
 **/
void
CHugGetReportHandler(void)
{
	uint8_t bytes_to_send;

	/* handle only get_feature reports */
	if ((SetupPkt.wValue & 0xff00) != HID_FEATURE)
		return;

	/* clear for debugging */
	memset ((uint8_t*)&TxFeature, 0xff, sizeof (TxFeature));

	switch (SetupPkt.wValue & 0x00ff) {
		case CH_REPORT_HID_SENSOR:
			TxFeature.hid.report_id = CH_REPORT_HID_SENSOR;
			TxFeature.hid.connection_type = 0x01; /* PC External */
			TxFeature.hid.reporting_state = 0x02; /* Report All Events */
			TxFeature.hid.power_state = multiplier == CH_FREQ_SCALE_100 ?
						     0x01 : /* Full Power */
						     0x02; /* Low Power */
			TxFeature.hid.sensor_state = CH_SENSOR_STATE_READY;
			TxFeature.hid.report_interval = 1000;
			bytes_to_send = sizeof(ChHidSensorFeature);
			break;
		case CH_REPORT_SENSOR_SETTINGS:
			TxFeature.sensor.report_id = CH_REPORT_SENSOR_SETTINGS;
			TxFeature.sensor.color_select = CHugGetColorSelect();
			TxFeature.sensor.LEDs_state = CHugGetLEDs();
			TxFeature.sensor.multiplier = multiplier;
			TxFeature.sensor.integral_time = SensorIntegralTime;
			bytes_to_send = sizeof(ChSensorSettings);
			break;
		case CH_REPORT_SYSTEM_SETTINGS:
			TxFeature.system.report_id = CH_REPORT_SYSTEM_SETTINGS;
			TxFeature.system.hardware_version = 0x04;
			TxFeature.system.version_major = CH_VERSION_MAJOR;
			TxFeature.system.version_minor = CH_VERSION_MINOR;
			TxFeature.system.version_micro = CH_VERSION_MICRO;
			TxFeature.system.serial = 0x00;
			TxFeature.system.reset = 0x00; /* Reset - write only */
			TxFeature.system.flash_flag = 0x00; /* Flash Success - write only */
			bytes_to_send = sizeof(ChSystemSettings);
			break;
		default:
			return;
	}

	if (SetupPkt.wLength < bytes_to_send)
		bytes_to_send = SetupPkt.wLength;

	/* Now send the reponse packet data to the host, via the control transfer on EP0 */
	USBEP0SendRAMPtr((uint8_t*)&TxFeature, bytes_to_send, USB_EP0_RAM);
}

static uint8_t
CHugSetFlashSuccess(void)
{
	uint8_t rc;

	/* TODO check if we are not sending data */
	rc = CHugFlashErase(CH_EEPROM_ADDR_FLASH_SUCCESS,
			    CH_FLASH_ERASE_BLOCK_SIZE);
	if (rc != CH_ERROR_NONE)
		return rc;

	return CHugFlashWrite(CH_EEPROM_ADDR_FLASH_SUCCESS, 1,
			      &RxFeature.system.flash_flag);
}

/**
 * CHugSetReportComplete:
 *
 * Called when USBEP0Receive from SET_REPORT completes.
 **/
static void
CHugSetReportComplete(void)
{
	switch (SetupPkt.wValue & 0x00ff) {
		case CH_REPORT_HID_SENSOR:
			if (RxFeature.hid.power_state) {
				multiplier = RxFeature.hid.power_state == 0x01 ?
						CH_FREQ_SCALE_100 : CH_FREQ_SCALE_0;
			}
			break;
		case CH_REPORT_SENSOR_SETTINGS:
			CHugSetColorSelect(RxFeature.sensor.color_select);
			CHugSetLEDs(RxFeature.sensor.LEDs_state);
			multiplier = RxFeature.sensor.multiplier;
			SensorIntegralTime = RxFeature.sensor.integral_time;
			break;
		case CH_REPORT_SYSTEM_SETTINGS:
			if (RxFeature.system.flash_flag)
				CHugSetFlashSuccess();

			/* the reset flag has to be checked last */
			if (RxFeature.system.reset)
				idle_command = CH_CMD_RESET;
			break;
	}
}

/**
 * CHugSetReportHandler:
 **/
void
CHugSetReportHandler(void)
{
	uint8_t bytesToReceive;

	/* handle only set_feature reports */
	if ((SetupPkt.wValue & 0xff00) != HID_FEATURE)
		return;

	switch (SetupPkt.wValue & 0x00ff) {
		case CH_REPORT_HID_SENSOR:
		case CH_REPORT_SENSOR_SETTINGS:
		case CH_REPORT_SYSTEM_SETTINGS:
			break;
		default:
			return;
	}

	if (SetupPkt.wLength > sizeof(RxFeature))
		bytesToReceive = sizeof(RxFeature);
	else
		bytesToReceive = SetupPkt.wLength;

	/* Prepare EP0 to receive the control transfer data */
	USBEP0Receive((BYTE*)&RxFeature,
		      bytesToReceive,
		      CHugSetReportComplete);
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

	/* convince the compiler it's actually used */
	if (flash_id[0] == '\0')
		CHugFatalError(CH_ERROR_WRONG_UNLOCK_CODE);

	while(1) {

		/* clear watchdog */
		CLRWDT();

		/* check bus status and service USB interrupts */
		USBDeviceTasks();

		ProcessIO();
	}
}
