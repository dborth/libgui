/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libgui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "drivers/ogc/OgcPlatform.h"
#include "menu.h"
#include "filelist.h"
#include "demo.h"
#include "libgui/Gui.h"

struct SSettings Settings;
bool ExitRequested = false;

static OgcPlatform ogcPlatformInstance;
Platform* platform = &ogcPlatformInstance;

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
main(int argc, char *argv[])
{
	(void)argc; (void)argv; // unused

	platform->init();

	fontSystem = new GuiTextRenderer(font_ttf, font_ttf_size, platform->getVideo()->getGlyphRenderer());
	textTranslator = new GuiTextTranslator();
	textTranslator->loadLanguage(en_lang, en_lang_size);

	InitGUIThreads(); // Initialize GUI
	DefaultSettings();
	MainMenu(MENU_SETTINGS);

	platform->shutdown();
}
