/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * GameCubePlatform.h
 ***************************************************************************/
#pragma once

#include "../Platform.h"
#include "OgcVideoDriver.h"
#include "OgcInputDriver.h"
#include "OgcThreadDriver.h"
#include "OgcAudioDriver.h"
#include "GameCubeFileSystemDriver.h"

class GameCubePlatform : public Platform
{
	public:
		GameCubePlatform() {}

		void init(int width, int height) override;
		void shutdown() override;

		SystemEvent getSystemEvent() override { return SystemEvent::None; }

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }
		ThreadDriver* getThread() override { return threadDriver; }

	private:
		OgcAudioDriver* audioDriver = nullptr;
		OgcVideoDriver* videoDriver = nullptr;
		OgcInputDriver* inputDriver = nullptr;
		GameCubeFileSystemDriver* fileSystemDriver = nullptr;
		OgcThreadDriver* threadDriver = nullptr;
};
