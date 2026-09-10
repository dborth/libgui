/****************************************************************************
 * Platform Abstraction Layer (WUT driver)
 * Daryl Borth 2026
 * WutPlatform.cpp
 ***************************************************************************/
#include <stdlib.h>

#include "WutPlatform.h"

#include <sysapp/launch.h>
#include <proc_ui/procui.h>
#include <unistd.h>

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

	this->logger = new Logger();
	this->logger->registerBackend(LOGGER_OSREPORT,	new WutLoggerOSReport());
	this->logger->registerBackend(LOGGER_UDP,		new WutLoggerUdp());
	this->logger->registerBackend(LOGGER_SERIAL,	new WutLoggerUsbSerial());
	this->logger->registerBackend(LOGGER_FILE,		new LoggerFile());
	this->logger->init(LogConfig{});
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

	if(logger)
	{
		logger->shutdown();
		delete logger;
		logger = nullptr;
	}

	if(threadDriver)
	{
		threadDriver->shutdown();
		delete threadDriver;
		threadDriver = nullptr;
	}
}

// Once WHBProcShutdown() has run (in shutdown(), above), returning from
// the app is all that's needed - the OS reclaims the foreground on its
// own. There's no separate loader/power-off distinction to make here.
void WutPlatform::requestExit()
{
	// If the exit was user-initiated, Cafe OS has not been notified yet.
	// SYSLaunchMenu() tells Cafe OS to switch back to the system menu or loader.
	if(ProcUIIsRunning()) {
		SYSLaunchMenu();
		while (WHBProcIsRunning()) {
			usleep(1000);
		}
	}

	exit(0);
}

//! Polls Cafe OS process events. Transitions permanently to Exiting once
//! WHBProcIsRunning() returns false, and tracks Paused vs Running via ProcUIInForeground().
SystemEvent WutPlatform::getSystemEvent()
{
	// Once latched in Exiting, always return ShutdownRequested
	if (status == Status::Exiting)
		return SystemEvent::ShutdownRequested;

	// WHBProcIsRunning() pumps the ProcUI message queue - only call this once per frame
	if (!WHBProcIsRunning())
	{
		status = Status::Exiting;
		return SystemEvent::ShutdownRequested;
	}

	// Fast in-memory check for focus/foreground state
	if (!ProcUIInForeground())
	{
		status = Status::Paused;
	}
	else
	{
		status = Status::Running;
	}

	return SystemEvent::None;
}
