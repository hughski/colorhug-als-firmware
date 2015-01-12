# Copyright (C) 2012 t-lo <thilo@thilo-fromm.de>
# Copyright (C) 2012 Richard Hughes <richard@hughsie.com>
#
# Licensed under the GNU General Public License Version 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
#
# The libraries package, "Microchip Application Libraries", is referenced here:
#  <http://www.microchip.com/stellent/idcplg?IdcService=SS_GET_PAGE&nodeId=2680&dDocName=en547784>
#
# The toolchain package, "MPLAB® C18 Lite Compiler for PIC18 MCUs", is
# referenced at this page :
#  <http://www.microchip.com/pagehandler/en-us/family/mplabx/>
#
# When editing this makefile please keep in mind that Microchip likes to put
# white spaces in both file names and directory names.

MICROCHIP_ROOT	= /opt/microchip
DOWNLOAD_DIR 	= $(shell pwd)/microchip-toolchain-downloads

MICROCHIP_TOOLCHAIN_ROOT = ${MICROCHIP_ROOT}/xc8/v1.33
TOOLCHAIN_URL = http://ww1.microchip.com/downloads/mplab/X/mplabc18-v3.40-linux-full-installer.run
TOOLCHAIN_INSTALLER   = ${DOWNLOAD_DIR}/mplabc18-v3.40-linux-full-installer.run
TOOLCHAIN_UNINSTALLER = ${MICROCHIP_TOOLCHAIN_ROOT}/UninstallMPLABC18v3.40

CC = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8
AS = ${MICROCHIP_TOOLCHAIN_ROOT}/mpasm/MPASMWIN
LD = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8
AR = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8s
COLORHUG_CMD = /usr/bin/colorhug-cmd

MICROCHIP_APP_LIB_ROOT 	 = ${MICROCHIP_ROOT}/libs-v2013-06-15

.DEFAULT_GOAL := all

# include toolchain headers and app lib includes into the build
CFLAGS  +=							\
	-I.				\
	-I$(MICROCHIP_TOOLCHAIN_ROOT)/h				\
	-I$(MICROCHIP_TOOLCHAIN_ROOT)/include/plib		\
	-I${MICROCHIP_APP_LIB_ROOT}/Microchip/Include		\
	--chip=16F1454						\
	-w3							\
	-nw=3004
firmware_CFLAGS = ${CFLAGS}
bootloader_CFLAGS = ${CFLAGS} -DCOLORHUG_BOOTLOADER

# include toolchain libraries into the build
bootloader_LDFLAGS +=						\
	-p16F1454						\
	-l ${MICROCHIP_TOOLCHAIN_ROOT}/lib			\
	-w							\
	-z__MPLAB_BUILD=1					\
	-u_CRUNTIME

firmware_LDFLAGS +=						\
	-p16F1454						\
	-l ${MICROCHIP_TOOLCHAIN_ROOT}/lib			\
	-w							\
	-v							\
	-z__MPLAB_BUILD=1					\
	-u_CRUNTIME

microchip_usbhid_SRC = ${MICROCHIP_APP_LIB_ROOT}/Microchip/USB/HID\ Device\ Driver/usb_function_hid.c
microchip_usbdev_SRC = ${MICROCHIP_APP_LIB_ROOT}/Microchip/USB/usb_device.c

firmware_OBJS =							\
	firmware.o						\
	ch-common.o						\
	ch-math.o						\
	usb_descriptors_firmware.o				\
	usb_device.o						\
	usb_function_hid.o

bootloader_OBJS =						\
	bootloader.o						\
	ch-common.o						\
	usb_descriptors_bootloader.o				\
	usb_device.o						\
	usb_function_hid.o

# Specific rules for sources from Microchip's application library.
# Treated specially since Microchip likes to put white spaces into its
# default application install paths.
usb_device.o: ${microchip_usbdev_SRC}
	${CC} --PASS1 ${CFLAGS} ${microchip_usbdev_SRC}
usb_function_hid.o: ${microchip_usbhid_SRC}
	${CC} --PASS1 ${CFLAGS} ${microchip_usbhid_SRC}

# common stuff
ch-common.o: Makefile ch-common.h ch-common.c
	$(CC) --PASS1 $(CFLAGS) ch-common.c
ch-math.o: Makefile ch-math.h ch-math.c
	$(CC) $(CFLAGS) ch-math.c
ch-sram.o: Makefile ch-sram.h ch-sram.c
	$(CC) $(CFLAGS) ch-sram.c
ch-temp.o: Makefile ch-temp.h ch-temp.c
	$(CC) $(CFLAGS) ch-temp.c

# bootloader
usb_descriptors_bootloader.o: Makefile usb_config.h usb_descriptors.c
	$(CC) $(bootloader_CFLAGS) usb_descriptors.c -fo $@
bootloader.o: Makefile ColorHug.h bootloader.c
	$(CC) $(bootloader_CFLAGS) bootloader.c
bootloader.cof bootloader.hex: Makefile ${bootloader_OBJS}
	$(LD) $(bootloader_LDFLAGS) ${bootloader_OBJS} -o $@

# firmware
usb_descriptors_firmware.o: Makefile usb_config.h usb_descriptors.c
	$(CC) $(firmware_CFLAGS) usb_descriptors.c -fo $@
firmware.o: Makefile ColorHug.h firmware.c
	$(CC) $(firmware_CFLAGS) firmware.c
firmware.cof firmware.hex: Makefile ${firmware_OBJS}
	$(LD) 18F46J50.lkr $(firmware_LDFLAGS) ${firmware_OBJS} -o $@

# Pad the HEX file into an easy-to-distribute BIN file
firmware.bin: firmware.hex $(COLORHUG_CMD)
	$(COLORHUG_CMD) inhx32-to-bin $< $@

all: firmware.bin bootloader.hex

install: firmware.bin
	${COLORHUG_CMD} flash-firmware-force firmware.bin

test: firmware.bin
	${COLORHUG_CMD} set-integral-time 15 && \
	${COLORHUG_CMD} take-reading-raw -v

clean:
	rm -f							\
	*.hex							\
	*.cof							\
	*.bin							\
	*.o
