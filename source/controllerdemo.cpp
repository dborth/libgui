/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * controllerdemo.cpp
 ***************************************************************************/
#include "controllerdemo.h"

static const char * const kTargets[] = { "nothing", "Key: Ctrl", "Key: Enter", "Joystick button 1", "Mouse left" };
static const int kTargetCount = (int)(sizeof(kTargets) / sizeof(kTargets[0]));

int GuiControllerDemo::highlighted() const
{
	const std::vector<int>& h = view->getHighlight();
	return h.empty() ? 0 : h[0];
}

// One row of a list: an icon and a name. Selecting it (pointer or D-pad) highlights the control on the
// controller and draws a line to it; clicking it picks the next example target for that control.
GuiCard& GuiControllerDemo::listRow(GLYPH glyph, const char * letter, const char * name, int control, Side side, int width)
{
	const GuiTheme& t = ui.theme();

	GuiLayout& content = ui.row().size(width - 16, 0).gap(8).align(LAYOUT_ALIGN::CENTRE);
	if(side == Side::LEFT)
		content.addSpace().add(ui.label(name).fontSize(15)).add(ui.glyph(glyph, letter));
	else
		content.add(ui.glyph(glyph, letter)).add(ui.label(name).fontSize(15));

	GuiCard& row = ui.card();
	row.size(width, 0).style(t.rowStyle()).pad(8, 3);
	row.corners(GuiCorners(4.0f));
	row.onFocus([this, &row, control, side, name]{
		view->setHighlight(control);
		const float midY = (float)(row.getTop() + row.getHeight() / 2);
		if(side == Side::LEFT)
			view->setLeader(true, (float)(row.getLeft() + row.getWidth()), midY);
		else if(side == Side::RIGHT)
			view->setLeader(true, (float)row.getLeft(), midY);
		else
			view->setLeader(true, (float)(row.getLeft() + row.getWidth() / 2), (float)row.getTop());
		status = GuiFormat("%s: %s   (A to change)", name, kTargets[targetOf[control] % kTargetCount]);
	});
	row.onClick([this, control, name]{
		int& i = targetOf[control];
		i = (i + 1) % kTargetCount;
		status = GuiFormat("%s: %s", name, kTargets[i]);
	});
	row.add(content);
	return row;
}

GuiControllerDemo::GuiControllerDemo(GuiTrigger * trigger, GuiSound * soundOver) :
	view(nullptr), status("Move over a control, A to change what it does"), backBtn(nullptr)
{
	ui.setTrigger(trigger);
	ui.setSounds(soundOver, nullptr);
	const GuiTheme& t = ui.theme();
	const int sideW = 164;
	const int groupW = 145;

	view = &ui.controllerView(GuiProControllerLayout(), 0.68f);

	// the controller in the middle, shoulder and system controls listed beside it
	GuiLayout& left = ui.column().gap(4).align(LAYOUT_ALIGN::END).pad(0, 14, 0, 0)
		.add(listRow(GLYPH::BUMPER, "L", "Left Bumper", INPUT_TRIGGER_L, Side::LEFT, sideW))
		.add(listRow(GLYPH::BUMPER, "ZL", "Left Trigger", INPUT_TRIGGER_ZL, Side::LEFT, sideW))
		.add(listRow(GLYPH::MINUS, "", "Select", INPUT_BTN_MINUS, Side::LEFT, sideW))
		.add(listRow(GLYPH::CAPTURE, "", "Capture", GUI_CTL_CAPTURE, Side::LEFT, sideW));
	GuiLayout& right = ui.column().gap(4).pad(0, 14, 0, 0)
		.add(listRow(GLYPH::BUMPER, "R", "Right Bumper", INPUT_TRIGGER_R, Side::RIGHT, sideW))
		.add(listRow(GLYPH::BUMPER, "ZR", "Right Trigger", INPUT_TRIGGER_ZR, Side::RIGHT, sideW))
		.add(listRow(GLYPH::PLUS, "", "Start", INPUT_BTN_PLUS, Side::RIGHT, sideW))
		.add(listRow(GLYPH::HOME, "", "Home", INPUT_BTN_HOME, Side::RIGHT, sideW));

	// the groups underneath
	GuiLayout& groups = ui.row().gap(t.spacing + 4)
		.add(ui.column().gap(2)
			.add(ui.label("LEFT STICK").fontSize(13).muted())
			.add(listRow(GLYPH::STICK, "L", "Joystick", INPUT_THUMB_L, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::STICK, "L", "Click", INPUT_THUMB_L, Side::BOTTOM, groupW)))
		.add(ui.column().gap(2)
			.add(ui.label("D-PAD").fontSize(13).muted())
			.add(listRow(GLYPH::UP, "", "Up", INPUT_BTN_UP, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::DOWN, "", "Down", INPUT_BTN_DOWN, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::LEFT, "", "Left", INPUT_BTN_LEFT, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::RIGHT, "", "Right", INPUT_BTN_RIGHT, Side::BOTTOM, groupW)))
		.add(ui.column().gap(2)
			.add(ui.label("RIGHT STICK").fontSize(13).muted())
			.add(listRow(GLYPH::STICK, "R", "Joystick", INPUT_THUMB_R, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::STICK, "R", "Click", INPUT_THUMB_R, Side::BOTTOM, groupW)))
		.add(ui.column().gap(2)
			.add(ui.label("FACE BUTTONS").fontSize(13).muted())
			.add(listRow(GLYPH::BUTTON, "B", "B Button", INPUT_BTN_B, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::BUTTON, "A", "A Button", INPUT_BTN_A, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::BUTTON, "Y", "Y Button", INPUT_BTN_Y, Side::BOTTOM, groupW))
			.add(listRow(GLYPH::BUTTON, "X", "X Button", INPUT_BTN_X, Side::BOTTOM, groupW)));

	// status on the left, Back on the right, on a dark bar
	GuiTextButton& back = ui.button("Back").size(90, 30);
	backBtn = &back;

	this->gap(8);
	this->add(ui.row().gap(4).add(left).add(*view).add(right));
	this->add(groups);
	this->add(ui.card().selectable(false).style(t.barStyle()).size(624, 0).pad(10, 6)
		.add(ui.row().size(604, 0).align(LAYOUT_ALIGN::CENTRE)
			.add(ui.label().bind([this]{ return status; }).fontSize(15).muted())
			.addSpace()
			.add(back)));

	this->setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	this->setPosition(8, 6);
	ui.attachTo(*this);
}
