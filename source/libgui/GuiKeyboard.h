#ifndef GUIKEYBOARD_H
#define GUIKEYBOARD_H

#include "Gui.h"

constexpr int KB_ROWS = 4;
constexpr int KB_COLUMNS = 11;

#define MAX_KEYBOARD_DISPLAY	32

typedef struct _keytype {
	char ch, chShift;
} Key;

//!On-screen keyboard
class GuiKeyboard : public GuiWindow
{
	public:
		GuiKeyboard(char * t, u32 m);
		~GuiKeyboard();
		void update(GuiTrigger * t);
		char kbtextstr[256];
	protected:
		u32 kbtextmaxlen;
		int shift;
		int caps;
		GuiText * kbText;
		GuiImage * keyTextboxImg;
		GuiText * keyCapsText;
		GuiImage * keyCapsImg;
		GuiImage * keyCapsOverImg;
		GuiButton * keyCaps;
		GuiText * keyShiftText;
		GuiImage * keyShiftImg;
		GuiImage * keyShiftOverImg;
		GuiButton * keyShift;
		GuiText * keyBackText;
		GuiImage * keyBackImg;
		GuiImage * keyBackOverImg;
		GuiButton * keyBack;
		GuiImage * keySpaceImg;
		GuiImage * keySpaceOverImg;
		GuiButton * keySpace;
		GuiButton * keyBtn[KB_ROWS][KB_COLUMNS];
		GuiImage * keyImg[KB_ROWS][KB_COLUMNS];
		GuiImage * keyImgOver[KB_ROWS][KB_COLUMNS];
		GuiText * keyTxt[KB_ROWS][KB_COLUMNS];
		GuiImageData * keyTextbox;
		GuiImageData * key;
		GuiImageData * keyOver;
		GuiImageData * keyMedium;
		GuiImageData * keyMediumOver;
		GuiImageData * keyLarge;
		GuiImageData * keyLargeOver;
		GuiSound * keySoundOver;
		GuiSound * keySoundClick;
		GuiTrigger * trigA;
		GuiTrigger * trig2;
		Key keys[KB_ROWS][KB_COLUMNS]; // two chars = less space than one pointer
};

#endif // GUIKEYBOARD_H
