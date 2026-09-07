/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
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
}

void GameCubePlatform::shutdown()
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
