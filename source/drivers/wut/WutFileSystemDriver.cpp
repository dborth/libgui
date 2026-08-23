/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.cpp
 ***************************************************************************/
#include <whb/sdcard.h>

#include "WutFileSystemDriver.h"

void WutFileSystemDriver::init()
{
	WHBMountSdCard();
}

void WutFileSystemDriver::shutdown()
{
	WHBUnmountSdCard();
}
