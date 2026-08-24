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

class WutPlatform : public Platform
{
	public:
		WutPlatform()
			: audioDriver(nullptr), videoDriver(nullptr), inputDriver(nullptr), fileSystemDriver(nullptr), threadDriver(nullptr) {}

		void init() override
		{
			WHBProcInit();

			this->threadDriver = new WutThreadDriver();
			this->threadDriver->init();

			this->videoDriver = new WutVideoDriver();
			this->videoDriver->init();

			this->audioDriver = new WutAudioDriver();
			this->audioDriver->init();

			this->inputDriver = new WutInputDriver();
			this->inputDriver->init();

			this->fileSystemDriver = new WutFileSystemDriver();
			this->fileSystemDriver->init();
		}

		void shutdown() override
		{
			if(fileSystemDriver)
			{
				fileSystemDriver->shutdown();
				delete fileSystemDriver;
				fileSystemDriver = nullptr;
			}

			if(inputDriver)
			{
				inputDriver->shutdown();
				delete inputDriver;
				inputDriver = nullptr;
			}

			if(audioDriver)
			{
				audioDriver->shutdown();
				delete audioDriver;
				audioDriver = nullptr;
			}

			if(videoDriver)
			{
				videoDriver->shutdown();
				delete videoDriver;
				videoDriver = nullptr;
			}

			if(threadDriver)
			{
				threadDriver->shutdown();
				delete threadDriver;
				threadDriver = nullptr;
			}

			WHBProcShutdown();
		}

		AudioDriver* getAudio() override { return audioDriver; }
		VideoDriver* getVideo() override { return videoDriver; }
		InputDriver* getInput() override { return inputDriver; }
		FileSystemDriver* getFileSystem() override { return fileSystemDriver; }
		ThreadDriver* getThread() override { return threadDriver; }

	private:
		WutAudioDriver* audioDriver;
		WutVideoDriver* videoDriver;
		WutInputDriver* inputDriver;
		WutFileSystemDriver* fileSystemDriver;
		WutThreadDriver* threadDriver;
};
