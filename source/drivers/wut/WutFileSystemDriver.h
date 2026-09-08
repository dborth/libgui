/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.h
 ***************************************************************************/
#pragma once
#include "../FileSystemDriver.h"
#include <coreinit/filesystem_fsa.h>

//! Optional capacity/health telemetry for a single device, filled in on
//! request via getStorageMetrics(). Kept separate from the generic
//! FileSystemDriver contract (rather than, say, a virtual on the base)
//! since not every platform can supply it the same way; StorageDevice
//! carries a copy of this once enumerateStorageDevices() has queried it.
struct WutStorageMetrics
{
	uint64_t totalBytes;
	uint64_t freeBytes;
	uint32_t blockSize; //!< allocation unit / cluster size in bytes
	bool     readOnly;
};

//! State tracker for a single storage device slot. Wii U tracks exactly
//! two slots - DEVICE_SD (always present once WHBMountSdCard() succeeds)
//! and DEVICE_USB (empty, ie. prefix[0] == 0 / isPresent false, until
//! tryMountUsb() claims it) - one mount per device type, like every other
//! platform (see the Device enum in FileSystemDriver.h).
struct WutDeviceState
{
	int  id;
	char name[16];		//!< human-readable base name, eg. "SD Card" or derived from prefix (eg. "usb0")
	char label[16];		//!< volume label when we can read one via FSAGetVolumeInfo, empty otherwise
	char prefix[32];	//!< devoptab mount prefix, eg. "usb0:/" or the runtime SD path
	bool isPresent;		//!< found on the last poll (stat()-able)
	bool isMounted;
	bool unmountRequired;
};

//!Wii U FileSystemDriver.
//!
//!SD: WHBMountSdCard() - a runtime-assigned FSA path, not a static devoptab name
//!USB: stock Cafe OS has no FAT driver for USB at all so we use libmocha
//!
//!Hotplug: Mocha_usb_isInserted() only reports whether we already have the
//!fd open - it doesn't re-probe hardware - so it can't drive polling the
//!way __io_usbstorage.isInserted() does on GC/Wii. Instead: while
//!unmounted, pollStorageDevices() retries fatMountSimple() each cycle
//!(which does force a fresh /dev/usb0N open attempt); while mounted, it
//!reads one raw sector directly through the disc interface as a genuine
//!liveness check, since stat()-ing the mount root wouldn't necessarily
//!touch the hardware at all.
class WutFileSystemDriver : public FileSystemDriver
{
	public:
		void init() override;
		void shutdown() override;

		int enumerateStorageDevices(StorageDevice outDevices[MAX_STORAGE_DEVICES]) override;
		MountResult mountStorageDevice(int deviceId) override;
		const char * mountResultMessage(int deviceId, MountResult result) override;
		void invalidateStorageDevice(int deviceId) override;
		void pollStorageDevices(int removedIds[MAX_STORAGE_DEVICES], int & outRemovedCount, bool & deviceListChanged) override;
		bool hasRemovableStorageDevices() const override { return true; }

		const char * getMountPath(int device) const override;
		const int * getValidLoadDevices(int & outCount) const override;
		const int * getValidSaveDevices(int & outCount) const override;

		//! WUT-specific extension: fills outMetrics with capacity/health info
		//! for deviceId via statvfs(). Returns false if the device isn't
		//! currently present or statvfs() failed.
		bool getStorageMetrics(int deviceId, WutStorageMetrics & outMetrics);

	private:
		static const int kSlotSD  = 0;
		static const int kSlotUSB = 1;
		static const int kSlotCount = 2;

		WutDeviceState  m_devices[kSlotCount];
		int             m_deviceCount;
		FSAClientHandle m_fsaClient;  //!< used only for best-effort volume-label lookups; 0 if unavailable
		bool            m_mochaReady; //!< Mocha_InitLibrary() succeeded - USB unavailable entirely if not

		int  findDeviceIndex(int deviceId) const;
		void refreshDisplayName(WutDeviceState & dev);

		//! Attempts fatMountSimple("usb", &Mocha_usb_disc_interface). Updates
		//! m_devices[DEVICE_USB] and returns whether it's mounted afterwards.
		bool tryMountUsb();
		//! fatUnmount("usb") + Mocha_usb_disc_interface.shutdown(), so the
		//! next tryMountUsb() genuinely re-probes hardware rather than
		//! reusing a stale fd. Safe to call whether or not USB is mounted.
		void unmountUsb();
		//! Real liveness check for an already-mounted USB volume: reads one
		//! raw sector directly through Mocha_usb_disc_interface.
		bool usbStillPresent();
};
