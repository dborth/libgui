/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * OgcDeviceTypes.h
 ***************************************************************************/
#pragma once

//!Storage device kinds recognized by the GC/Wii file system drivers.
enum {
	DEVICE_AUTO = 0,
	DEVICE_SD,
	DEVICE_USB,
	DEVICE_DVD,
	DEVICE_SMB,
	DEVICE_SD_SLOTA,   //!< GameCube memory card slot A
	DEVICE_SD_SLOTB,   //!< GameCube memory card slot B
	DEVICE_SD_PORT2,   //!< GameCube SD Gecko in memory card slot B
	DEVICE_SD_GCLOADER,
	DEVICE_LENGTH
};

//!devoptab mount prefix for each DEVICE_* type above, indexed the same way.
const char pathPrefix[DEVICE_LENGTH][11] =
{ "", "sd:/", "usb:/", "dvd:/", "smb:/", "carda:/", "cardb:/", "port2:/", "gcloader:/" };
