/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.cpp
 *
 * Wii U storage device enumeration + mounting: SD via WHB, USB via
 * libmocha's raw disc interface + libdvm (see dvm_wut.c/h).
 ***************************************************************************/
#include <whb/sdcard.h>
#include <mocha/mocha.h>
#include <mocha/disc_interface.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include "WutFileSystemDriver.h"
#include "dvm_wut.h"

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

	// USB (via Mocha_usb1/2_disc_interface) needs Mocha; SD (via WHB)
	// doesn't. If this fails - not booted under Aroma/compatible CFW - USB
	// just stays permanently absent rather than the whole driver failing.
	m_mochaReady = (Mocha_InitLibrary() == MOCHA_RESULT_SUCCESS);

	// Independent of Mocha - dvmWutInit() just registers libdvm's vfat/
	// exfat filesystem drivers, which don't touch hardware themselves.
	dvmWutInit();

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
	usb.prefix[0] = '\0'; // set once a physical slot actually mounts - see tryMountUsb()
	usb.isPresent = false;
	usb.isMounted = false;
	usb.unmountRequired = false;

	m_usbSlots[0] = { &Mocha_usb1_disc_interface, "usb1", false };
	m_usbSlots[1] = { &Mocha_usb2_disc_interface, "usb2", false };
	m_activeUsbSlot = -1;
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

	// unmountUsb() only touches whichever slot was actually mounted - a
	// slot that got poisoned but never became the active mount could still
	// theoretically hold an open fd (eg. app killed mid-probe). Cheap and
	// safe to call unconditionally; Mocha_usbN_shutdown() no-ops if the fd
	// isn't open.
	for(int i = 0; i < kUsbSlotCount; i++)
		if(m_usbSlots[i].iface)
			m_usbSlots[i].iface->shutdown();

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

bool WutFileSystemDriver::tryMountUsbSlot(int slotIndex)
{
	WutUsbPhysicalSlot & slot = m_usbSlots[slotIndex];

	if(slot.poisoned)
	{
		// Known-unmountable as of the last full probe. A bare startup()
		// call is a single cheap IOSU open, versus a real mount attempt's
		// open+read+signature-check pipeline, so this is the backoff:
		// don't repeat the expensive probe every poll cycle for a port
		// group we already know can't mount.
		if(slot.iface->isInserted())
			return false; // the fd we poisoned earlier is still open - nothing has changed here

		// fd isn't open (tryMountUsbSlot() shuts it down whenever it
		// poisons a slot, below) - see if anything is there right now.
		if(!slot.iface->startup())
			return false; // still nothing / still gone - cheap to recheck next cycle

		// The port group's fd state just changed - something was removed
		// and something (maybe the same device, maybe not) is there now.
		// Clear the poison and fall through to a real mount attempt.
		slot.poisoned = false;
	}

	// dvmWutMountUsb() takes a non-const DISC_INTERFACE* (matching
	// dvmDiscCreate()'s own signature upstream) but never mutates it -
	// only ever calls through its function pointers.
	if(dvmWutMountUsb(slot.mountName, (DISC_INTERFACE *) slot.iface, kUsbCachePages, kUsbSectorsPerPage))
		return true;

	// Either genuinely nothing in this port group (startup() itself failed
	// during the mount attempt) or something's there but unrecognized -
	// either way, stop paying for a full mount attempt every poll cycle.
	// dvmWutMountUsb() already shuts the interface down on failure, but
	// calling shutdown() again ourselves is a harmless no-op (Mocha checks
	// isInserted() first) - kept as a defensive belt-and-suspenders so the
	// isInserted()/startup() pair above is guaranteed to see a closed fd
	// regardless of exactly how the failure happened.
	slot.iface->shutdown();
	slot.poisoned = true;
	return false;
}

bool WutFileSystemDriver::tryMountUsb()
{
	WutDeviceState & usb = m_devices[kSlotUSB];

	if(usb.isMounted)
		return true;

	if(!m_mochaReady)
		return false;

	// Try every physical port group that isn't currently poisoned, in
	// order, stopping at the first that mounts. A device sitting in one
	// group - mountable or not - never hides the other one anymore.
	for(int i = 0; i < kUsbSlotCount; i++)
	{
		if(!tryMountUsbSlot(i))
			continue;

		m_activeUsbSlot = i;
		usb.isPresent = true;
		usb.isMounted = true;
		usb.unmountRequired = false;
		snprintf(usb.prefix, sizeof(usb.prefix), "%s:/", m_usbSlots[i].mountName);
		refreshDisplayName(usb);
		return true;
	}

	usb.isPresent = false;
	return false;
}

void WutFileSystemDriver::unmountUsb()
{
	WutDeviceState & usb = m_devices[kSlotUSB];

	if(m_activeUsbSlot >= 0)
	{
		WutUsbPhysicalSlot & slot = m_usbSlots[m_activeUsbSlot];

		// dvmWutUnmountUsb() drops the fat driver's reference on the
		// DvmDisc it was mounted through, which - once that's the last
		// reference - destroys the disc and calls slot.iface->shutdown()
		// itself (see dvm_wut.c and fat_driver.c's dvmDiscAddUser()/
		// dvmDiscRemoveUser() pairing). No separate shutdown() call needed
		// here the way the pre-dvm version needed one.
		if(usb.isMounted)
			dvmWutUnmountUsb(slot.mountName);

		// An app-driven unmount (eg. "safely remove" from a menu, or this
		// same slot going away and being re-detected) isn't evidence the
		// device is bad - don't carry poisoning across a clean unmount.
		slot.poisoned = false;
	}

	m_activeUsbSlot = -1;
	usb.isPresent = false;
	usb.isMounted = false;
	usb.unmountRequired = false;
}

bool WutFileSystemDriver::usbStillPresent()
{
	if(!m_mochaReady || m_activeUsbSlot < 0)
		return false;

	// Forces a genuine, uncached raw sector read through the active slot's
	// mounted disc - see dvmDiscProbePresence() in the libdvm fork. Unlike
	// stat()-ing the mount root (which libdvm's own sector cache, like
	// libfat's before it, can answer entirely from memory without ever
	// touching hardware again after mount), this always re-touches the
	// device, and does so under libdvm's own cache lock so it can't race
	// a concurrent file read/write on another thread.
	return dvmWutUsbStillPresent(m_usbSlots[m_activeUsbSlot].mountName);
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
