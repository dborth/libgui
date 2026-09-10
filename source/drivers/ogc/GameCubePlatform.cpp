/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * GameCubePlatform.cpp
 ***************************************************************************/
#include "GameCubePlatform.h"

void GameCubePlatform::init(int width, int height)
{
	this->threadDriver = new OgcThreadDriver();
	this->threadDriver->init();

	this->videoDriver = new OgcVideoDriver();
	this->videoDriver->init(width, height);

	this->audioDriver = new OgcAudioDriver();
	this->audioDriver->init();

	this->inputDriver = new OgcInputDriver();
	this->inputDriver->init();

	this->fileSystemDriver = new GameCubeFileSystemDriver();
	this->fileSystemDriver->init();

	this->logger = new Logger();
	this->logger->registerBackend(LOGGER_OSREPORT,	new OgcLoggerSysReport());
	this->logger->registerBackend(LOGGER_SERIAL,	new OgcLoggerUsbGecko());
	this->logger->registerBackend(LOGGER_FILE,		new LoggerFile());

	LogConfig config;
	static const int deviceCandidates[] = { DEVICE_SD_PORT2 };
	const char * mountPath = FindFirstMountedPath(this->fileSystemDriver, deviceCandidates, 1);

	if(mountPath[0] != '\0') {
		// mountPath already ends in "/" (eg. "port2:/") - no separator needed.
		snprintf(config.filePath, sizeof(config.filePath), "%sdebug.log", mountPath);
	}

	this->logger->init(config);
}

void GameCubePlatform::shutdown()
{
	if (logger) {
		logger->shutdown();
		delete logger;
		logger = nullptr;
	}

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
}

// GameCube has no power-button/shutdown concept to honor (getSystemEvent()
// always reports None - see Platform.h) and no menu/loader distinction
// worth making here, so this is unconditional.
void GameCubePlatform::requestExit()
{
	this->shutdown();
	exit(0);
}
