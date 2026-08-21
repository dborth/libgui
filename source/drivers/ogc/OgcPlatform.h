/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * OgcPlatform.h
 ***************************************************************************/
#pragma once

#include "../Platform.h"
#include "OgcAudioDriver.h"
#include "OgcVideoDriver.h"
#include "OgcInputDriver.h"
#include "OgcFileSystemDriver.h"

class OgcPlatform : public Platform
{
	public:
		OgcPlatform()
			: audioDriver(nullptr), videoDriver(nullptr), inputDriver(nullptr), fileSystemDriver(nullptr) {}

		void init() override
		{
			this->videoDriver = new OgcVideoDriver();
			this->videoDriver->init();

			this->audioDriver = new OgcAudioDriver();
			this->audioDriver->init();

			this->inputDriver = new OgcInputDriver();
			this->inputDriver->init();

			this->fileSystemDriver = new OgcFileSystemDriver();
			this->fileSystemDriver->init();
		}

		void shutdown() override
		{
			if (fileSystemDriver) {
				fileSystemDriver->shutdown();
				delete fileSystemDriver;
				fileSystemDriver = nullptr;
			}

			if (inputDriver) {
				inputDriver->shutdown();
				delete inputDriver;
				inputDriver = nullptr;
			}

			if (audioDriver) {
				audioDriver->shutdown();
				delete audioDriver;
				audioDriver = nullptr;
			}

			if (videoDriver) {
				videoDriver->shutdown();
				delete videoDriver;
				videoDriver = nullptr;
			}
			exit(0);
		}

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }

	private:
		OgcAudioDriver* audioDriver;
		OgcVideoDriver* videoDriver;
		OgcInputDriver* inputDriver;
		OgcFileSystemDriver* fileSystemDriver;
};
