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

#ifndef __USB_DESCRIPTORS_C
#define __USB_DESCRIPTORS_C

#include <USB/usb.h>
#include <USB/usb_function_hid.h>

/* device descriptor, see usb_ch9.h */
ROM USB_DEVICE_DESCRIPTOR device_dsc=
{
	0x12,				/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_DEVICE,		/* DEVICE descriptor type */
	0x0200,				/* USB Spec Release Number (BCD format) */
	0x00,				/* Class Code */
	0x00,				/* Subclass */
	0x00,				/* Protocol */
	USB_EP0_BUFF_SIZE,		/* Max packet size for EP0, see usb_config.h */
	CH_USB_VID,			/* Vendor ID */
#ifdef COLORHUG_BOOTLOADER
	CH_USB_PID_BOOTLOADER,		/* Product ID */
#else
	CH_USB_PID_FIRMWARE,		/* Product ID */
#endif
	0x0002,				/* Device release number (BCD format) */
	0x01,				/* Manufacturer string index */
	0x02,				/* Product string index */
	0x00,				/* Device serial number string index */
	0x01				/* Number of possible configurations */
};

#ifdef COLORHUG_BOOTLOADER
/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]={
	/* Configuration Descriptor */
	0x09,			/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_CONFIGURATION,	/* CONFIGURATION descriptor type */
	0x32,0x00,			/* Total length of data */
	1,				/* Number of interfaces */
	1,				/* Index value of this configuration */
	0,				/* Configuration string index */
	_DEFAULT | _SELF,		/* Attributes (this device is self-powered, but has no remote wakeup), see usb_device.h */
	150,				/* Max power consumption (2X mA)

	/* Interface Descriptor */
	0x09,				/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_INTERFACE,	/* INTERFACE descriptor type */
	1,				/* Interface Number */
	0,				/* Alternate Setting Number */
	0,				/* Number of endpoints in this intf */
	0xff,				/* Class code */
	'F',				/* Subclass code */
	'W',				/* Protocol code */
	0x03,				/* Interface string index */

	/* Interface Descriptor */
	0x09,   			/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_INTERFACE,	/* INTERFACE descriptor type */
	0,				/* Interface Number */
	0,				/* Alternate Setting Number */
	2,				/* Number of endpoints in this intf */
	HID_INTF,			/* Class code */
	0,				/* Subclass code */
	0,				/* Protocol code */
	0,				/* Interface string index */

	/* HID Class-Specific Descriptor */
	0x09,				/* Size of this descriptor in bytes */
	DSC_HID,			/* HID descriptor type */
	0x11,0x01,			/* HID Spec Release Number (BCD format) */
	0x00,				/* Country Code (0x00 for Not supported) */
	HID_NUM_OF_DSC,			/* Number of class descriptors, see usbcfg.h */
	DSC_RPT,			/* Report descriptor type */
	HID_RPT01_SIZE,0x00,		/* Size of the report descriptor (with extra byte) */

	/* Endpoint Descriptor */
	0x07,
	USB_DESCRIPTOR_ENDPOINT,	/* Endpoint Descriptor */
	HID_EP | _EP_IN,		/* EndpointAddress */
	_INTERRUPT,			/* Attributes */
	0x40,0x00,			/* size (with extra byte) */
	0x01,				/* polling interval */

	/* Endpoint Descriptor */
	0x07,
	USB_DESCRIPTOR_ENDPOINT,	/* Endpoint Descriptor */
	HID_EP | _EP_OUT,		/* EndpointAddress */
	_INTERRUPT,			/* Attributes */
	0x40,0x00,			/* size (with extra byte) */
	0x01				/* polling interval */
};
#else
/* Configuration 1 Descriptor */
ROM BYTE configDescriptor1[]={
	/* Configuration Descriptor */
	0x09,			/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_CONFIGURATION,	/* CONFIGURATION descriptor type */
	0x34,0x00,			/* Total length of data */
	2,				/* Number of interfaces */
	1,				/* Index value of this configuration */
	0,				/* Configuration string index */
	_DEFAULT | _SELF,		/* Attributes (this device is self-powered, but has no remote wakeup), see usb_device.h */
	150,				/* Max power consumption (2X mA) */

	/* Interface Descriptor */
	0x09,				/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_INTERFACE,	/* INTERFACE descriptor type */
	1,				/* Interface Number */
	0,				/* Alternate Setting Number */
	0,				/* Number of endpoints in this intf */
	0xff,				/* Class code */
	'F',				/* Subclass code */
	'W',				/* Protocol code */
	0x03,				/* Interface string index */

	/* Interface Descriptor */
	0x09,				/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_INTERFACE,	/* INTERFACE descriptor type */
	2,				/* Interface Number */
	0,				/* Alternate Setting Number */
	0,				/* Number of endpoints in this intf */
	0xff,				/* Class code */
	'G',				/* Subclass code */
	'U',				/* Protocol code */
	0x04,				/* Interface string index */

	/* Interface Descriptor */
	0x09,   			/* Size of this descriptor in bytes */
	USB_DESCRIPTOR_INTERFACE,	/* INTERFACE descriptor type */
	0,				/* Interface Number */
	0,				/* Alternate Setting Number */
	1,				/* Number of endpoints in this intf */
	HID_INTF,			/* Class code */
	0,				/* Subclass code */
	0,				/* Protocol code */
	0,				/* Interface string index */

	/* HID Class-Specific Descriptor */
	0x09,				/* Size of this descriptor in bytes */
	DSC_HID,			/* HID descriptor type */
	0x11,0x01,			/* HID Spec Release Number (BCD format) */
	0x00,				/* Country Code (0x00 for Not supported) */
	HID_NUM_OF_DSC,			/* Number of class descriptors, see usbcfg.h */
	DSC_RPT,			/* Report descriptor type */
	HID_RPT01_SIZE & 0xff, HID_RPT01_SIZE >> 8,	/* Size of the report descriptor */

	/* Endpoint Descriptor */
	0x07,
	USB_DESCRIPTOR_ENDPOINT,	/* Endpoint Descriptor */
	HID_EP | _EP_IN,		/* EndpointAddress */
	_INTERRUPT,			/* Attributes */
	0x40,0x00,			/* size (with extra byte) */
	0x01,				/* polling interval */
};
#endif

/* Language code string descriptor */
static ROM struct
{
	BYTE bLength;
	BYTE bDscType;
	WORD string[1];
} sd000 =
{
	sizeof(sd000),
	USB_DESCRIPTOR_STRING,
	{0x0409}
};

/* Manufacturer string descriptor (unicode) */
static ROM struct
{
	BYTE bLength;
	BYTE bDscType;
	WORD string[12];
} sd001 =
{
	sizeof(sd001),
	USB_DESCRIPTOR_STRING,
	{'H','u','g','h','s','k','i',' ','L','t','d','.'}
};

/* Product string descriptor (unicode) */
#ifdef COLORHUG_BOOTLOADER
static ROM struct
{
	BYTE bLength;
	BYTE bDscType;
	WORD string[24];
} sd002 =
{
	sizeof(sd002),
	USB_DESCRIPTOR_STRING,
	{'C','o','l','o','r','H','u','g','A','L','S',' ',
	 '(','b','o','o','t','l','o','a','d','e','r',')'}
};
#else
static ROM struct
{
	BYTE bLength;
	BYTE bDscType;
	WORD string[11];
} sd002 =
{
	sizeof(sd002),
	USB_DESCRIPTOR_STRING,
	{'C','o','l','o','r','H','u','g','A','L','S'}
};
#endif

/* Firmware version string descriptor (unicode) */
ROM struct{BYTE bLength;BYTE bDscType;WORD string[5];}sd003={
sizeof(sd003),USB_DESCRIPTOR_STRING,
{
	0x30 + CH_VERSION_MAJOR,'.',
	0x30 + CH_VERSION_MINOR,'.',
	0x30 + CH_VERSION_MICRO
}};

/* Firmware GUID string descriptor (unicode) */
ROM struct{BYTE bLength;BYTE bDscType;WORD string[36];}sd004={
sizeof(sd004),USB_DESCRIPTOR_STRING,
{'8','4','f','4','0','4','6','4','-',
 '9','2','7','2','-',
 '4','e','f','7','-','9','3','9','9','-',
 'c','d','9','5','f','1','2','d','a','6','9','6'
}};

/* HID descriptor -- see http://www.usb.org/developers/hidpage#HID%20Descriptor%20Tool */
ROM struct
{
	BYTE report[HID_RPT01_SIZE];
} hid_rpt01 = {
{
#ifdef COLORHUG_BOOTLOADER
	0x06, 0x00, 0xFF,		/* Usage Page = 0xFF00 (Vendor Defined Page 1) */
	0x09, 0x01,			/* Usage (Vendor Usage 1) */
	0xA1, 0x01,			/* Collection (Application) */
	0x19, 0x01,			/* Usage Minimum */
	0x29, 0x40,			/* Usage Maximum -- 64 input usages total (0x01 to 0x40) */
	0x15, 0x00,			/* Logical Minimum (Vendor Usage = 0) */
	0x26, 0xFF, 0x00,		/* Logical Maximum (Vendor Usage = 255) */
	0x75, 0x08,			/* Report Size: 8-bit field size */
	0x95, 0x40,			/* Report Count: Make sixty-four 8-bit fields (the next time the parser hits an "Input", "Output", or "Feature" item) */
	0x81, 0x02,			/* Input (Data, Array, Abs): Instantiates input packet fields based on the above report size, count, logical min/max, and usage. */
	0x19, 0x01,			/* Usage Minimum */
	0x29, 0x40,			/* Usage Maximum - 64 output usages total (0x01 to 0x40) */
	0x91, 0x02,			/* Output (Data, Array, Abs): Instantiates output packet fields.  Uses same report size and count as "Input" fields, since nothing new/different was specified to the parser since the "Input" item. */
	0xC0				/* End Collection */
#else
	0x05, 0x20,			/* Usage Page (Sensor)				*/
	0x09, 0x01,			/* Usage (Sensor)				*/
	0xa1, 0x01,			/* Collection (Application)			*/
	0x09, 0x41,			/*  Usage (Light Ambient Light)			*/
	0xa1, 0x00,			/*  Collection (Physical)			*/
	0x85, 0x01,			/*   Report ID (1)				*/
	0x0a, 0x09, 0x03,		/*   Usage (Property: Sensor Connection Type)	*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x01,			/*   Logical Maximum (1)			*/
	0x75, 0x08,			/*   Report Size (8)				*/
	0x95, 0x01,			/*   Report Count (1)				*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x31, 0x08,		/*    Usage (Connection Type: PC Attached)	*/
	0x0a, 0x32, 0x08,		/*    Usage (Connection Type: PC External)	*/
	0xb1, 0x00,			/*    Feature (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0x16, 0x03,		/*   Usage (Property: Reporting State)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x02,			/*   Logical Maximum (2)			*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x40, 0x08,		/*    Usage (Reporting State: Report No Events)	*/
	0x0a, 0x41, 0x08,		/*    Usage (Reporting State: Report All Events)*/
	0x0a, 0x42, 0x08,		/*    Usage (Reporting State: Report Threshold Events)*/
	0xb1, 0x00,			/*    Feature (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0x19, 0x03,		/*   Usage (Property: Power State)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x02,			/*   Logical Maximum (2)			*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x50, 0x08,		/*    Usage (Power State: Undefined)		*/
	0x0a, 0x51, 0x08,		/*    Usage (Power State: D0 Full Power)	*/
	0x0a, 0x52, 0x08,		/*    Usage (Power State: D1 Low Power)		*/
	0xb1, 0x00,			/*    Feature (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0x01, 0x02,		/*   Usage (Event: Sensor State)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x06,			/*   Logical Maximum (6)			*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x00, 0x08,		/*    Usage (Sensor State: Undefined)		*/
	0x0a, 0x01, 0x08,		/*    Usage (Sensor State: Ready)		*/
	0x0a, 0x02, 0x08,		/*    Usage (Sensor State: Not Available)	*/
	0x0a, 0x03, 0x08,		/*    Usage (Sensor State: No Data Sel)		*/
	0x0a, 0x04, 0x08,		/*    Usage (Sensor State: Initializing)	*/
	0x0a, 0x05, 0x08,		/*    Usage (Sensor State: Access Denied)	*/
	0x0a, 0x06, 0x08,		/*    Usage (Sensor State: Error)		*/
	0xb1, 0x00,			/*    Feature (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0x0e, 0x03,		/*   Usage (Property: Report Interval)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x27, 0xff, 0xff, 0xff, 0xff,	/*   Logical Maximum (4294967295)		*/
	0x75, 0x20,			/*   Report Size (32)				*/
	0x55, 0x00,			/*   Unit Exponent (0)				*/
	0xb1, 0x02,			/*   Feature (Data,Var,Abs)			*/
	0x05, 0x20,			/*   Usage Page (Sensor)			*/
	0x0a, 0x01, 0x02,		/*   Usage (Event: Sensor State)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x05,			/*   Logical Maximum (5)			*/
	0x75, 0x08,			/*   Report Size (8)				*/
	0x95, 0x01,			/*   Report Count (1)				*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x00, 0x08,		/*    Usage (Sensor State: Undefined)		*/
	0x0a, 0x01, 0x08,		/*    Usage (Sensor State: Ready)		*/
	0x0a, 0x02, 0x08,		/*    Usage (Sensor State: Not Available)	*/
	0x0a, 0x03, 0x08,		/*    Usage (Sensor State: No Data Sel)		*/
	0x0a, 0x05, 0x08,		/*    Usage (Sensor State: Access Denied)	*/
	0x0a, 0x06, 0x08,		/*    Usage (Sensor State: Error)		*/
	0x81, 0x00,			/*    Input (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0x02, 0x02,		/*   Usage (Event: Sensor Event)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x25, 0x04,			/*   Logical Maximum (4)			*/
	0xa1, 0x02,			/*   Collection (Logical)			*/
	0x0a, 0x10, 0x08,		/*    Usage (Sensor Event: Unknown)		*/
	0x0a, 0x11, 0x08,		/*    Usage (Sensor Event: State Changed)	*/
	0x0a, 0x12, 0x08,		/*    Usage (Sensor Event: Property Changed)	*/
	0x0a, 0x13, 0x08,		/*    Usage (Sensor Event: Data Updated)	*/
	0x0a, 0x14, 0x08,		/*    Usage (Sensor Event: Poll Response)	*/
	0x81, 0x00,			/*    Input (Data,Arr,Abs)			*/
	0xc0,				/*   End Collection				*/
	0x0a, 0xd1, 0x04,		/*   Usage (Data Field: Illuminance)		*/
	0x15, 0x00,			/*   Logical Minimum (0)			*/
	0x26, 0xff, 0xff,		/*   Logical Maximum (65535)			*/
	0x75, 0x20,			/*   Report Size (32)				*/
	0x55, 0x0e,			/*   Unit Exponent (-2)				*/
	0x81, 0x02,			/*   Input (Data,Var,Abs)			*/
	0x06, 0xc0, 0xff,		/*   Usage Page (Vendor Usage Page 0xffc0)	*/
	0x09, 0x02,			/*   Usage (Vendor Usage 0x02)			*/
	0xa1, 0x01,			/*   Collection (Application)			*/
	0x85, 0x02,			/*    Report ID (2)				*/
	0x09, 0x01,			/*    Usage (Vendor Usage 0x01)			*/
	0x09, 0x0d,			/*    Usage (Vendor Usage 0x0d)			*/
	0x09, 0x03,			/*    Usage (Vendor Usage 0x03)			*/
	0x95, 0x03,			/*    Report Count (3)				*/
	0xb1, 0x22,			/*    Feature (Data,Var,Abs,NoPref)		*/
	0x09, 0x05,			/*    Usage (Vendor Usage 0x05)			*/
	0x75, 0x10,			/*    Report Size (16)				*/
	0x95, 0x01,			/*    Report Count (1)				*/
	0xb1, 0x22,			/*    Feature (Data,Var,Abs,NoPref)		*/
	0xc0,				/*   End Collection				*/
	0x09, 0x03,			/*   Usage (Vendor Usage 0x03)			*/
	0xa1, 0x01,			/*   Collection (Application)			*/
	0x85, 0x03,			/*    Report ID (3)				*/
	0x09, 0x30,			/*    Usage (Vendor Usage 0x30)			*/
	0x75, 0x08,			/*    Report Size (8)				*/
	0x95, 0x01,			/*    Report Count (1)				*/
	0xb1, 0x22,			/*    Feature (Data,Var,Abs,NoPref)		*/
	0x09, 0x07,			/*    Usage (Vendor Usage 0x07)			*/
	0x75, 0x30,			/*    Report Size (48)				*/
	0x95, 0x01,			/*    Report Count (1)				*/
	0xb1, 0x22,			/*    Feature (Data,Var,Abs,NoPref)		*/
	0x09, 0x0b,			/*    Usage (Vendor Usage 0x0b)			*/
	0x75, 0x20,			/*    Report Size (32)				*/
	0xb1, 0x22,			/*    Feature (Data,Var,Abs,NoPref)		*/
	0x09, 0x24,			/*    Usage (Vendor Usage 0x24)			*/
	0x09, 0x28,			/*    Usage (Vendor Usage 0x28)			*/
	0x75, 0x08,			/*    Report Size (8)				*/
	0x95, 0x02,			/*    Report Count (2)				*/
	0xc0,				/*   End Collection				*/
	0xc0,				/*  End Collection				*/
	0xc0,				/* End Collection				*/
#endif
}
};

/* only one configuration descriptor */
ROM BYTE *ROM USB_CD_Ptr[]=
{
	(ROM BYTE *ROM)&configDescriptor1
};

/* string descriptors */
ROM BYTE *ROM USB_SD_Ptr[]=
{
	(ROM BYTE *ROM)&sd000,
	(ROM BYTE *ROM)&sd001,
	(ROM BYTE *ROM)&sd002,
	(ROM BYTE *ROM)&sd003,
	(ROM BYTE *ROM)&sd004
};

#endif
