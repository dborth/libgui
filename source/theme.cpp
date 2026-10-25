/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * theme.cpp
 *
 * The look of the template application: every color and metric the core's
 * GuiTheme reads. The core has no look of its own, so each application
 * defines GuiThemeInit() (this file is the template's; copy it and change the
 * values to restyle an application).
 ***************************************************************************/

#include "libgui/GuiTheme.h"

static PixelColor rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	PixelColor c = { r, g, b, a };
	return c;
}

void GuiThemeInit(GuiTheme& t)
{
	// colors: a dark blue-gray theme with an orange accent
	t.background    = rgb(18, 22, 29);
	t.surface       = rgb(28, 35, 48);
	t.surfaceRaised = rgb(36, 46, 64);
	t.outline       = rgb(43, 54, 71);
	t.outlineStrong = rgb(74, 90, 115);
	t.accent        = rgb(255, 150, 40);
	t.accentLight   = rgb(255, 200, 110);
	t.accentDark    = rgb(200, 110, 20);
	t.text          = rgb(232, 237, 244);
	t.textMuted     = rgb(152, 164, 181);
	t.textOnAccent  = rgb(30, 20, 5);
	t.textInverse   = rgb(30, 20, 5);
	t.good          = rgb(63, 208, 176);
	t.highlight     = GuiMixColor(t.surfaceRaised, t.outlineStrong, 0.65f);
	t.mark          = rgb(255, 150, 40);
	t.shadow        = rgb(0, 0, 0, 150);

	// metrics
	t.radius      = 12.0f;
	t.strokeWidth = 2.0f;
	t.padding     = 12.0f;
	t.spacing     = 8.0f;
	t.shadowBlur  = 14.0f;

	// glyphs
	t.glyphSize          = 19.0f;
	t.glyphBumperWidth   = 27.0f;
	t.glyphBumperHeight  = 18.0f;
	t.glyphFontSize      = 13;
	t.glyphFontSizeSmall = 10;

	// style recipe tuning
	t.shadowOffsetY       = 4.0f;
	t.glowGrow            = 2.0f;
	t.focusGlowAlpha      = 110;
	t.pressGlowAlpha      = 70;
	t.buttonGlowBlur      = 10.0f;
	t.buttonGlowAlpha     = 90;
	t.rowPressAlpha       = 110;
	t.cardDisabledAlpha   = 140;
	t.buttonDisabledAlpha = 150;
	t.buttonStrokeLight   = 0.35f;
	t.barDarken           = 0.55f;
}
