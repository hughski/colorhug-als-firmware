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
#include "ch-self-test.h"

#include <delays.h>
#include <USB/usb.h>
#include <USB/usb_common.h>
#include <USB/usb_device.h>
#include <USB/usb_function_hid.h>

/* configuration */
__CONFIG(FOSC_INTOSC & WDTE_SWDTEN & PWRTE_ON & MCLRE_OFF & CP_ON & BOREN_ON & CLKOUTEN_OFF & IESO_OFF & FCMEN_OFF);
__CONFIG(WRT_HALF & CPUDIV_NOCLKDIV & USBLSCLK_48MHz & PLLMULT_3x & PLLEN_ENABLED & STVREN_ON & BORV_LO & LPBOR_OFF & LVP_OFF);

/* ensure this is incremented on each released build */
static uint16_t	FirmwareVersion[3] = { 0, 2, 1 };

/* USB idle support */
static uint8_t idle_command = 0x00;
static uint8_t idle_counter = 0x00;

/* USB buffers */
uint8_t RxBuffer[CH_USB_HID_EP_SIZE];
uint8_t TxBuffer[CH_USB_HID_EP_SIZE];

USB_HANDLE	USBOutHandle = 0;
USB_HANDLE	USBInHandle = 0;

/**
 * ISRCode:
 **/
void interrupt
ISRCode (void)
{
	asm("ljmp 0x1004");
}

/**
 * CHugBootFlash:
 **/
static void
CHugBootFlash(void)
{
	asm("ljmp 0x1000");
}

/**
 * CHugCalculateChecksum:
 **/
static uint8_t
CHugCalculateChecksum(uint8_t *data, uint8_t length)
{
	int i;
	uint8_t checksum = 0xff;
	for (i = 0; i < length; i++)
		checksum ^= data[i];
	return checksum;
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
	case CH_CMD_BOOT_FLASH:
		CHugBootFlash();
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
	uint16_t erase_length;
	uint8_t length;
	uint8_t checksum;
	uint8_t cmd;
	uint8_t rc = CH_ERROR_NONE;
	static uint16_t led_counter = 0x0;

	/* reset the LED state */
	if (--led_counter == 0) {
		PORTCbits.RC5 ^= 1;
		led_counter = 0x2fff;
	}

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

	/* got data, reset idle counter */
	idle_counter = 0;

	/* clear for debugging */
	memset (TxBuffer, 0xff, sizeof (TxBuffer));

	cmd = RxBuffer[CH_BUFFER_INPUT_CMD];
	switch(cmd) {
	case CH_CMD_GET_HARDWARE_VERSION:
		TxBuffer[CH_BUFFER_OUTPUT_DATA] = 0x04;
		break;
	case CH_CMD_RESET:
		/* only reset when USB stack is not busy */
		idle_command = CH_CMD_RESET;
		break;
	case CH_CMD_GET_FIRMWARE_VERSION:
		memcpy (&TxBuffer[CH_BUFFER_OUTPUT_DATA],
			&FirmwareVersion,
			2 * 3);
		break;
	case CH_CMD_ERASE_FLASH:
		memcpy (&address,
			(const void *) &RxBuffer[CH_BUFFER_INPUT_DATA+0],
			2);
		/* allow to erase any address but not the bootloader */
		if (address < CH_EEPROM_ADDR_RUNCODE ||
		    address > CH_EEPROM_ADDR_MAX) {
			rc = CH_ERROR_INVALID_ADDRESS;
			break;
		}
		memcpy (&erase_length,
			(const void *) &RxBuffer[CH_BUFFER_INPUT_DATA+2],
			2);
		rc = CHugFlashErase(address, erase_length);
		break;
	case CH_CMD_READ_FLASH:
		/* allow to read any address */
		memcpy (&address,
			(const void *) &RxBuffer[CH_BUFFER_INPUT_DATA+0],
			2);
		length = RxBuffer[CH_BUFFER_INPUT_DATA+2];
		if (length > 60) {
			rc = CH_ERROR_INVALID_LENGTH;
			break;
		}
		rc = CHugFlashRead(address, length,
				   &TxBuffer[CH_BUFFER_OUTPUT_DATA+1]);
		checksum = CHugCalculateChecksum (&TxBuffer[CH_BUFFER_OUTPUT_DATA+1],
						  length);
		TxBuffer[CH_BUFFER_OUTPUT_DATA+0] = checksum;
		break;
	case CH_CMD_WRITE_FLASH:
		/* write to flash that's not the bootloader */
		memcpy (&address,
			(const void *) &RxBuffer[CH_BUFFER_INPUT_DATA+0],
			2);
		if (address < CH_EEPROM_ADDR_RUNCODE ||
		    address > CH_EEPROM_ADDR_MAX) {
			rc = CH_ERROR_INVALID_ADDRESS;
			break;
		}
		length = RxBuffer[CH_BUFFER_INPUT_DATA+2];
		if (length > CH_FLASH_TRANSFER_BLOCK_SIZE) {
			rc = CH_ERROR_INVALID_LENGTH;
			break;
		}
		checksum = CHugCalculateChecksum(&RxBuffer[CH_BUFFER_INPUT_DATA+4],
						 length);
		if (checksum != RxBuffer[CH_BUFFER_INPUT_DATA+3]) {
			rc = CH_ERROR_INVALID_CHECKSUM;
			break;
		}
		rc = CHugFlashWrite(address, length,
				    &RxBuffer[CH_BUFFER_INPUT_DATA+4]);
		break;
	case CH_CMD_BOOT_FLASH:
		/* only boot when USB stack is not busy */
		idle_command = CH_CMD_BOOT_FLASH;
		break;
	case CH_CMD_SET_FLASH_SUCCESS:
		if (RxBuffer[CH_BUFFER_INPUT_DATA] != 0x00) {
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
	case CH_CMD_SELF_TEST:
		rc = CHugSelfTest();
		break;
	default:
		rc = CH_ERROR_UNKNOWN_CMD_FOR_BOOTLOADER;
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
	uint16_t runcode_start = 0x3f3f;
	uint8_t flash_success = 0xff;

	/* switch ports to digital mode */
	ANSELA = 0x00;
	ANSELC = 0x00;

	/* set RA0 input-only (D+)
	 * set RA1 input-only (D-)
	 * set RA2 input (missing)
	 * set RA3 input-only (MCLR)
	 * set RA4 input (OUT)
	 * set RA5 output (S1)
	 * set RA6 input (n/a)
	 * set RA7 input (n/a) */
	TRISA = 0b11011111;

	/* set RC0 input (ICSPDAT)
	 * set RC1 input (ICSPCLK)
	 * set RC2 output (S3)
	 * set RC3 output (S0)
	 * set RC4 output (S2)
	 * set RC5 output (LED)
	 * set RC6 input (n/a)
	 * set RC7 input (n/a) */
	TRISC = 0b11000011;

	/* switch to 16Mhz operation */
	OSCCONbits.SCS = 0x0;
	OSCCONbits.IRCF = 0xf;

	/* wait for the PPL to lock */
	while (!OSCSTATbits.PLLRDY);

	/* stack overflow / underflow */
	if (PCONbits.STKUNF || PCONbits.STKOVF)
		CHugFatalError(CH_ERROR_OVERFLOW_STACK);

	/* the watchdog saved us from our doom */
	if (!PCONbits.nRWDT)
		CHugFatalError(CH_ERROR_WATCHDOG);

	/*
	 * Boot into the flashed program if all of these are true:
	 *  1. we didn't do soft-reset
	 *  2. the flashed program exists
	 *  3. the flash success is 0x01
	 */
	CHugFlashRead(CH_EEPROM_ADDR_RUNCODE, 2,
		      (uint8_t *) &runcode_start);
	CHugFlashRead(CH_EEPROM_ADDR_FLASH_SUCCESS, 1,
		      (uint8_t *) &flash_success);
	if (PCONbits.nRI &&
	    runcode_start != 0x3f3f &&
	    flash_success == 0x01)
		CHugBootFlash();

	/* Initializes USB module SFRs and firmware variables to known states */
	USBDeviceInit();
	USBDeviceAttach();

	while(1) {

		/* clear watchdog */
		CLRWDT();

		/* check bus status and service USB interrupts */
		USBDeviceTasks();

		ProcessIO();
	}
}
