/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * layoutdemo.h
 * The Controllers screen from the design mockups, described with the builder
 * layer (UiScope): no pixel coordinates, no new/delete, no per-widget helper
 * structs. Read layoutdemo.cpp top to bottom; it is the screen.
 ***************************************************************************/
#ifndef _LAYOUTDEMO_H_
#define _LAYOUTDEMO_H_

#include "libgui/Gui.h"

//!The whole screen: title bar, two selectable player cards, a devices panel, a grid of buttons and a status line.
class GuiLayoutDemo : public GuiColumn
{
	public:
		//!\param trigger Trigger the buttons and cards click with (null for the default, A)
		GuiLayoutDemo(GuiTrigger * trigger);
		GuiButton& backButton() { return *backBtn; }
		//!Opens the controller configuration screen
		GuiButton& configButton() { return *configBtn; }
	private:
		struct Player {
			const char * device;
			const char * role;
			const char * scheme;
			int battery;
		};

		GuiCard& playerCard(int n);
		int battery(int n) const;

		UiScope ui;				// owns every element below; declared first so it is destroyed last
		Player players[2];
		int frame;				// counts frames, so the battery figures visibly change
		int focused;			// the last card the selection visited (-1 = none yet)
		std::string status;		// shown on the bottom line
		GuiTextButton * backBtn;
		GuiTextButton * configBtn;
};

#endif
