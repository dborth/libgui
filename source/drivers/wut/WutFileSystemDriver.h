/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.h
 ***************************************************************************/
#pragma once
#include "../FileSystemDriver.h"

class WutFileSystemDriver : public FileSystemDriver
{
	public:
		void init() override;
		void shutdown() override;
};
