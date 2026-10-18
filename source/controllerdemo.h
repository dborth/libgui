/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * controllerdemo.h
 * A controller configuration screen: a controller drawn from shapes, grouped
 * lists of its controls on either side and underneath, and a highlight that
 * follows the selected row. Described with the builder layer; no images.
 ***************************************************************************/
#ifndef _CONTROLLERDEMO_H_
#define _CONTROLLERDEMO_H_

#include <map>
#include "libgui/Gui.h"

class GuiControllerDemo : public GuiColumn
{
	public:
		//!\param trigger Trigger rows and buttons click with (null for the default, A)
		GuiControllerDemo(GuiTrigger * trigger);
		GuiButton& backButton() { return *backBtn; }
		//!The control highlighted on the controller (0 = none)
		int highlighted() const;
	private:
		enum class Side { LEFT, RIGHT, BOTTOM };

		GuiCard& listRow(GLYPH glyph, const char * letter, const char * name, int control, Side side, int width);

		UiScope ui;
		GuiControllerView * view;
		std::string status;
		std::map<int, int> targetOf;	// control -> index into the list of example targets
		GuiTextButton * backBtn;
};

#endif
