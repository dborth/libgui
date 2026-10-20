/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiPopoverPlacement.h
 *
 * Where a popover goes, and the tail that points at what asked for it. Pure
 * arithmetic on rectangles: no platform, no elements, so it can be tested on
 * a host. GuiPopover.h is the panel that uses it.
 ***************************************************************************/
#pragma once

//!The control a popover opens beside, in canvas pixels.
//!\ingroup grp_layout
struct GuiPopoverAnchor {
	//!Which way the panel lines up: with the anchor's nearer edge (AUTO), or against a side margin to leave the other side of the screen clear
	enum Edge { AUTO, LEFT_MARGIN, RIGHT_MARGIN };
	int x, y, w, h;
	bool valid;		//!< false for a popover with no anchor: it is centered
	Edge edge;
	//!No anchor: the popover is centered
	GuiPopoverAnchor() : x(0), y(0), w(0), h(0), valid(false), edge(AUTO) {}
	//!The popover opens beside the rectangle `x`,`y`,`w`,`h`
	GuiPopoverAnchor(int ax, int ay, int aw, int ah, Edge e = AUTO) : x(ax), y(ay), w(aw), h(ah), valid(true), edge(e) {}
};

//!Where the popover's top-left corner goes.
//!\ingroup grp_layout
struct GuiPopoverPosition { int x, y; };

//!What a popover keeps clear at the screen's edges: the bars at the top and bottom, and the margin at both sides.
//!The default is none: the whole screen is available.
//!\ingroup grp_layout
struct GuiPopoverInsets {
	int top, bottom, side;
	GuiPopoverInsets(int t = 0, int b = 0, int s = 0) : top(t), bottom(b), side(s) {}
};

//!Which way a popover's tail points.
//!\ingroup grp_layout
enum class POPOVER_TAIL { UP, DOWN, LEFT, RIGHT };

//!The little tail on a popover: `x`,`y` is its top-left corner, `w`,`h` its size, `direction` the way it points (toward the anchor).
//!\ingroup grp_layout
struct GuiPopoverTail { bool shown; POPOVER_TAIL direction; int x, y, w, h; };

//!The most height a popover can have: the band between the bars.
//!\ingroup grp_layout
int GuiPopoverMaxHeight(int screenHeight, const GuiPopoverInsets& insets);

//!Places a popover of `w` x `h` beside its anchor: below it when that fits above the bottom bar, else above it,
//!and failing both inside the band between the bars. A popover whose anchor is in a bar comes straight out of it,
//!so the bars bind only an anchor that is in the band. Horizontally it lines up with the anchor's right edge when the
//!anchor is on the right half of the screen and its left edge otherwise (or against a side margin, per the anchor's
//!edge), and always stays inside the side margins. With no anchor it is centered.
//!\ingroup grp_layout
GuiPopoverPosition GuiPopoverPlace(const GuiPopoverAnchor& anchor, int w, int h, int screenWidth, int screenHeight, const GuiPopoverInsets& insets);

//!Where the tail goes for a panel of `panelW` x `panelH` at `at`: on the side facing the anchor, level with the
//!anchor's middle but kept clear of the panel's corners. Not shown when the panel covers the anchor or there is none.
//!\ingroup grp_layout
GuiPopoverTail GuiPopoverTailAt(const GuiPopoverAnchor& anchor, const GuiPopoverPosition& at, int panelW, int panelH);
