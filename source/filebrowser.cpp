/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 *
 * filebrowser.cpp
 *
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

#ifdef __WIIU__
#include <whb/sdcard.h>
#endif

#define THREAD_SLEEP 100
#define DEVICE_THREAD_STACKSIZE (32 * 1024)

BROWSERINFO browser;
BROWSERENTRY * browserList = nullptr; // list of files/folders in browser

char rootdir[128];
bool browserDeviceListChanged = false;

/****************************************************************************
 * Thread Synchronization Primitives
 ***************************************************************************/
struct ThreadSync
{
	Mutex mutex;
	Cond  workCond; // main -> worker: wake/re-check available
	Cond  idleCond; // worker -> main: now idle/halted
};

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
 * InitDeviceThread()
 * Starts the device-checking background thread
 ***************************************************************************/
void InitDeviceThread()
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
	/* go up to parent directory */
	else if (strcmp(browserList[browser.selIndex].filename,"..") == 0)
	{
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

		return 1;
	}
	/* Open a directory */
	else
	{
		/* test new directory namelength */
		if ((strlen(browser.dir)+1+strlen(browserList[browser.selIndex].filename)) < MAXPATHLEN)
		{
			/* update current directory name */
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
 * Browse subdirectories
 **************************************************************************/
int
ParseDirectory()
{
	DIR *dir = nullptr;
	char fulldir[MAXPATHLEN];
	struct dirent *entry;

	// reset browser
	ResetBrowser();

	// add currentDevice to path
	strcpy(fulldir, rootdir);
	strcat(fulldir, browser.dir);
	// open the directory
	dir = opendir(fulldir);

	// if we can't open the dir, try opening the root dir
	if (dir == nullptr)
	{
		sprintf(browser.dir,"/");
		dir = opendir(rootdir);
		if (dir == nullptr)
		{
			return -1;
		}
	}

	// index files/folders
	int entryNum = 0;

	while((entry = readdir(dir)))
	{
		if(strcmp(entry->d_name,".") == 0)
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

		if(strcmp(entry->d_name,"..") == 0)
		{
			strcpy(browserList[entryNum].displayname, "Up One Level");
			browserList[entryNum].isdir = 1; // flag this as a dir
		}
		else
		{
			memcpy(browserList[entryNum].displayname, entry->d_name, MAXDISPLAY); // crop name for display
			browserList[entryNum].displayname[MAXDISPLAY] = '\0';

			if(entry->d_type==DT_DIR)
				browserList[entryNum].isdir = 1; // flag this as a dir
		}

		entryNum++;
	}

	// close directory
	closedir(dir);

	// Sort the file list
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

/****************************************************************************
 * BrowseDevice
 * Displays a list of files on the selected device
 ***************************************************************************/
int BrowseDevice()
{
	sprintf(browser.dir, "/");

#ifdef __WIIU__
	// Wii U has no "sd:/"-style libfat device name - WutFileSystemDriver
	// mounts the SD card at init() via WHBMountSdCard(), which assigns an
	// actual FS path (typically "/vol/external01/") only known at
	// runtime. "sd:/" (the Wii/GC libfat device name below) simply
	// doesn't exist as a devoptab entry on Wii U, so opendir() against it
	// would just fail - this pulls the real mounted path instead.
	const char * sdPath = WHBGetSdCardMountPath();
	if(sdPath && sdPath[0] != '\0')
		snprintf(rootdir, sizeof(rootdir), "%s", sdPath);
	else
		snprintf(rootdir, sizeof(rootdir), "/");
#else
	sprintf(rootdir, "sd:/");
#endif

	ParseDirectory(); // Parse root directory
	return browser.numEntries;
}
