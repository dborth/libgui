/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * WiiPlatform.cpp
 ***************************************************************************/
#include "WiiPlatform.h"

void WiiPlatform::init(int width, int height)
{
	this->threadDriver = new OgcThreadDriver();
	this->threadDriver->init();

	this->videoDriver = new OgcVideoDriver();
	this->videoDriver->init(width, height);

	this->audioDriver = new OgcAudioDriver();
	this->audioDriver->init();

	this->inputDriver = new OgcInputDriver();
	this->inputDriver->init();

	this->fileSystemDriver = new WiiFileSystemDriver();
	this->fileSystemDriver->init();
}

void WiiPlatform::shutdown()
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

	if (threadDriver) {
		threadDriver->shutdown();
		delete threadDriver;
		threadDriver = nullptr;
	}
	
	exit(0);
}

/****************************************************************************
 * Shutdown/reset
 ***************************************************************************/
static bool shutdownRequestedFlag = false;

void NotifyWiiShutdownRequested() { shutdownRequestedFlag = true; }

// No reset callback is registered - SYS_ResetButtonDown() (polled in
// getSystemEvent() below) is a real libogc polling primitive, so there's
// nothing for a callback to add here.
SystemEvent WiiPlatform::getSystemEvent()
{
	if(shutdownRequestedFlag)
		return SystemEvent::ShutdownRequested;

	static bool wasResetDown = false;
	bool isResetDown = SYS_ResetButtonDown();
	bool justPressed = isResetDown && !wasResetDown;
	wasResetDown = isResetDown;

	if(justPressed)
		return SystemEvent::ResetRequested;

	return SystemEvent::None;
}
