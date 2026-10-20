/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * layoutdemo.cpp
 ***************************************************************************/
#include "layoutdemo.h"

const char * GuiSchemePopover::schemeName(int n)
{
	static const char * const names[SCHEMES] = { "Arrow keys", "Joystick", "Mouse" };
	return names[n];
}

GuiSchemePopover::GuiSchemePopover(const GuiPopoverAnchor& anchor, GuiTrigger * trigger, int screenWidth, int screenHeight) :
	GuiPopover(screenWidth, screenHeight), picked(-1)
{
	ui.setTrigger(trigger);
	const GuiTheme& t = ui.theme();
	const int rowW = 200, rowH = 40, pad = 12;
	const int panelW = rowW + 2 * pad, panelH = SCHEMES * rowH + (SCHEMES - 1) * (int)t.spacing + 2 * pad;

	GuiLayout& rows = ui.column().gap(t.spacing);
	for(int i = 0; i < SCHEMES; i++)
		rows.add(ui.button(schemeName(i)).size(rowW, rowH).onClick([this, i]{ picked = i; dismiss(); }));

	GuiCard& panel = ui.card().selectable(false).size(panelW, panelH).pad(pad, pad).add(rows);
	mount(panel, panelW, panelH, anchor, GuiPopoverInsets(8, 8, 24));
	ui.attachTo(*this);
}

GuiPopoverAnchor GuiLayoutDemo::schemeAnchor()
{
	return GuiPopoverAnchor(schemeBtn->getLeft(), schemeBtn->getTop(), schemeBtn->getWidth(), schemeBtn->getHeight());
}

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

GuiLayoutDemo::GuiLayoutDemo(GuiTrigger * trigger) :
	frame(0), focused(-1), status("Move with the D-pad or the pointer"), backBtn(nullptr), configBtn(nullptr), schemeBtn(nullptr), scheme(0)
{
	players[0] = Player{ "Wiimote + Nunchuk", "Joystick 1", "Arrow keys", 80 };
	players[1] = Player{ "Classic Controller", "Keys + mouse", "Arrow keys", 45 };

	ui.setTrigger(trigger);
	const GuiTheme& t = ui.theme();

	ui.bind([this]{ frame++; });

	// title on the left, scheme on the right
	this->gap(t.spacing + 4);
	this->add(ui.row().size(592, 0)
		.add(ui.label("Controllers").fontSize(28))
		.addSpace()
		.add(ui.label().bind([this]{ return std::string("Scheme: ") + GuiSchemePopover::schemeName(scheme); }).fontSize(18).accent()));

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
	GuiTextButton& config = ui.button("Config").size(150, 44);
	configBtn = &config;
	GuiTextButton& schemeAsk = ui.button("Scheme").size(150, 44);	// no callback: the page loop polls it and opens a GuiSchemePopover
	schemeBtn = &schemeAsk;
	this->add(ui.grid(3).gap(t.spacing + 4)
		.add(ui.button("Test").size(150, 44).onClick([this]{ status = "Test pressed"; }))
		.add(schemeAsk)
		.add(ui.button("Reset").size(150, 44).onClick([this]{ status = "Reset pressed"; }))
		.add(ui.button("Identify").size(150, 44)
			.onClick([this]{ status = GuiFormat("Identifying player %d", focused + 1); })
			.enabledWhen([this]{ return focused >= 0; }))
		.add(config)
		.add(back));

	this->add(ui.label().bind([this]{ return status; }).fontSize(16).muted());

	this->setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	this->setPosition(24, 16);
	ui.attachTo(*this);
}
