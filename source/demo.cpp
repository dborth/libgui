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
#include "libgui/Gui.h"

struct SSettings Settings;
bool ExitRequested = false;

#ifdef __WIIU__
static WutPlatform platformInstance;
#else
static OgcPlatform platformInstance;
#endif
Platform* platform = &platformInstance;

void
DefaultSettings()
{
	Settings.LoadMethod = METHOD_AUTO;
	Settings.SaveMethod = METHOD_AUTO;
	sprintf (Settings.Folder1,"libgui/first folder");
	sprintf (Settings.Folder2,"libgui/second folder");
	sprintf (Settings.Folder3,"libgui/third folder");
	Settings.AutoLoad = 1;
	Settings.AutoSave = 1;
}

int
main(int, char **)
{
	platform->init();

	fontSystem = new GuiTextRenderer(font_ttf, font_ttf_size, platform->getVideo()->getGlyphRenderer());
	textTranslator = new GuiTextTranslator();
	textTranslator->loadLanguage(en_lang, en_lang_size);

	InitGUIThreads(); // Initialize GUI
	DefaultSettings();
	MainMenu(MENU_SETTINGS);

	platform->shutdown();
}
