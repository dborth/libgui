/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutFileSystemDriver.cpp
 ***************************************************************************/
#include <whb/sdcard.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>

#include "WutFileSystemDriver.h"

void WutFileSystemDriver::init()
{
	WHBMountSdCard();

	memset(m_devices, 0, sizeof(m_devices));
	m_deviceCount = 0;

	// Pre-populate the primary SD slot and map root mount prefix to sdmc:/
	m_devices[m_deviceCount].id = DEVICE_SD;
	strcpy(m_devices[m_deviceCount].name, "sd");
	strcpy(m_devices[m_deviceCount].prefix, "sdmc:/");
	m_devices[m_deviceCount].isPresent = false;
	m_devices[m_deviceCount].isMounted = false;
	m_devices[m_deviceCount].unmountRequired = false;
	m_deviceCount++;
}

void WutFileSystemDriver::shutdown()
{
	WHBUnmountSdCard();

	memset(m_devices, 0, sizeof(m_devices));
	m_deviceCount = 0;
}

int WutFileSystemDriver::allocateUniqueId(int preferredId)
{
	int newId = preferredId;
	bool idUsed;
	do {
		idUsed = false;
		for (int i = 0; i < m_deviceCount; i++) {
			if (m_devices[i].id == newId) {
				idUsed = true;
				break;
			}
		}

		if (idUsed) {
			// Ensure we skip over static OGC IDs like DEVICE_DVD (3) or DEVICE_SMB (4)
			if (newId < DEVICE_LENGTH) {
				newId = DEVICE_LENGTH;
			} else {
				newId++;
			}
		}
	} while (idUsed);

	return newId;
}

int WutFileSystemDriver::enumerateStorageDevices(StorageDevice outDevices[MAX_STORAGE_DEVICES])
{
	int count = 0;
	for (int i = 0; i < m_deviceCount; i++) {
		if (m_devices[i].isPresent) {
			outDevices[count].id = m_devices[i].id;
			strcpy(outDevices[count].name, m_devices[i].name);
			strcpy(outDevices[count].prefix, m_devices[i].prefix);
			outDevices[count].removable = true;
			outDevices[count].autoMountAtStartup = true;
			count++;
		}
	}
	return count;
}

MountResult WutFileSystemDriver::mountStorageDevice(int deviceId)
{
	for (int i = 0; i < m_deviceCount; i++) {
		if (m_devices[i].id == deviceId) {
			if (m_devices[i].isMounted)
				return MountResult::Success;

			if (m_devices[i].unmountRequired) {
				// WUT devoptab drivers handle the underlying block I/O internally;
				// we just need to clear our local state lock.
				m_devices[i].unmountRequired = false;
				m_devices[i].isMounted = false;
			}

			// Verify if the mount point is actually accessible via stat()
			struct stat st;
			if (stat(m_devices[i].prefix, &st) == 0) {
				m_devices[i].isMounted = true;
				return MountResult::Success;
			} else {
				return MountResult::DeviceNotFound;
			}
		}
	}
	return MountResult::DeviceNotFound;
}

const char * WutFileSystemDriver::mountResultMessage(int deviceId, MountResult result)
{
	if (result == MountResult::MountFailed) {
		return "Unable to mount device.";
	}

	if (deviceId == DEVICE_SD) {
		return "SD card not found!";
	}

	// Handles dynamically mapped USBs whether they use DEVICE_USB or extended IDs
	return "USB drive not found!";
}

void WutFileSystemDriver::invalidateStorageDevice(int deviceId)
{
	for (int i = 0; i < m_deviceCount; i++) {
		if (m_devices[i].id == deviceId) {
			m_devices[i].isMounted = false;
			m_devices[i].unmountRequired = true;
			break;
		}
	}
}

void WutFileSystemDriver::pollStorageDevices(int removedIds[MAX_STORAGE_DEVICES], int & outRemovedCount, bool & deviceListChanged)
{
	outRemovedCount = 0;
	deviceListChanged = false;

	// 1. Verify all currently tracked devices
	for (int i = 0; i < m_deviceCount; i++) {
		struct stat st;
		bool currentlyPresent = (stat(m_devices[i].prefix, &st) == 0);

		if (m_devices[i].isPresent && !currentlyPresent) {
			// Device has been removed
			m_devices[i].isPresent = false;
			m_devices[i].isMounted = false;
			m_devices[i].unmountRequired = true;

			removedIds[outRemovedCount++] = m_devices[i].id;
			deviceListChanged = true;
		}
		else if (!m_devices[i].isPresent && currentlyPresent) {
			// Device has been re-inserted
			m_devices[i].isPresent = true;
			deviceListChanged = true;
		}
	}

	// 2. Dynamically probe the file system for newly inserted devices
	// Iterate through potential mount aliases
	const char* potentialUsbPrefixes[] = { "usb:/", "usb0:/", "usb1:/", "fat32:/" };

	for (const char* prefix : potentialUsbPrefixes) {
		struct stat st;
		if (stat(prefix, &st) == 0) {
			bool isTracked = false;
			for (int i = 0; i < m_deviceCount; i++) {
				if (strcmp(m_devices[i].prefix, prefix) == 0) {
					isTracked = true;
					break;
				}
			}

			// If the alias works and we haven't tracked it yet, register a new drive
			if (!isTracked && m_deviceCount < MAX_STORAGE_DEVICES) {
				int newId = allocateUniqueId(DEVICE_USB);

				m_devices[m_deviceCount].id = newId;

				// Extract a clean display name by stripping the trailing ":/"
				size_t prefixLen = strlen(prefix);
				strncpy(m_devices[m_deviceCount].name, prefix, prefixLen - 2);
				m_devices[m_deviceCount].name[prefixLen - 2] = '\0';

				strcpy(m_devices[m_deviceCount].prefix, prefix);
				m_devices[m_deviceCount].isPresent = true;
				m_devices[m_deviceCount].isMounted = false;
				m_devices[m_deviceCount].unmountRequired = false;

				m_deviceCount++;
				deviceListChanged = true;
			}
		}
	}
}
