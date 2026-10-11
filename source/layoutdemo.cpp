/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 * layoutdemo.cpp
 ***************************************************************************/
#include "layoutdemo.h"

int GuiLayoutDemo::battery(int n) const
{
	return players[n].battery - (frame / 120) % 10; // draining, to show bound text updating
}

// One player's card. Selecting it (pointer or D-pad) lights it up; clicking it runs onClick.
GuiCard& GuiLayoutDemo::playerCard(int n)
{
	return ui.card().size(592, 0)
		.onFocus([this, n]{ focused = n; status = GuiFormat("Selected player %d", n + 1); })
		.onClick([this, n]{ status = GuiFormat("Opened player %d", n + 1); })
		.add(ui.column().gap(ui.theme().spacing)
			.add(ui.row().size(568, 0).gap(8).align(LAYOUT_ALIGN::CENTRE)
				.add(ui.label(n == 0 ? "P1" : "P2").fontSize(22).accent())
				.add(ui.label(players[n].device), GuiItem().grow(1))
				.add(ui.label().bind([this, n]{ return GuiFormat("battery %d%%", battery(n)); }).fontSize(16).muted()))
			.add(ui.row().gap(8).align(LAYOUT_ALIGN::CENTRE)
				.add(ui.chip(players[n].role))
				.add(ui.label(players[n].scheme).fontSize(16).muted())));
}

GuiLayoutDemo::GuiLayoutDemo(GuiTrigger * trigger, GuiSound * soundOver) :
	frame(0), focused(-1), status("Move with the D-pad or the pointer"), backBtn(nullptr)
{
	players[0] = Player{ "Wiimote + Nunchuk", "Joystick 1", "Arrow keys", 80 };
	players[1] = Player{ "Classic Controller", "Keys + mouse", "Arrow keys", 45 };

	ui.setTrigger(trigger);
	ui.setSounds(soundOver, nullptr);
	const GuiTheme& t = ui.theme();

	ui.bind([this]{ frame++; });

	// title on the left, scheme on the right
	this->gap(t.spacing + 4);
	this->add(ui.row().size(592, 0)
		.add(ui.label("Controllers").fontSize(28))
		.addSpace()
		.add(ui.label("Scheme: Arrow keys").fontSize(18).accent()));

	this->add(ui.column().gap(t.spacing)
		.add(playerCard(0))
		.add(playerCard(1)));

	// four facts on a plain (non-selectable) panel
	this->add(ui.card().selectable(false).style(t.panelStyle()).pad(14, 10)
		.add(ui.grid(2).size(564, 0).gap(0, 6)
			.add(ui.label("USB keyboard: connected").fontSize(15).muted())
			.add(ui.label("USB mouse: none").fontSize(15).muted())
			.add(ui.label("Wiimote IR: no (Nunchuk)").fontSize(15).muted())
			.add(ui.label("GamePad touch: n/a").fontSize(15).muted())));

	// five buttons in rows of three. Identify only works once a card has been visited.
	GuiTextButton& back = ui.button("Go Back").size(150, 44);
	backBtn = &back;
	this->add(ui.grid(3).gap(t.spacing + 4)
		.add(ui.button("Test").size(150, 44).onClick([this]{ status = "Test pressed"; }))
		.add(ui.button("Map").size(150, 44).onClick([this]{ status = "Map pressed"; }))
		.add(ui.button("Reset").size(150, 44).onClick([this]{ status = "Reset pressed"; }))
		.add(ui.button("Identify").size(150, 44)
			.onClick([this]{ status = GuiFormat("Identifying player %d", focused + 1); })
			.enabledWhen([this]{ return focused >= 0; }))
		.add(back));

	this->add(ui.label().bind([this]{ return status; }).fontSize(16).muted());

	this->setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	this->setPosition(24, 16);
	ui.attachTo(*this);
}
