/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiPopover.cpp
 *
 * See GuiPopover.h.
 ***************************************************************************/
#include "Gui.h"

GuiPopover::GuiPopover() : GuiPopover(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight())
{
}

GuiPopover::GuiPopover(int screenWidth, int screenHeight) : screenW(screenWidth), screenH(screenHeight), cancel(nullptr), click(nullptr), panel(nullptr), dismissed(false), by(-1)
{
	placed.x = 0;
	placed.y = 0;
	cancel = &ui.own<GuiTrigger>();
	cancel->setSecondaryTrigger();
	click = ui.trigger();

	setFixedSize(screenW, screenH);
	setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	setPosition(0, 0);
}

void GuiPopover::dismiss()
{
	dismissed = true;
	by = -1;
}

void GuiPopover::update(InputController * c)
{
	GuiOverlay::update(c);
	if(dismissed || !c || getState() == STATE::DISABLED)
		return;
	if(parentElement && parentElement->getState() == STATE::DISABLED)
		return;

	// B, or a click anywhere outside the panel. The popover covers the whole screen, so every pointer is always over
	// it: each controller decides for itself, whichever the screen happens to update first.
	const int chan = c->getChannel();
	bool leave = cancel->isClicked(c);
	if(!leave && chan >= 0 && click->isClicked(c))
	{
		const InputPadData& pad = c->getPadData();
		leave = pad.validPointer && (!panel || !panel->isInside(pad.cursor_x, pad.cursor_y));
	}
	if(leave)
	{
		dismissed = true;
		by = chan;
	}
}

void GuiPopover::mount(GuiCard& card, int panelW, int panelH, const GuiPopoverAnchor& anchor, const GuiPopoverInsets& insets)
{
	mount(card, panelW, panelH, anchor, insets, GuiTheme::current().surfaceRaised);
}

void GuiPopover::mount(GuiCard& card, int panelW, int panelH, const GuiPopoverAnchor& anchor, const GuiPopoverInsets& insets, PixelColor tailColor)
{
	panel = &card;
	placed = GuiPopoverPlace(anchor, panelW, panelH, screenW, screenH, insets);

	GuiLayout& at = ui.column().size(screenW, screenH).addGap((float)placed.y).add(ui.row().size(screenW, panelH).addGap((float)placed.x).add(card));
	add(&at);

	// the tail, drawn over the panel's edge in the panel's color, pointing at what the popover belongs to
	const GuiPopoverTail tail = GuiPopoverTailAt(anchor, placed, panelW, panelH);
	if(tail.shown)
	{
		static const SHAPE kinds[] = { SHAPE::TRIANGLE_UP, SHAPE::TRIANGLE_DOWN, SHAPE::TRIANGLE_LEFT, SHAPE::TRIANGLE_RIGHT };
		GuiShape& arrow = ui.own<GuiShape>(tail.w, tail.h);
		arrow.setArrow(kinds[(int)tail.direction], tailColor, tailColor);
		GuiLayout& tailPlaced = ui.column().size(screenW, screenH).addGap((float)tail.y).add(ui.row().size(screenW, tail.h).addGap((float)tail.x).add(arrow));
		add(&tailPlaced);
	}
}
