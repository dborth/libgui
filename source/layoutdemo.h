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

//!A question asked in place: which control scheme to use. A GuiPopover with one button per scheme; the one the
//!player picks is chosen(), and clicking a button (or B, or clicking away) closes it. Read it as the sample of how to
//!use GuiPopover: build the panel in `ui`, mount() it beside the control that asked, attach `ui`.
class GuiSchemePopover : public GuiPopover
{
	public:
		static const int SCHEMES = 3;
		static const char * schemeName(int n);
		//!\param anchor The control it opens beside
		//!\param trigger Trigger the buttons click with (null for the default, A)
		//!\param screenWidth, screenHeight The screen it covers
		GuiSchemePopover(const GuiPopoverAnchor& anchor, GuiTrigger * trigger, int screenWidth, int screenHeight);
		//!The scheme that was picked, or -1 if it was closed without one
		int chosen() const { return picked; }
	private:
		int picked;
};

//!The whole screen: title bar, two selectable player cards, a devices panel, a grid of buttons and a status line.
class GuiLayoutDemo : public GuiColumn
{
	public:
		//!\param trigger Trigger the buttons and cards click with (null for the default, A)
		GuiLayoutDemo(GuiTrigger * trigger);
		GuiButton& backButton() { return *backBtn; }
		//!Opens the controller configuration screen
		GuiButton& configButton() { return *configBtn; }
		//!Asks which scheme to use: the page loop shows a GuiSchemePopover beside schemeAnchor() and calls setScheme()
		GuiButton& schemeButton() { return *schemeBtn; }
		GuiPopoverAnchor schemeAnchor();
		void setScheme(int n) { scheme = n; status = GuiFormat("Scheme: %s", GuiSchemePopover::schemeName(n)); }
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
		GuiTextButton * schemeBtn;
		int scheme;				// the index of the scheme in use
};

#endif
