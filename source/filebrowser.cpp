/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 * filebrowser.cpp
 * Generic file routines - reading, writing, browsing
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <malloc.h>

#include "filebrowser.h"
#include "menu.h"

#include "drivers/ThreadDriver.h"
#include "drivers/Cond.h"
#include "drivers/Platform.h"
#include "drivers/FileSystemDriver.h"

#define THREAD_SLEEP 100
#define DEVICE_THREAD_STACKSIZE (32 * 1024)

BROWSERINFO browser;
BROWSERENTRY * browserList = nullptr; // list of files/folders in browser

char rootdir[128];
bool browserDeviceListChanged = false;

/****************************************************************************
 * Thread Synchronization Primitives
 ***************************************************************************/
static ThreadSync & DeviceSync() { static ThreadSync s; return s; }

// device thread state
static Thread deviceThread;
static volatile bool deviceCheckingHalt = true;
static bool deviceThreadStarted = false;
static bool deviceIdle = false; // protected by DeviceSync().mutex

/****************************************************************************
 * ResumeDeviceCheckingThread()
 * Signals the device thread to start, and resumes the thread.
 ***************************************************************************/
void ResumeDeviceCheckingThread()
{
	if(!deviceThreadStarted)
		return;

	DeviceSync().mutex.lock();
	deviceCheckingHalt = false;
	DeviceSync().workCond.signal();
	DeviceSync().mutex.unlock();
}

/****************************************************************************
 * HaltDeviceCheckingThread()
 * Signals the device thread to stop.
 ***************************************************************************/
void HaltDeviceCheckingThread()
{
	if(!deviceThreadStarted)
		return;

	deviceCheckingHalt = true;
	DeviceSync().mutex.lock();
	DeviceSync().workCond.signal(); // interrupt condvar sleep if the thread is in one
	while(!deviceIdle)
		DeviceSync().idleCond.wait(DeviceSync().mutex);
	DeviceSync().mutex.unlock();
}

/****************************************************************************
 * devicecallback()
 * Checks devices for hotplug changes (SD/USB removed or inserted)
 ***************************************************************************/
static void * devicecallback(void *)
{
	while (1)
	{
		int removed[MAX_STORAGE_DEVICES];
		int removedCount = 0;
		bool deviceListChanged = false;

		platform->getFileSystem()->pollStorageDevices(removed, removedCount, deviceListChanged);

		if(removedCount > 0 || deviceListChanged)
			browserDeviceListChanged = true; // signal the UI to refresh

		// sleep ~1 sec in 100us steps so we can react to a halt request quickly
		for(int i = 0; i < 10000 && !deviceCheckingHalt; i++)
			usleep(THREAD_SLEEP);

		// if halted, block here until ResumeDeviceCheckingThread wakes us
		if(deviceCheckingHalt)
		{
			DeviceSync().mutex.lock();
			deviceIdle = true;
			DeviceSync().idleCond.signal(); // tell HaltDeviceCheckingThread we've stopped
			while(deviceCheckingHalt)
				DeviceSync().workCond.wait(DeviceSync().mutex);
			deviceIdle = false;
			DeviceSync().mutex.unlock();
		}
	}
	return nullptr;
}

/****************************************************************************
 * InitDeviceCheckingThread()
 * Starts the device-checking background thread
 ***************************************************************************/
void InitDeviceCheckingThread()
{
	if(platform->getFileSystem()->hasRemovableStorageDevices())
	{
		DeviceSync();
		deviceThreadStarted = true;
		deviceThread.start(devicecallback, nullptr, DEVICE_THREAD_STACKSIZE, ThreadPriority::Low);
	}
}

/****************************************************************************
 * ResetBrowser()
 * Clears the file browser memory, and allocates one initial entry
 ***************************************************************************/
void ResetBrowser()
{
	browser.numEntries = 0;
	browser.selIndex = 0;
	browser.pageIndex = 0;

	// Clear any existing values
	if(browserList != nullptr)
	{
		free(browserList);
		browserList = nullptr;
	}
	// set aside space for 1 entry
	browserList = (BROWSERENTRY *)malloc(sizeof(BROWSERENTRY));
	memset(browserList, 0, sizeof(BROWSERENTRY));
}

/****************************************************************************
 * UpdateDirName()
 * Update curent directory name for file browser
 ***************************************************************************/
int UpdateDirName()
{
	int size=0;
	char * test;
	char temp[1024];

	/* current directory doesn't change */
	if (strcmp(browserList[browser.selIndex].filename,".") == 0)
	{
		return 0;
	}
	/* go up to parent directory or device list */
	else if (strcmp(browserList[browser.selIndex].filename,"..") == 0)
	{
		/* We are at the root of a device, drop back to the device list */
		if (strcmp(browser.dir, "/") == 0 || browser.dir[0] == '\0')
		{
			rootdir[0] = '\0';
			browser.dir[0] = '\0';
			return 1;
		}

		/* determine last subdirectory namelength */
		sprintf(temp,"%s",browser.dir);
		test = strtok(temp,"/");
		while (test != nullptr)
		{
			size = strlen(test);
			test = strtok(nullptr,"/");
		}

		/* remove last subdirectory name */
		size = strlen(browser.dir) - size - 1;
		browser.dir[size] = 0;

		/* if we stripped back to empty, force standard root */
		if(browser.dir[0] == '\0')
			strcpy(browser.dir, "/");

		return 1;
	}
	/* Open a directory or Device */
	else
	{
		/* If in device list, selection sets the new root */
		if (rootdir[0] == '\0')
		{
			strcpy(rootdir, browserList[browser.selIndex].filename);
			strcpy(browser.dir, "/");
			return 1;
		}

		/* test new directory namelength */
		if ((strlen(browser.dir)+1+strlen(browserList[browser.selIndex].filename)) < MAXPATHLEN)
		{
			if(strcmp(browser.dir, "/") != 0)
				strcat(browser.dir, "/");
			strcat(browser.dir, browserList[browser.selIndex].filename);
			return 1;
		}
		else
		{
			return -1;
		}
	}
}

/****************************************************************************
 * FileSortCallback
 *
 * Quick sort callback to sort file entries with the following order:
 *   .
 *   ..
 *   <dirs>
 *   <files>
 ***************************************************************************/
int FileSortCallback(const void *f1, const void *f2)
{
	/* Special case for implicit directories */
	if(((BROWSERENTRY *)f1)->filename[0] == '.' || ((BROWSERENTRY *)f2)->filename[0] == '.')
	{
		if(strcmp(((BROWSERENTRY *)f1)->filename, ".") == 0) { return -1; }
		if(strcmp(((BROWSERENTRY *)f2)->filename, ".") == 0) { return 1; }
		if(strcmp(((BROWSERENTRY *)f1)->filename, "..") == 0) { return -1; }
		if(strcmp(((BROWSERENTRY *)f2)->filename, "..") == 0) { return 1; }
	}

	/* If one is a file and one is a directory the directory is first. */
	if(((BROWSERENTRY *)f1)->isdir && !(((BROWSERENTRY *)f2)->isdir)) return -1;
	if(!(((BROWSERENTRY *)f1)->isdir) && ((BROWSERENTRY *)f2)->isdir) return 1;

	return strcasecmp(((BROWSERENTRY *)f1)->filename, ((BROWSERENTRY *)f2)->filename);
}

/***************************************************************************
 * ParseDeviceList
 * Generates the top level display of connected devices, sourced from the
 * platform's FileSystemDriver rather than a hardcoded path list - so this
 * automatically reflects whatever devices enumerateStorageDevices() (and
 * the device-checking thread's pollStorageDevices()) currently know about.
 **************************************************************************/
int ParseDeviceList()
{
	ResetBrowser();
	rootdir[0] = '\0';
	browser.dir[0] = '\0';

	StorageDevice devices[MAX_STORAGE_DEVICES];
	int deviceCount = platform->getFileSystem()->enumerateStorageDevices(devices);

	int entryNum = 0;

	for(int i = 0; i < deviceCount; i++)
	{
		MountResult result = platform->getFileSystem()->mountStorageDevice(devices[i].id);
		if(result != MountResult::Success)
			continue; // not currently reachable - leave it off the list rather than show a dead entry

		BROWSERENTRY * newBrowserList = (BROWSERENTRY *)realloc(browserList, (entryNum+1) * sizeof(BROWSERENTRY));
		if(!newBrowserList)
			break;

		browserList = newBrowserList;
		memset(&(browserList[entryNum]), 0, sizeof(BROWSERENTRY));

		strncpy(browserList[entryNum].filename, devices[i].prefix, MAXJOLIET);
		browserList[entryNum].filename[MAXJOLIET] = '\0';

		// Append the volume label when one is set
		if(devices[i].label[0] != '\0')
			snprintf(browserList[entryNum].displayname, MAXDISPLAY + 1, "%s (%s)", devices[i].name, devices[i].label);
		else
			strncpy(browserList[entryNum].displayname, devices[i].name, MAXDISPLAY);
		browserList[entryNum].displayname[MAXDISPLAY] = '\0';

		browserList[entryNum].isdir = 1;
		entryNum++;
	}

	browser.numEntries = entryNum;
	return entryNum;
}

/***************************************************************************
 * ParseDirectory
 **************************************************************************/
int ParseDirectory()
{
	// Route to device listing if rootdir was cleared by going up from a device root
	if(rootdir[0] == '\0')
		return ParseDeviceList();

	DIR *dir = nullptr;
	char fulldir[MAXPATHLEN];
	struct dirent *entry;

	// reset browser
	ResetBrowser();

	strcpy(fulldir, rootdir);

	// Safely construct the full path without doubling up on slashes
	if(strcmp(browser.dir, "/") == 0)
	{
		if(fulldir[strlen(fulldir)-1] != '/')
			strcat(fulldir, "/");
	}
	else
	{
		if(fulldir[strlen(fulldir)-1] == '/')
			fulldir[strlen(fulldir)-1] = '\0';
		strcat(fulldir, browser.dir);
	}

	dir = opendir(fulldir);

	// If a device becomes suddenly unavailable, fallback to the device list
	if (dir == nullptr)
	{
		return ParseDeviceList();
	}

	int entryNum = 0;

	// Inject "Up One Level"
	BROWSERENTRY * newBrowserList = (BROWSERENTRY *)realloc(browserList, (entryNum+1) * sizeof(BROWSERENTRY));
	if(newBrowserList)
	{
		browserList = newBrowserList;
		memset(&(browserList[entryNum]), 0, sizeof(BROWSERENTRY));
		strcpy(browserList[entryNum].filename, "..");
		strcpy(browserList[entryNum].displayname, "Up One Level");
		browserList[entryNum].isdir = 1;
		entryNum++;
	}

	while((entry = readdir(dir)))
	{
		if(strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0)
			continue;

		// Skip the filesystem's ".." if we already injected it at the root
		if(strcmp(entry->d_name,"..") == 0 && strcmp(browser.dir, "/") == 0)
			continue;

		BROWSERENTRY * newBrowserList = (BROWSERENTRY *)realloc(browserList, (entryNum+1) * sizeof(BROWSERENTRY));

		if(!newBrowserList) // failed to allocate required memory
		{
			ResetBrowser();
			entryNum = -1;
			break;
		}
		else
		{
			browserList = newBrowserList;
		}
		memset(&(browserList[entryNum]), 0, sizeof(BROWSERENTRY)); // clear the new entry

		memcpy(browserList[entryNum].filename, entry->d_name, MAXJOLIET);
		browserList[entryNum].filename[MAXJOLIET] = '\0';

		memcpy(browserList[entryNum].displayname, entry->d_name, MAXDISPLAY); // crop name for display
		browserList[entryNum].displayname[MAXDISPLAY] = '\0';

		if(entry->d_type==DT_DIR)
			browserList[entryNum].isdir = 1; // flag this as a dir

		entryNum++;
	}

	// close directory
	closedir(dir);

	// Sort the file list
	if(entryNum > 0)
		qsort(browserList, entryNum, sizeof(BROWSERENTRY), FileSortCallback);

	browser.numEntries = entryNum;
	return entryNum;
}

/****************************************************************************
 * BrowserChangeFolder
 *
 * Update current directory and set new entry list if directory has changed
 ***************************************************************************/
int BrowserChangeFolder()
{
	if(!UpdateDirName())
		return -1;

	ParseDirectory();

	return browser.numEntries;
}
