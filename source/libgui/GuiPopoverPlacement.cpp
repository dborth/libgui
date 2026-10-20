/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiPopoverPlacement.cpp
 *
 * See GuiPopoverPlacement.h.
 ***************************************************************************/
#include "GuiPopoverPlacement.h"
#include <algorithm>

static const int kGap = 6;	// between the anchor and the panel

int GuiPopoverMaxHeight(int screenHeight, const GuiPopoverInsets& insets)
{
	return screenHeight - insets.top - insets.bottom - 2 * kGap;
}

GuiPopoverPosition GuiPopoverPlace(const GuiPopoverAnchor& a, int w, int h, int screenW, int screenH, const GuiPopoverInsets& insets)
{
	const int top = insets.top + kGap;
	const int bottom = screenH - insets.bottom - kGap;
	GuiPopoverPosition p;
	if(!a.valid)
	{
		p.x = (screenW - w) / 2;
		p.y = (screenH - h) / 2;
	}
	else
	{
		// Straight under its anchor if that fits above the footer bar, else straight over it; a chip in a bar
		// gets a panel that comes right out of it. Otherwise (a row in the middle of the page) it keeps to the band.
		const int below = a.y + a.h + kGap, above = a.y - h - kGap;
		if(below + h <= bottom)
			p.y = below;
		else if(above >= top)
			p.y = above;
		else
			p.y = std::max(top, std::min(below, bottom - h));
		if(a.edge == GuiPopoverAnchor::LEFT_MARGIN)
			p.x = insets.side;
		else if(a.edge == GuiPopoverAnchor::RIGHT_MARGIN)
			p.x = screenW - insets.side - w;
		else
			p.x = (a.x + a.w / 2 > screenW / 2) ? a.x + a.w - w : a.x;
	}
	p.x = std::max(insets.side, std::min(p.x, screenW - insets.side - w));
	return p;
}

GuiPopoverTail GuiPopoverTailAt(const GuiPopoverAnchor& a, const GuiPopoverPosition& at, int panelW, int panelH)
{
	const int len = 8, base = 16, edge = 18;	// how far it sticks out, how wide it is, how far from a corner it stays
	GuiPopoverTail t = { false, POPOVER_TAIL::UP, 0, 0, 0, 0 };
	if(!a.valid)
		return t;
	const int ax = a.x + a.w / 2, ay = a.y + a.h / 2;
	if(at.y >= a.y + a.h)			// below it: pointing up, under the anchor's middle
	{
		t = { true, POPOVER_TAIL::UP, std::max(at.x + edge, std::min(ax - base / 2, at.x + panelW - edge - base)), at.y - len + 1, base, len };
	}
	else if(at.y + panelH <= a.y)	// above it: pointing down
	{
		t = { true, POPOVER_TAIL::DOWN, std::max(at.x + edge, std::min(ax - base / 2, at.x + panelW - edge - base)), at.y + panelH - 1, base, len };
	}
	else if(at.x >= ax)				// beside it, on its right (it may cover the anchor's end): pointing left, level with the anchor
	{
		t = { true, POPOVER_TAIL::LEFT, at.x - len + 1, std::max(at.y + edge, std::min(ay - base / 2, at.y + panelH - edge - base)), len, base };
	}
	else if(at.x + panelW <= ax)	// on its left: pointing right
	{
		t = { true, POPOVER_TAIL::RIGHT, at.x + panelW - 1, std::max(at.y + edge, std::min(ay - base / 2, at.y + panelH - edge - base)), len, base };
	}
	return t;
}
