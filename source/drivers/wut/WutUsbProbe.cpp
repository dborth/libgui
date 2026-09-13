/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutUsbProbe.cpp
 *
 * Intentionally does NOT touch Mocha_usbX_disc_interface
 ***************************************************************************/
#include <nsysuhs/uhs.h>
#include <nsysuhs/uhs_usbspec.h>
#include <malloc.h>
#include <string.h>

#include "WutUsbProbe.h"

static const uint32_t kMaxControllersToProbe = 4;
static const int32_t  kMaxProfilesPerController = 8;

static void ProbeController(uint32_t controllerNum, void * cfgBuffer, UsbHardwareSignature * outSig)
{
	UhsConfig config;
	memset(&config, 0, sizeof(config));
	config.controller_num = controllerNum;
	config.buffer = cfgBuffer;
	config.buffer_size = UHS_CONFIG_BUFFER_SIZE;

	UhsHandle handle;
	memset(&handle, 0, sizeof(handle));

	UHSStatus openStatus = UhsClientOpen(&handle, &config);

	if(openStatus != UHS_STATUS_OK)
		return;

	UhsInterfaceFilter filter;
	memset(&filter, 0, sizeof(filter));
	filter.match_params = MATCH_ANY; // no server-side filtering - filtered on if_desc.bInterfaceClass client-side below

	UhsInterfaceProfile * profiles = (UhsInterfaceProfile *)memalign(0x40, sizeof(UhsInterfaceProfile) * kMaxProfilesPerController);
	if(profiles)
	{
		memset(profiles, 0, sizeof(UhsInterfaceProfile) * kMaxProfilesPerController);

		UhsQueryInterfaces(&handle, &filter, profiles, kMaxProfilesPerController);

		for(int i = 0; i < kMaxProfilesPerController; i++)
		{
			if(profiles[i].if_handle == 0)
				continue;

			bool isStorage = (profiles[i].if_desc.bInterfaceClass == USBCLASS_STORAGE);

			if(isStorage && outSig && outSig->count < UsbHardwareSignature::kMaxInterfaces)
			{
				UsbHardwareInterfaceInfo & entry = outSig->interfaces[outSig->count++];
				entry.ifHandle = profiles[i].if_handle;
				entry.vid = profiles[i].dev_desc.idVendor;
				entry.pid = profiles[i].dev_desc.idProduct;
			}
		}

		free(profiles);
	}

	UhsClientClose(&handle);
}

void ScanUsbHardwareSignature(UsbHardwareSignature & out)
{
	out.count = 0;

	void * cfgBuffer = memalign(0x40, UHS_CONFIG_BUFFER_SIZE);
	if(!cfgBuffer)
		return;

	for(uint32_t controllerNum = 0; controllerNum < kMaxControllersToProbe; controllerNum++)
		ProbeController(controllerNum, cfgBuffer, &out);

	free(cfgBuffer);
}

bool UsbHardwareSignatureChanged(const UsbHardwareSignature & a, const UsbHardwareSignature & b)
{
	if(a.count != b.count)
		return true;

	for(int i = 0; i < a.count; i++)
	{
		bool foundInB = false;
		for(int j = 0; j < b.count; j++)
		{
			if(a.interfaces[i].ifHandle == b.interfaces[j].ifHandle)
			{
				foundInB = true;
				break;
			}
		}
		if(!foundInB)
			return true;
	}

	return false;
}
