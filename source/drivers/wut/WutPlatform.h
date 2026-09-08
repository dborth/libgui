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
		WutPlatform() {}

		void init(int width, int height) override;
		void shutdown() override;
		void requestExit() override;
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
		WutAudioDriver* audioDriver = nullptr;
		WutVideoDriver* videoDriver = nullptr;
		WutInputDriver* inputDriver = nullptr;
		WutFileSystemDriver* fileSystemDriver = nullptr;
		WutThreadDriver* threadDriver = nullptr;
};
