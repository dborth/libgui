/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * demo.cpp
 * Basic template/demonstration of libgui capabilities. For a
 * full-featured app using many more extensions, check out Snes9x GX.
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <fat.h>

#include "video.h"
#include "audio.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "demo.h"
#include "libgui/Gui.h"
#include "WiiGlyphRenderer.h"

struct SSettings Settings;
int ExitRequested = 0;

void ExitApp()
{
	ShutoffRumble();
	StopGX();
	exit(0);
}

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

	InitVideo(); // Initialize video
	SetupPads(); // Initialize input
	InitAudio(); // Initialize audio
	fatInitDefault(); // Initialize file system

	glyphRenderer = new WiiGlyphRenderer(GX_VTXFMT1);
	fontSystem = new GuiTextRenderer(font_ttf, font_ttf_size, glyphRenderer);
	textTranslator->loadLanguage(en_lang, en_lang_size);

	InitGUIThreads(); // Initialize GUI

	DefaultSettings();
	MainMenu(MENU_SETTINGS);
}
