/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.cpp
 *
 * Wii U storage device enumeration + mounting: SD (via WHB) and
 * dynamically-probed FAT USB volumes.
 *
 * Unlike Wii/GameCube's DISC_INTERFACE, stock wut has no public API to
 * mount arbitrary FAT-formatted USB storage or to be notified of USB
 * hotplug events - the official Wii U USB storage path (nn::spm,
 * "/vol/storage_usb01") is for Nintendo-formatted game/save storage, not
 * plain FAT sticks. Reading a FAT USB drive from homebrew requires a
 * loader/CFW component (eg. Aroma, or a plugin like wafel_usb_partition)
 * to register a devoptab for it first. So on Wii U, "enumeration" means
 * polling the devoptab prefixes such tools are known to register.
 ***************************************************************************/
#include <whb/sdcard.h>
#include <sys/statvfs.h>
#include <string.h>
#include <stdio.h>

#include "WutFileSystemDriver.h"

// Candidate devoptab mount prefixes probed each poll for a newly-attached
// USB volume. If your loader/plugin registers something else, add it here.
static const char * const kUsbProbePrefixes[] = { "usb:/", "usb0:/", "usb1:/", "usb2:/", "usb3:/" };
static const int kUsbProbePrefixCount = sizeof(kUsbProbePrefixes) / sizeof(kUsbProbePrefixes[0]);

static bool DevicePresent(const char * prefix)
{
	struct statvfs st;
	return statvfs(prefix, &st) == 0;
}

void WutFileSystemDriver::init()
{
	memset(m_devices, 0, sizeof(m_devices));
	m_deviceCount = 0;

	FSAInit();
	m_fsaClient = FSAAddClient(nullptr); // best-effort; volume-label lookups just fall back if this is 0

	WHBMountSdCard();

	WutDeviceState & sd = m_devices[m_deviceCount++];
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
}

void WutFileSystemDriver::shutdown()
{
	WHBUnmountSdCard();

	if(m_fsaClient)
	{
		FSADelClient(m_fsaClient);
		m_fsaClient = 0;
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

int WutFileSystemDriver::allocateDeviceId()
{
	int newId = DEVICE_USB;
	bool used;
	do
	{
		used = false;
		for(int i = 0; i < m_deviceCount; i++)
		{
			if(m_devices[i].id == newId)
			{
				used = true;
				newId++;
				break;
			}
		}
	} while(used);
	return newId;
}

void WutFileSystemDriver::refreshDisplayName(WutDeviceState & dev)
{
	// Best-effort: FSAGetVolumeInfo only succeeds if dev.prefix genuinely
	// resolves through our own FSA client - true for the SD card (which we
	// mounted ourselves), not guaranteed for a devoptab a third-party
	// loader/plugin registered for USB. Fall back to a name derived from
	// the mount prefix either way, so `name` is never left empty.
	bool gotLabel = false;

	if(m_fsaClient)
	{
		FSAVolumeInfo volInfo;
		memset(&volInfo, 0, sizeof(volInfo));
		if(FSAGetVolumeInfo(m_fsaClient, dev.prefix, &volInfo) == FS_ERROR_OK && volInfo.volumeLabel[0] != '\0')
		{
			strncpy(dev.name, volInfo.volumeLabel, sizeof(dev.name) - 1);
			dev.name[sizeof(dev.name) - 1] = '\0';
			gotLabel = true;
		}
	}

	if(!gotLabel && dev.name[0] == '\0')
	{
		// Derive eg. "usb0" from "usb0:/" by stripping the trailing ":/"
		size_t len = strlen(dev.prefix);
		size_t copyLen = (len >= 2) ? len - 2 : len;
		if(copyLen >= sizeof(dev.name))
			copyLen = sizeof(dev.name) - 1;
		strncpy(dev.name, dev.prefix, copyLen);
		dev.name[copyLen] = '\0';
	}
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

	// 1. Re-verify every device we already know about (covers both
	//    removal and re-insertion of something we've seen before).
	for(int i = 0; i < m_deviceCount; i++)
	{
		WutDeviceState & dev = m_devices[i];
		bool present = DevicePresent(dev.prefix);

		if(dev.isPresent && !present)
		{
			dev.isPresent = false;
			dev.isMounted = false;
			dev.unmountRequired = true;

			if(outRemovedCount < MAX_STORAGE_DEVICES)
				removedIds[outRemovedCount++] = dev.id;
			deviceListChanged = true;
		}
		else if(!dev.isPresent && present)
		{
			dev.isPresent = true;
			refreshDisplayName(dev);
			deviceListChanged = true;
		}
	}

	// 2. Probe for USB volumes we haven't seen at all yet.
	for(int p = 0; p < kUsbProbePrefixCount && m_deviceCount < MAX_STORAGE_DEVICES; p++)
	{
		const char * prefix = kUsbProbePrefixes[p];

		bool alreadyTracked = false;
		for(int i = 0; i < m_deviceCount; i++)
		{
			if(strcmp(m_devices[i].prefix, prefix) == 0)
			{
				alreadyTracked = true;
				break;
			}
		}
		if(alreadyTracked || !DevicePresent(prefix))
			continue;

		WutDeviceState & dev = m_devices[m_deviceCount++];
		memset(&dev, 0, sizeof(dev));
		dev.id = allocateDeviceId();
		strncpy(dev.prefix, prefix, sizeof(dev.prefix) - 1);
		dev.isPresent = true;
		dev.isMounted = false;
		dev.unmountRequired = false;
		refreshDisplayName(dev);

		deviceListChanged = true;
	}
}

bool WutFileSystemDriver::getStorageMetrics(int deviceId, WutStorageMetrics & outMetrics)
{
	int idx = findDeviceIndex(deviceId);
	if(idx < 0 || !m_devices[idx].isPresent)
		return false;

	struct statvfs st;
	if(statvfs(m_devices[idx].prefix, &st) != 0)
		return false;

	outMetrics.totalBytes = (uint64_t)st.f_blocks * st.f_frsize;
	outMetrics.freeBytes  = (uint64_t)st.f_bavail * st.f_frsize;
	outMetrics.blockSize  = (uint32_t)st.f_frsize;
	outMetrics.readOnly   = (st.f_flag & ST_RDONLY) != 0;
	return true;
}
