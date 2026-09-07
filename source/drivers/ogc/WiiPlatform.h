/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * WiiPlatform.h
 ***************************************************************************/
#pragma once

#include <stdint.h>

#include "../Platform.h"
#include "OgcVideoDriver.h"
#include "OgcInputDriver.h"
#include "OgcThreadDriver.h"
#include "OgcAudioDriver.h"
#include "WiiFileSystemDriver.h"

void NotifyWiiShutdownRequested();

//!Wii Platform, built on the shared GC/Wii driver set (OgcVideoDriver,
//!OgcAudioDriver, OgcInputDriver, OgcThreadDriver) plus WiiFileSystemDriver
//!for hot-pluggable SD/USB/DVD.
class WiiPlatform : public Platform
{
	public:
		WiiPlatform() {}

		void init(int width, int height) override;
		void shutdown() override;

		SystemEvent getSystemEvent() override;
		Status getStatus() const override { return status; }
		void triggerExit() override { status = Status::Exiting; }

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }
		ThreadDriver* getThread() override { return threadDriver; }

	private:
		Status status = Status::Running;
		OgcAudioDriver* audioDriver = nullptr;
		OgcVideoDriver* videoDriver = nullptr;
		OgcInputDriver* inputDriver = nullptr;
		WiiFileSystemDriver* fileSystemDriver = nullptr;
		OgcThreadDriver* threadDriver = nullptr;
};
