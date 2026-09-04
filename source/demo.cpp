/****************************************************************************
 * libgui Template
 * Tantric 2009-2026
 *
 * demo.cpp
 * Basic template/demonstration of libgui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIIU__
#include "drivers/wut/WutPlatform.h"
#else
#include "drivers/ogc/OgcPlatform.h"
#endif
#include "menu.h"
#include "filelist.h"
#include "demo.h"
#include "filebrowser.h"
#include "libgui/Gui.h"

struct SSettings Settings;
bool ExitRequested = false;

#ifdef __WIIU__
static WutPlatform platformInstance;
#else
static OgcPlatform platformInstance;
#endif
Platform* platform = &platformInstance;

#define IMAGE_DECODE_SCRATCH_SIZE ((640 * 480 * 4) + (480 * sizeof(void*)))

void DefaultSettings()
{
	Settings.LoadMethod = METHOD_AUTO;
	Settings.SaveMethod = METHOD_AUTO;
	sprintf (Settings.Folder1,"libgui/first folder");
	sprintf (Settings.Folder2,"libgui/second folder");
	sprintf (Settings.Folder3,"libgui/third folder");
	Settings.AutoLoad = AUTO_SOME;
	Settings.AutoSave = AUTO_SOME;
}

int main(int, char **)
{
	platform->init(640, 480);

	void * decodeScratch = malloc(IMAGE_DECODE_SCRATCH_SIZE);
	GuiImageData::setDecodeScratch(decodeScratch, decodeScratch ? IMAGE_DECODE_SCRATCH_SIZE : 0);

	fontSystem = new GuiTextRenderer(font_ttf, font_ttf_size, platform->getVideo()->getGlyphRenderer());
	textTranslator = new GuiTextTranslator();
	textTranslator->loadLanguage(en_lang, en_lang_size);

	platform->getAudio()->start();

	DefaultSettings();
	InitDeviceCheckingThread();
	MainMenu(MENU_SETTINGS);

	platform->shutdown();
}
