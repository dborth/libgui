/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * OgcSmbDriver.cpp
 ***************************************************************************/
#include <cstring>
#include <network.h>
#include <smb.h>

#include "OgcSmbDriver.h"

// libogc's tinysmb always registers under the fixed device name "smb"
static const char * const kSmbDeviceName = "smb";

void OgcSmbDriver::init()
{
	connected = false;
}

void OgcSmbDriver::shutdown()
{
	disconnect();
}

SmbConnectResult OgcSmbDriver::connect(const SmbShareInfo & info)
{
	if(connected)
		return SmbConnectResult::Success;

	if(info.host[0] == '\0' || info.share[0] == '\0')
		return SmbConnectResult::InvalidSettings;

	// Blocking network bring-up
	if(net_init() < 0)
		return SmbConnectResult::NetworkUnavailable;

	if(!smbInit(info.user, info.password, info.share, info.host))
		return SmbConnectResult::ConnectFailed;

	connected = true;
	return SmbConnectResult::Success;
}

void OgcSmbDriver::disconnect()
{
	if(!connected)
		return;

	smbClose(kSmbDeviceName);
	connected = false;
}

const char * OgcSmbDriver::connectResultMessage(SmbConnectResult result) const
{
	switch(result)
	{
		case SmbConnectResult::Success:            return "Connected.";
		case SmbConnectResult::InvalidSettings:    return "Network share host/name is blank.";
		case SmbConnectResult::NetworkUnavailable: return "Unable to initialize network!";
		case SmbConnectResult::ConnectFailed:      return "Failed to connect to network share.";
		default:                                   return "Unknown network share error.";
	}
}
