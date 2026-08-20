/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiSound.cpp
 ***************************************************************************/

#include "Gui.h"

GuiSound::GuiSound(const uint8_t * s, int32_t l, SOUND t)
{
	sound = s;
	length = l;
	type = t;
	voice = -1;
	volume = 100;
	loop = false;
}

GuiSound::~GuiSound()
{
	if(type == SOUND::OGG)
		StopOgg();
}

void GuiSound::play()
{
	int vol;

	switch(type)
	{
		case SOUND::PCM:
		vol = 255*(volume/100.0);
		voice = ASND_GetFirstUnusedVoice();
		if(voice >= 0)
			ASND_SetVoice(voice, VOICE_STEREO_16BIT, 48000, 0,
				(uint8_t *)sound, length, vol, vol, nullptr);
		break;

		case SOUND::OGG:
		voice = 0;
		if(loop)
			PlayOgg((char *)sound, length, 0, OGG_INFINITE_TIME);
		else
			PlayOgg((char *)sound, length, 0, OGG_ONE_TIME);
		SetVolumeOgg(255*(volume/100.0));
		break;
	}
}

void GuiSound::stop()
{
	if(voice < 0)
		return;

	switch(type)
	{
		case SOUND::PCM:
		ASND_StopVoice(voice);
		break;

		case SOUND::OGG:
		StopOgg();
		break;
	}
}

void GuiSound::pause()
{
	if(voice < 0)
		return;

	switch(type)
	{
		case SOUND::PCM:
		ASND_PauseVoice(voice, 1);
		break;

		case SOUND::OGG:
		PauseOgg(1);
		break;
	}
}

void GuiSound::resume()
{
	if(voice < 0)
		return;

	switch(type)
	{
		case SOUND::PCM:
		ASND_PauseVoice(voice, 0);
		break;

		case SOUND::OGG:
		PauseOgg(0);
		break;
	}
}

bool GuiSound::isPlaying()
{
	if(ASND_StatusVoice(voice) == SND_WORKING || ASND_StatusVoice(voice) == SND_WAITING)
		return true;
	else
		return false;
}

void GuiSound::setVolume(int vol)
{
	volume = vol;

	if(voice < 0)
		return;

	int newvol = 255*(volume/100.0);

	switch(type)
	{
		case SOUND::PCM:
		ASND_ChangeVolumeVoice(voice, newvol, newvol);
		break;

		case SOUND::OGG:
		SetVolumeOgg(255*(volume/100.0));
		break;
	}
}

void GuiSound::setLoop(bool l)
{
	loop = l;
}
