/****************************************************************************
 * libgui Template
 * Daryl Borth 2009-2026
 * layoutdemo.cpp
 ***************************************************************************/
#include "layoutdemo.h"

DemoButton::DemoButton(const char * label, int w, int h, GuiTrigger * trigger, GuiSound * soundOver) :
	button(w, h),
	shape(w, h),
	text(label, 20, GuiTheme::current().text),
	textLit(label, 20, GuiTheme::current().textOnAccent)
{
	const GuiTheme& theme = GuiTheme::current();

	shape.setCapsule();
	shape.setStyle(theme.buttonStyle());
	text.setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);
	textLit.setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);

	button.setImage(&shape);
	button.setLabel(&text);
	button.setLabelOver(&textLit);
	button.setSoundOver(soundOver);
	button.setTrigger(trigger);
	button.setEffectGrow();
}

DemoPlayerCard::DemoPlayerCard(const char * number, const char * device, const char * battery, const char * role, const char * scheme) :
	backdrop(1, 1),
	numberTxt(number, 22, GuiTheme::current().accent),
	deviceTxt(device, 20, GuiTheme::current().text),
	batteryTxt(battery, 16, GuiTheme::current().textMuted),
	roleTxt(role, 16, GuiTheme::current().text),
	schemeTxt(scheme, 16, GuiTheme::current().textMuted),
	hintTxt("Test  >   Map  >", 16, GuiTheme::current().textMuted),
	chipBackdrop(1, 1),
	header(264, 0),
	details()
{
	const GuiTheme& theme = GuiTheme::current();

	backdrop.setCorners(GuiCorners(theme.radius));
	backdrop.setStyle(theme.cardStyle());
	chipBackdrop.setCapsule();
	chipBackdrop.setStyle(theme.chipStyle());

	// "Joystick 1" on a pill
	chip.setPadding(10, 3);
	chip.add(&chipBackdrop, GuiItem().background());
	chip.add(&roleTxt);

	// P1   Wiimote + Nunchuk        battery 80%
	header.setSpacing(theme.spacing).setAlignItems(LAYOUT_ALIGN::CENTRE);
	header.add(&numberTxt);
	header.add(&deviceTxt, GuiItem().grow(1));
	header.add(&batteryTxt);

	// [Joystick 1]  Arrow keys
	details.setSpacing(theme.spacing).setAlignItems(LAYOUT_ALIGN::CENTRE);
	details.add(&chip);
	details.add(&schemeTxt);

	body.setSpacing(theme.spacing);
	body.add(&header);
	body.add(&details);
	body.add(&hintTxt);

	// the card: a backdrop behind the content, with room around it
	this->setPadding(theme.padding);
	this->add(&backdrop, GuiItem().background());
	this->add(&body);
}

GuiLayoutDemo::GuiLayoutDemo(GuiTrigger * trigger, GuiSound * soundOver) :
	titleTxt("Controllers", 28, GuiTheme::current().text),
	schemeTxt("Scheme: Arrow keys", 18, GuiTheme::current().accent),
	titleBar(592, 0),
	card1("P1", "Wiimote + Nunchuk", "battery 80%", "Joystick 1", "Arrow keys"),
	card2("P2", "Classic Controller", "battery 45%", "Keys + mouse", "Arrow keys"),
	stripBackdrop(1, 1),
	keyboardTxt("USB keyboard: connected", 15, GuiTheme::current().textMuted),
	mouseTxt("USB mouse: none", 15, GuiTheme::current().textMuted),
	irTxt("Wiimote IR: no (Nunchuk)", 15, GuiTheme::current().textMuted),
	touchTxt("GamePad touch: n/a", 15, GuiTheme::current().textMuted),
	stripRow(560, 0),
	testBtn("Test", 150, 44, trigger, soundOver),
	mapBtn("Map", 150, 44, trigger, soundOver),
	backBtn("Go Back", 150, 44, trigger, soundOver)
{
	const GuiTheme& theme = GuiTheme::current();

	// title on the left, scheme on the right, whatever width each text turns out to be
	titleBar.add(&titleTxt);
	titleBar.addSpace();
	titleBar.add(&schemeTxt);

	cards.setSpacing(theme.spacing + 4);
	cards.add(&card1);
	cards.add(&card2);

	// four short facts spread evenly across a panel
	stripBackdrop.setCorners(GuiCorners(theme.radius));
	stripBackdrop.setStyle(theme.panelStyle());
	stripRow.setJustify(LAYOUT_JUSTIFY::SPACE_BETWEEN);
	stripRow.add(&keyboardTxt);
	stripRow.add(&mouseTxt);
	stripRow.add(&irTxt);
	stripRow.add(&touchTxt);
	strip.setPadding(14, 10);
	strip.add(&stripBackdrop, GuiItem().background());
	strip.add(&stripRow);

	buttons.setSpacing(theme.spacing + 4);
	buttons.add(&testBtn.button);
	buttons.add(&mapBtn.button);
	buttons.add(&backBtn.button);

	// the screen: top to bottom, placed once
	this->setSpacing(theme.spacing + 4);
	this->add(&titleBar);
	this->add(&cards);
	this->add(&strip);
	this->add(&buttons);
	this->setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
	this->setPosition(24, 20);
}
