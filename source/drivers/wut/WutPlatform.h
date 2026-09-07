/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutPlatform.h
 ***************************************************************************/
#pragma once

#include <whb/proc.h>
#include "../Platform.h"
#include "WutAudioDriver.h"
#include "WutVideoDriver.h"
#include "WutInputDriver.h"
#include "WutFileSystemDriver.h"
#include "WutThreadDriver.h"

//!Wii U Platform, wrapping WHBProcInit/WHBProcIsRunning to detect the OS
//!asking the app to exit (see getSystemEvent()).
class WutPlatform : public Platform
{
	public:
		WutPlatform() {}

		void init(int width, int height) override;
		void shutdown() override;
		SystemEvent getSystemEvent() override;

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }
		ThreadDriver* getThread() override { return threadDriver; }

	private:
		WutAudioDriver* audioDriver = nullptr;
		WutVideoDriver* videoDriver = nullptr;
		WutInputDriver* inputDriver = nullptr;
		WutFileSystemDriver* fileSystemDriver = nullptr;
		WutThreadDriver* threadDriver = nullptr;
};
