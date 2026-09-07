/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutPlatform.cpp
 ***************************************************************************/
#include "WutPlatform.h"

void WutPlatform::init(int width, int height)
{
	WHBProcInit();

	this->threadDriver = new WutThreadDriver();
	this->threadDriver->init();

	this->videoDriver = new WutVideoDriver();
	this->videoDriver->init(width, height);

	this->audioDriver = new WutAudioDriver();
	this->audioDriver->init();

	this->inputDriver = new WutInputDriver();
	this->inputDriver->init();

	this->fileSystemDriver = new WutFileSystemDriver();
	this->fileSystemDriver->init();
}

void WutPlatform::shutdown()
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

//!Wii U has no separate reset signal - a lost foreground (OS asking us
//!to quit, via WHBProcIsRunning() going false) is the only system
//!event this platform can report, and it maps directly onto
//!ShutdownRequested. This is also the same signal video/audio guard
//!themselves against - see WutVideoDriver::render()/WutAudioDriver.
SystemEvent WutPlatform::getSystemEvent()
{
	return WHBProcIsRunning() ? SystemEvent::None : SystemEvent::ShutdownRequested;
}
