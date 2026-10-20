/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiPopover.h
 *
 * A question asked in place: a panel that opens beside the control that asked
 * it, over the screen it came from.
 ***************************************************************************/
#pragma once

#include "GuiPopoverPlacement.h"

//!A panel that opens beside the control that asked, over the screen it came from, with a tail that points at the
//!control. It is a full-screen overlay: it places the panel (GuiPopoverPlace), draws the tail, and decides when the
//!popover is dismissed. It closes on B from any controller, or on a click from any controller whose pointer is
//!outside the panel; a click on the panel is the panel's own. What is on the panel, and how it looks, is up to the caller.
//!
//!A popover does not run itself. Add it on top of the screen it came from (disabling that screen so it dims and
//!stops listening), keep updating the window, and watch isDismissed(). To keep it open after all (a B that only
//!stopped something inside it), call reopen().
//!
//!Derive from it, build the panel in `ui`, mount() it, then attach `ui`:
//!\code
//!class Ask : public GuiPopover
//!{
//!	public:
//!		Ask(const GuiPopoverAnchor& anchor)
//!		{
//!			GuiCard& panel = ui.card().selectable(false).size(280, 160).add(ui.label("Sure?"));
//!			mount(panel, 280, 160, anchor, GuiPopoverInsets(60, 60, 20));
//!			ui.attachTo(*this);
//!		}
//!};
//!\endcode
//!\ingroup grp_layout
class GuiPopover : public GuiOverlay
{
	public:
		//!A popover as large as the screen the platform reports
		GuiPopover();
		//!A popover over a screen of `screenWidth` x `screenHeight`
		GuiPopover(int screenWidth, int screenHeight);
		//!Puts the built `panel` (a card of `panelW` x `panelH`, owned by `ui`) beside `anchor`, with the tail pointing at
		//!it, drawn in `tailColor` (the panel's face). `insets` is what to keep clear at the screen's edges.
		//!Call once, before attaching `ui`.
		void mount(GuiCard& panel, int panelW, int panelH, const GuiPopoverAnchor& anchor, const GuiPopoverInsets& insets, PixelColor tailColor);
		//!The same, with the tail in the theme's surfaceRaised
		void mount(GuiCard& panel, int panelW, int panelH, const GuiPopoverAnchor& anchor, const GuiPopoverInsets& insets);
		//!Dismisses the popover as B does (a close button on the panel calls this)
		void dismiss();
		//!Whether B, a click outside the panel or dismiss() has asked for it to close
		bool isDismissed() const { return dismissed; }
		//!The controller channel that dismissed it, or -1 for dismiss() or while it is open
		int dismissedBy() const { return by; }
		//!Keeps it open after all: clears the dismissal
		void reopen() { dismissed = false; by = -1; }
		//!Where the panel went (its top-left corner, in canvas pixels). Valid after mount().
		const GuiPopoverPosition& position() const { return placed; }
		//!Updates the panel first (a click on it is its own), then watches for the dismissal
		void update(InputController * c) override;
	protected:
		UiScope ui;	//!< Owns the panel and everything else the popover is made of; build the panel with it
	private:
		int screenW, screenH;
		GuiTrigger * cancel;	// B
		GuiTrigger * click;		// the primary press, for a click outside the panel
		GuiElement * panel;
		GuiPopoverPosition placed;
		bool dismissed;
		int by;
};
