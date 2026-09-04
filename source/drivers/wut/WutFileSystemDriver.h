/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.h
 ***************************************************************************/
#pragma once
#include "../FileSystemDriver.h"
#include <coreinit/filesystem_fsa.h>

enum {
	DEVICE_SD,
	DEVICE_USB, //!< first dynamically-allocated id for a detected USB volume;
	            //!< more are allocated as needed (see allocateDeviceId())
	DEVICE_LENGTH
};

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

//! State tracker for a single dynamically-probed storage device.
struct WutDeviceState
{
	int  id;
	char name[16];    //!< human-readable; volume label when we can read one, otherwise derived from prefix
	char prefix[32];   //!< devoptab mount prefix, eg. "usb0:/" or the runtime SD path
	bool isPresent;     //!< found on the last poll (stat()-able)
	bool isMounted;
	bool unmountRequired;
};

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

		//! WUT-specific extension: fills outMetrics with capacity/health info
		//! for deviceId via statvfs(). Returns false if the device isn't
		//! currently present or statvfs() failed.
		bool getStorageMetrics(int deviceId, WutStorageMetrics & outMetrics);

	private:
		WutDeviceState  m_devices[MAX_STORAGE_DEVICES];
		int             m_deviceCount;
		FSAClientHandle m_fsaClient; //!< used only for best-effort volume-label lookups; 0 if unavailable

		int  findDeviceIndex(int deviceId) const;
		int  allocateDeviceId();
		void refreshDisplayName(WutDeviceState & dev);
};
