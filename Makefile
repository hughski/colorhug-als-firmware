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
PK2CMD_DIR 	= ../../pk2cmd/pk2cmd

MICROCHIP_TOOLCHAIN_ROOT = ${MICROCHIP_ROOT}/xc8/v1.34
TOOLCHAIN_URL = http://ww1.microchip.com/downloads/mplab/X/mplabc18-v3.40-linux-full-installer.run
TOOLCHAIN_INSTALLER   = ${DOWNLOAD_DIR}/mplabc18-v3.40-linux-full-installer.run
TOOLCHAIN_UNINSTALLER = ${MICROCHIP_TOOLCHAIN_ROOT}/UninstallMPLABC18v3.40
TOOLCHAIN_DIR = ${MICROCHIP_ROOT}/libs-v2013-06-15/Microchip/USB

CC = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8
AS = ${MICROCHIP_TOOLCHAIN_ROOT}/mpasm/MPASMWIN
LD = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8
AR = ${MICROCHIP_TOOLCHAIN_ROOT}/bin/xc8s
COLORHUG_CMD = /usr/bin/colorhug-cmd

MICROCHIP_APP_LIB_ROOT 	 = ${MICROCHIP_ROOT}/libs-v2013-06-15

.DEFAULT_GOAL := all

# include toolchain headers and app lib includes into the build
CFLAGS  +=							\
	-I.							\
	-I$(MICROCHIP_TOOLCHAIN_ROOT)/h				\
	-I$(MICROCHIP_TOOLCHAIN_ROOT)/include/plib		\
	-I${MICROCHIP_APP_LIB_ROOT}/Microchip/Include		\
	-I${MICROCHIP_APP_LIB_ROOT}/Microchip			\
	--chip=16F1454						\
	--asmlist						\
	--opt=+speed						\
	-w3							\
	-nw=3004

# 0000
#  | <--- Bootloader (crossing 2 pages)
# 0f9b
# 1f9c
#  | <--- Config space
# 1fff
# 1000
#  | <--- Firmware (crossing 2 pages)
# 1f9b
# 1f9c
#  | <--- Flash success
# 1fff
firmware_CFLAGS =						\
	${CFLAGS}						\
	--rom=1000-17ff,1800-1f9b				\
	--codeoffset=0x1000
bootloader_CFLAGS =						\
	${CFLAGS}						\
	-DCOLORHUG_BOOTLOADER					\
	--rom=0-07ff,800-0f9b

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

firmware_OBJS =							\
	firmware.p1						\
	d10ktcyx.p1						\
	ch-common.p1						\
	ch-flash.p1						\
	usb_descriptors_firmware.p1				\
	usb_device.p1						\
	usb_function_hid_firmware.p1

bootloader_OBJS =						\
	bootloader.p1						\
	d10ktcyx.p1						\
	ch-common.p1						\
	ch-flash.p1						\
	ch-self-test.p1						\
	usb_descriptors_bootloader.p1				\
	usb_device.p1						\
	usb_function_hid_bootloader.p1

# Specific rules for sources from Microchip's application library.
# Treated specially since Microchip likes to put white spaces into its
# default application install paths.
usb_device.p1: ${TOOLCHAIN_DIR}/usb_device.c
	${CC} --pass1 ${CFLAGS} ${TOOLCHAIN_DIR}/usb_device.c
usb_function_hid_bootloader.p1: ${TOOLCHAIN_DIR}/HID\ Device\ Driver/usb_function_hid.c
	${CC} --pass1 ${bootloader_CFLAGS} ${TOOLCHAIN_DIR}/HID\ Device\ Driver/usb_function_hid.c -o$@
usb_function_hid_firmware.p1: ${TOOLCHAIN_DIR}/HID\ Device\ Driver/usb_function_hid.c
	${CC} --pass1 ${firmware_CFLAGS} ${TOOLCHAIN_DIR}/HID\ Device\ Driver/usb_function_hid.c -o$@

# common stuff
d10ktcyx.p1: d10ktcyx.c Makefile
	$(CC) --pass1 $(CFLAGS) d10ktcyx.c -o$@
ch-common.p1: ch-common.c ch-common.h Makefile
	$(CC) --pass1 $(CFLAGS) ch-common.c -o$@
ch-self-test.p1: ch-self-test.c ch-self-test.h Makefile
	$(CC) --pass1 $(CFLAGS) ch-self-test.c -o$@
ch-flash.p1: Makefile ch-flash.h ch-flash.c
	$(CC) --pass1 $(CFLAGS) ch-flash.c -o$@
ch-sram.p1: Makefile ch-sram.h ch-sram.c
	$(CC) --pass1 $(CFLAGS) ch-sram.c -o$@
ch-temp.p1: Makefile ch-temp.h ch-temp.c
	$(CC) --pass1 $(CFLAGS) ch-temp.c -o$@

# bootloader
usb_descriptors_bootloader.p1: Makefile usb_config.h usb_descriptors.c
	$(CC) --pass1 $(bootloader_CFLAGS) usb_descriptors.c -o$@
bootloader.p1: Makefile ColorHug.h bootloader.c
	$(CC) --pass1 $(bootloader_CFLAGS) bootloader.c -o$@
bootloader.hex: Makefile ${bootloader_OBJS}
	$(CC) $(bootloader_CFLAGS) ${bootloader_OBJS} -o$@

# firmware
usb_descriptors_firmware.p1: Makefile usb_config.h usb_descriptors.c
	$(CC) --pass1 $(firmware_CFLAGS) usb_descriptors.c -o$@
firmware.p1: Makefile ColorHug.h firmware.c
	$(CC) --pass1 $(firmware_CFLAGS) firmware.c -o$@
firmware.hex: Makefile ${firmware_OBJS}
	$(CC) $(firmware_CFLAGS) ${firmware_OBJS} -o$@
#18F46J50.lkr

# Pad the HEX file into an easy-to-distribute BIN file
firmware.bin: firmware.hex $(COLORHUG_CMD)
	$(COLORHUG_CMD) inhx32-to-bin $< $@

all: firmware.bin bootloader.hex

install: firmware.bin
	${COLORHUG_CMD} flash-firmware-force firmware.bin -v

install-bootloader: bootloader.hex
	${PK2CMD_DIR}/pk2cmd -pPIC16F1454 -f $< -b ${PK2CMD_DIR}/ -m -r

erase:
	${PK2CMD_DIR}/pk2cmd -pPIC16F1454 -b ${PK2CMD_DIR}/ -e

test: firmware.bin
	${COLORHUG_CMD} set-integral-time 15 && \
	${COLORHUG_CMD} take-reading-raw -v

clean:
	rm -f *.as
	rm -f *.bin
	rm -f *.cmf
	rm -f *.cof
	rm -f *.d
	rm -f funclist
	rm -f *.hex
	rm -f *.hxl
	rm -f *.mum
	rm -f *.o
	rm -f *.obj
	rm -f *.p1
	rm -f *.pre
	rm -f *.rlf
	rm -f *.sdb
	rm -f *.sym
