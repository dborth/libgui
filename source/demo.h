/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 * demo.h
 ***************************************************************************/

#ifndef _DEMO_H_
#define _DEMO_H_

enum {
	METHOD_AUTO = 0,
	METHOD_SD,
	METHOD_USB,
	METHOD_NETWORK,
	METHOD_LENGTH
};

enum {
	AUTO_OFF = 0,
	AUTO_SOME,
	AUTO_ALL,
	AUTO_LENGTH
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
