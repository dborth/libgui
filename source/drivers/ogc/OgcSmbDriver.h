/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * OgcSmbDriver.h
 *
 * Wii / GameCube SmbDriver. libsmb2 is a plain client library with no
 * devoptab of its own: it hands back a struct smb2_context* and POSIX-shaped
 * calls. WutSmbDriver registers its own "smb:/" newlib devoptab on top of
 * libsmb2.
 ***************************************************************************/
#pragma once
#include "../SmbDriver.h"

struct smb2_context;

//!Wraps libsmb2 behind a "smb:/" devoptab. Both GameCube (broadband
//!adapter) and Wii bring the network up the same way via net_init(), then
//!share the identical libsmb2 connect/devoptab path.
class OgcSmbDriver : public SmbDriver
{
	public:
		void init() override;
		void shutdown() override;

		SmbConnectResult connect(const SmbShareInfo & info) override;
		void disconnect() override;
		bool isConnected() const override { return ctx != nullptr; }

		const char * getMountPath() const override { return ctx ? "smb:/" : ""; }
		const char * connectResultMessage(SmbConnectResult result) const override;
		const char * getLastError() const override;

		//! The active context, used by the free-function devoptab callbacks
		//! in OgcSmbDriver.cpp. Only one OgcSmbDriver/mount exists at a time.
		static smb2_context * getContext() { return ctx; }

	private:
		//! Brings the GameCube/Wii network interface up if it isn't already,
		//! via libogc's net_init(). Blocking. Returns false (with
		//! getLastError() set) if the call failed.
		bool ensureNetworkUp();

		static smb2_context * ctx;
		SmbShareInfo current = {};
		bool devoptabAdded = false;
		bool networkUp = false; //!< true once net_init() has succeeded at least once
};
