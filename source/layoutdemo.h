/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 * layoutdemo.h
 * The Controllers screen from the design mockups, built with the layout
 * containers: no pixel coordinates except where the screen itself is placed.
 * Read the constructors top to bottom; each block is one piece of the screen.
 ***************************************************************************/
#ifndef _LAYOUTDEMO_H_
#define _LAYOUTDEMO_H_

#include "libgui/Gui.h"

//!A themed button with no image assets: a capsule GuiShape with a label centred on it.
class DemoButton
{
	public:
		DemoButton(const char * label, int w, int h, GuiTrigger * trigger, GuiSound * soundOver);
		GuiButton button;
	private:
		GuiShape shape;
		GuiText text;
		GuiText textLit;
};

//!One player's card: backdrop, a header row (number, device, battery) and a role chip with the scheme.
class DemoPlayerCard : public GuiOverlay
{
	public:
		DemoPlayerCard(const char * number, const char * device, const char * battery, const char * role, const char * scheme);
	private:
		GuiShape backdrop;
		GuiText numberTxt, deviceTxt, batteryTxt, roleTxt, schemeTxt;
		GuiShape chipBackdrop;
		GuiOverlay chip;
		GuiRow header, details;
		GuiColumn body;
};

//!The whole screen: title bar, two cards, a devices panel and a grid of buttons.
class GuiLayoutDemo : public GuiColumn
{
	public:
		GuiLayoutDemo(GuiTrigger * trigger, GuiSound * soundOver);
		GuiButton& backButton() { return backBtn.button; }
	private:
		GuiText titleTxt, schemeTxt;
		GuiRow titleBar;
		DemoPlayerCard card1, card2;
		GuiColumn cards;
		GuiShape stripBackdrop;
		GuiText keyboardTxt, mouseTxt, irTxt, touchTxt;
		GuiGrid stripGrid;
		GuiOverlay strip;
		DemoButton testBtn, mapBtn, resetBtn, identifyBtn, backBtn;
		GuiGrid buttons;
};

#endif
