/****************************************************************************
 * libgui Template
 * Tantric 2009
 *
 * demo.h
 ***************************************************************************/

#ifndef _DEMO_H_
#define _DEMO_H_

enum {
	METHOD_AUTO,
	METHOD_SD,
	METHOD_USB,
	METHOD_DVD,
	METHOD_SMB,
	METHOD_MC_SLOTA,
	METHOD_MC_SLOTB,
	METHOD_SD_SLOTA,
	METHOD_SD_SLOTB
};

struct SSettings {
    int		AutoLoad;
    int		AutoSave;
    int		LoadMethod;
	int		SaveMethod;
	char	Folder1[256]; // Path to files
	char	Folder2[256]; // Path to files
	char	Folder3[256]; // Path to files
};
extern struct SSettings Settings;

extern bool ExitRequested;

#endif
