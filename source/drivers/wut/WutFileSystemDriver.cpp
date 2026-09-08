/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.cpp
 *
 * Wii U storage device enumeration + mounting: SD via WHB, USB via
 * libmocha's raw disc interface + vendored libfat.
 ***************************************************************************/
#include <whb/sdcard.h>
#include <mocha/mocha.h>
#include <mocha/disc_interface.h>
#include <fat.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include "WutFileSystemDriver.h"

static bool DevicePresent(const char * prefix)
{
	struct stat st;
	return stat(prefix, &st) == 0;
}

void WutFileSystemDriver::init()
{
	memset(m_devices, 0, sizeof(m_devices));
	m_deviceCount = 0;

	FSAInit();
	m_fsaClient = FSAAddClient(nullptr); // best-effort; volume-label lookups just fall back if this is 0

	// USB (via Mocha_usb_disc_interface) needs Mocha; SD (via WHB) doesn't.
	// If this fails - not booted under Aroma/compatible CFW - USB just
	// stays permanently absent rather than the whole driver failing.
	m_mochaReady = (Mocha_InitLibrary() == MOCHA_RESULT_SUCCESS);

	WHBMountSdCard();

	WutDeviceState & sd = m_devices[kSlotSD];
	memset(&sd, 0, sizeof(sd));
	sd.id = DEVICE_SD;
	strcpy(sd.name, "SD Card");

	// The SD mount path is only known at runtime - WHBMountSdCard() picks
	// the real FS path (typically "/vol/external01") and only exposes it
	// through WHBGetSdCardMountPath(). There's no "sd:/"-style static
	// devoptab name on Wii U the way there is on Wii/GameCube.
	const char * sdPath = WHBGetSdCardMountPath();
	strncpy(sd.prefix, (sdPath && sdPath[0]) ? sdPath : "sdmc:/", sizeof(sd.prefix) - 1);
	sd.prefix[sizeof(sd.prefix) - 1] = '\0';

	sd.isPresent = DevicePresent(sd.prefix);
	sd.isMounted = false;
	sd.unmountRequired = false;
	refreshDisplayName(sd);

	WutDeviceState & usb = m_devices[kSlotUSB];
	memset(&usb, 0, sizeof(usb));
	usb.id = DEVICE_USB;
	strcpy(usb.name, "USB Storage");
	strcpy(usb.prefix, "usb:/");
	usb.isPresent = false;
	usb.isMounted = false;
	usb.unmountRequired = false;
	// Deliberately not attempting the USB mount here: the raw open probe
	// is a real IOSU IPC round trip, and init() runs on the main thread
	// during app startup - not the storage-checking thread. The first
	// pollStorageDevices() call (or an explicit mountStorageDevice(),
	// eg. from "autoMountAtStartup") picks it up from there.

	m_deviceCount = kSlotCount;
}

void WutFileSystemDriver::shutdown()
{
	unmountUsb();

	WHBUnmountSdCard();

	if(m_fsaClient)
	{
		FSADelClient(m_fsaClient);
		m_fsaClient = 0;
	}

	if(m_mochaReady)
	{
		Mocha_DeInitLibrary();
		m_mochaReady = false;
	}

	memset(m_devices, 0, sizeof(m_devices));
	m_deviceCount = 0;
}

int WutFileSystemDriver::findDeviceIndex(int deviceId) const
{
	for(int i = 0; i < m_deviceCount; i++)
		if(m_devices[i].id == deviceId)
			return i;
	return -1;
}

void WutFileSystemDriver::refreshDisplayName(WutDeviceState & dev)
{
	// Volume label, kept separate from `name`
	// Best-effort: FSAGetVolumeInfo only succeeds if dev.prefix genuinely
	// resolves through our own FSA client.
	dev.label[0] = '\0';

	if(m_fsaClient)
	{
		FSAVolumeInfo volInfo;
		memset(&volInfo, 0, sizeof(volInfo));
		if(FSAGetVolumeInfo(m_fsaClient, dev.prefix, &volInfo) == FS_ERROR_OK && volInfo.volumeLabel[0] != '\0')
		{
			strncpy(dev.label, volInfo.volumeLabel, sizeof(dev.label) - 1);
			dev.label[sizeof(dev.label) - 1] = '\0';
		}
	}
}

bool WutFileSystemDriver::tryMountUsb()
{
	WutDeviceState & usb = m_devices[kSlotUSB];

	if(usb.isMounted)
		return true;

	if(!m_mochaReady)
		return false;

	// fatMountSimple() calls disc->startup() internally, which is what
	// actually does the /dev/usb01 (falling back to /dev/usb02) raw open -
	// a genuine hardware probe each time this is called, not a cached
	// result. Safe to call repeatedly while unmounted.
	if(!fatMountSimple("usb", &Mocha_usb_disc_interface))
	{
		usb.isPresent = false;
		return false;
	}

	usb.isPresent = true;
	usb.isMounted = true;
	usb.unmountRequired = false;
	refreshDisplayName(usb);
	return true;
}

void WutFileSystemDriver::unmountUsb()
{
	WutDeviceState & usb = m_devices[kSlotUSB];

	if(usb.isMounted)
		fatUnmount("usb");

	// Reset Mocha's cached fds regardless of our own mount-state bookkeeping,
	// so the next tryMountUsb() forces a real re-probe rather than reusing
	// (or failing on) a stale handle from before a removal.
	if(m_mochaReady)
		Mocha_usb_disc_interface.shutdown();

	usb.isPresent = false;
	usb.isMounted = false;
	usb.unmountRequired = false;
}

bool WutFileSystemDriver::usbStillPresent()
{
	if(!m_mochaReady)
		return false;

	// Deliberately routed through the normal devoptab stat(), same as SD,
	// rather than calling Mocha_usb_disc_interface.readSectors() directly:
	// libfat's own internal reads go through that same underlying Mocha 
	// call, but under libfat's mutex_t lock.
	// Calling readSectors() directly here, from the storage-checking
	// thread, would race with those - libmocha has no locking of its own
	// around the shared fd. stat() goes through the same lock libfat's
	// other callers use, at the cost of occasionally being served from
	// libfat's cache rather than genuinely re-touching the hardware, so a
	// removal can take a poll cycle or two longer to surface than a true
	// hardware probe would.
	return DevicePresent(m_devices[kSlotUSB].prefix);
}

int WutFileSystemDriver::enumerateStorageDevices(StorageDevice outDevices[MAX_STORAGE_DEVICES])
{
	int count = 0;
	for(int i = 0; i < m_deviceCount && count < MAX_STORAGE_DEVICES; i++)
	{
		if(!m_devices[i].isPresent)
			continue;

		StorageDevice & out = outDevices[count];
		out.id = m_devices[i].id;
		strncpy(out.name, m_devices[i].name, sizeof(out.name) - 1);
		out.name[sizeof(out.name) - 1] = '\0';
		strncpy(out.label, m_devices[i].label, sizeof(out.label) - 1);
		out.label[sizeof(out.label) - 1] = '\0';
		strncpy(out.prefix, m_devices[i].prefix, sizeof(out.prefix) - 1);
		out.prefix[sizeof(out.prefix) - 1] = '\0';
		out.removable = true;
		out.autoMountAtStartup = true;

		WutStorageMetrics metrics;
		out.metricsValid = getStorageMetrics(m_devices[i].id, metrics);
		if(out.metricsValid)
		{
			out.totalBytes = metrics.totalBytes;
			out.freeBytes  = metrics.freeBytes;
			out.blockSize  = metrics.blockSize;
			out.readOnly   = metrics.readOnly;
		}
		else
		{
			out.totalBytes = 0;
			out.freeBytes  = 0;
			out.blockSize  = 0;
			out.readOnly   = false;
		}

		count++;
	}
	return count;
}

MountResult WutFileSystemDriver::mountStorageDevice(int deviceId)
{
	int idx = findDeviceIndex(deviceId);
	if(idx < 0)
		return MountResult::DeviceNotFound; // not ours

	if(deviceId == DEVICE_USB)
		return tryMountUsb() ? MountResult::Success : MountResult::DeviceNotFound;

	// SD (and anything else using the DevicePresent()-style devoptab check)
	WutDeviceState & dev = m_devices[idx];

	if(dev.isMounted)
		return MountResult::Success;

	if(dev.unmountRequired)
	{
		// wut's devoptab drivers own the underlying block I/O themselves;
		// we just need to drop our local mount-state lock and re-verify.
		dev.unmountRequired = false;
		dev.isMounted = false;
	}

	if(!DevicePresent(dev.prefix))
	{
		dev.isPresent = false;
		return MountResult::DeviceNotFound;
	}

	dev.isPresent = true;
	dev.isMounted = true;
	refreshDisplayName(dev);
	return MountResult::Success;
}

const char * WutFileSystemDriver::mountResultMessage(int deviceId, MountResult result)
{
	if(result == MountResult::MountFailed)
		return "Unable to mount device.";

	return (deviceId == DEVICE_SD) ? "SD card not found!" : "USB drive not found!";
}

void WutFileSystemDriver::invalidateStorageDevice(int deviceId)
{
	if(deviceId == DEVICE_USB)
	{
		unmountUsb();
		return;
	}

	int idx = findDeviceIndex(deviceId);
	if(idx < 0)
		return;

	m_devices[idx].isMounted = false;
	m_devices[idx].unmountRequired = true;
}

void WutFileSystemDriver::pollStorageDevices(int removedIds[MAX_STORAGE_DEVICES], int & outRemovedCount, bool & deviceListChanged)
{
	outRemovedCount = 0;
	deviceListChanged = false;

	// SD: re-verify via stat() on its devoptab prefix, same as before.
	{
		WutDeviceState & sd = m_devices[kSlotSD];
		bool present = DevicePresent(sd.prefix);

		if(sd.isPresent && !present)
		{
			sd.isPresent = false;
			sd.isMounted = false;
			sd.unmountRequired = true;

			if(outRemovedCount < MAX_STORAGE_DEVICES)
				removedIds[outRemovedCount++] = sd.id;
			deviceListChanged = true;
		}
		else if(!sd.isPresent && present)
		{
			sd.isPresent = true;
			refreshDisplayName(sd);
			deviceListChanged = true;
		}
	}

	// USB: no external devoptab to stat() - we own the mount ourselves
	{
		WutDeviceState & usb = m_devices[kSlotUSB];

		if(usb.isMounted)
		{
			if(!usbStillPresent())
			{
				unmountUsb();

				if(outRemovedCount < MAX_STORAGE_DEVICES)
					removedIds[outRemovedCount++] = DEVICE_USB;
				deviceListChanged = true;
			}
		}
		else if(tryMountUsb())
		{
			deviceListChanged = true;
		}
	}
}

bool WutFileSystemDriver::getStorageMetrics(int deviceId, WutStorageMetrics & outMetrics)
{
	int idx = findDeviceIndex(deviceId);
	if(idx < 0 || !m_devices[idx].isPresent)
		return false;

	bool haveMetrics = false;

	struct statvfs st;
	if(statvfs(m_devices[idx].prefix, &st) == 0)
	{
		outMetrics.totalBytes = (uint64_t)st.f_blocks * st.f_frsize;
		outMetrics.freeBytes  = (uint64_t)st.f_bavail * st.f_frsize;
		outMetrics.blockSize  = (uint32_t)st.f_frsize;
		haveMetrics = true;
		outMetrics.readOnly = (st.f_flag & ST_RDONLY) != 0;
	}
	else
	{
		outMetrics.readOnly = false; // no signal either way - default false rather than guess true
	}

	return haveMetrics;
}

const char * WutFileSystemDriver::getMountPath(int device) const
{
	int idx = findDeviceIndex(device);
	if(idx < 0 || m_devices[idx].prefix[0] == '\0')
		return "";
	return m_devices[idx].prefix;
}

const int * WutFileSystemDriver::getValidLoadDevices(int & outCount) const
{
	// No DEVICE_DVD (no optical drive) and no DEVICE_SMB (deliberately not
	// ported to Wii U for this pass).
	static const int devices[] = { DEVICE_AUTO, DEVICE_SD, DEVICE_USB };
	outCount = sizeof(devices) / sizeof(devices[0]);
	return devices;
}

const int * WutFileSystemDriver::getValidSaveDevices(int & outCount) const
{
	static const int devices[] = { DEVICE_AUTO, DEVICE_SD, DEVICE_USB };
	outCount = sizeof(devices) / sizeof(devices[0]);
	return devices;
}
