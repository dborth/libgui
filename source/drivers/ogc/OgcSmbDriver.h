/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * OgcSmbDriver.h
 *
 * GameCube + Wii SmbDriver (shared, like the rest of drivers/ogc/) built on
 * libogc's bundled tinysmb (<smb.h>). smbInit() registers its own "smb:/"
 * devoptab internally on success, so once connected the rest of the app
 * reads through it exactly like sd:/ or usb:/ - no other code in this
 * driver needs to speak SMB at all.
 ***************************************************************************/
#pragma once
#include "../SmbDriver.h"

//!Wraps libogc's tinysmb (smbInit/smbClose). Both GameCube (broadband
//!adapter) and Wii support this the same way - net_init() then smbInit()
class OgcSmbDriver : public SmbDriver
{
	public:
		void init() override;
		void shutdown() override;

		SmbConnectResult connect(const SmbShareInfo & info) override;
		void disconnect() override;
		bool isConnected() const override { return connected; }

		const char * getMountPath() const override { return connected ? "smb:/" : ""; }
		const char * connectResultMessage(SmbConnectResult result) const override;

	private:
		bool connected = false;
};
