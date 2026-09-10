/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
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

	this->logger = new Logger();
	this->logger->registerBackend(LOGGER_OSREPORT,	new OgcLoggerSysReport());
	this->logger->registerBackend(LOGGER_UDP,		new OgcLoggerUdp());
	this->logger->registerBackend(LOGGER_SERIAL,	new OgcLoggerUsbGecko());
	this->logger->registerBackend(LOGGER_FILE,		new LoggerFile());
	this->logger->init(LogConfig{});
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

	if (logger) {
		logger->shutdown();
		delete logger;
		logger = nullptr;
	}

	if (threadDriver) {
		threadDriver->shutdown();
		delete threadDriver;
		threadDriver = nullptr;
	}
}

/****************************************************************************
 * Shutdown/reset
 ***************************************************************************/

// Status::Exiting alone doesn't tell requestExit() what to do below
static bool hardwarePowerOffRequested = false;

void NotifyWiiShutdownRequested() { hardwarePowerOffRequested = true; platform->triggerExit(); }

// shutdown() only tears down the HAL drivers - it has no opinion on what
// should happen to the console afterward. requestExit() is where that
// decision actually gets made: if the event that brought us here was a
// real power button press (console or Wiimote), honor it with a proper
// IOS-mediated power-off via SYS_ResetSystem rather than falling through
// to a plain exit()
void WiiPlatform::requestExit()
{
	this->shutdown();

	if(hardwarePowerOffRequested)
		SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, FALSE);
	else
		exit(0);
}

// No reset callback is registered - SYS_ResetButtonDown() (polled in
// getSystemEvent() below) is a real libogc polling primitive, so there's
// nothing for a callback to add here.
SystemEvent WiiPlatform::getSystemEvent()
{
	if(platform->getStatus() == Status::Exiting)
		return SystemEvent::ShutdownRequested;

	static bool wasResetDown = false;
	bool isResetDown = SYS_ResetButtonDown();
	bool justPressed = isResetDown && !wasResetDown;
	wasResetDown = isResetDown;

	if(justPressed)
		return SystemEvent::ResetRequested;

	return SystemEvent::None;
}
