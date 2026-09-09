/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * GameCubePlatform.h
 ***************************************************************************/
#pragma once

#include "../Platform.h"
#include "OgcVideoDriver.h"
#include "OgcInputDriver.h"
#include "OgcThreadDriver.h"
#include "OgcAudioDriver.h"
#include "GameCubeFileSystemDriver.h"
#include "OgcLoggerSysReport.h"
#include "OgcLoggerUsbGecko.h"
#include "../LoggerSd.h"

//!GameCube (HW_DOL) Platform: same driver set as WiiPlatform except for
//!file system, which uses GameCubeFileSystemDriver (memory card/GC
//!Loader/DVD) instead of SD/USB/DVD.
class GameCubePlatform : public Platform
{
	public:
		GameCubePlatform() {}

		void init(int width, int height) override;
		void shutdown() override;
		void requestExit() override;

		SystemEvent getSystemEvent() override { return SystemEvent::None; }
		Status getStatus() const override { return status; }
		void triggerExit() override { status = Status::Exiting; }

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }
		ThreadDriver* getThread() override { return threadDriver; }
		Logger* getLogger() override { return logger; }

	private:
		Status status = Status::Running;
		OgcAudioDriver* audioDriver = nullptr;
		OgcVideoDriver* videoDriver = nullptr;
		OgcInputDriver* inputDriver = nullptr;
		GameCubeFileSystemDriver* fileSystemDriver = nullptr;
		OgcThreadDriver* threadDriver = nullptr;
		Logger* logger = nullptr;
};
