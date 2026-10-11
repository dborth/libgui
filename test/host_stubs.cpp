// Stand-ins for the few platform-dependent pieces the real widgets reach, so the container, builder and
// window classes can run on a host machine. Text is measured as 0.5 x its pixel size per character.
#include "../source/libgui/Gui.h"
#include <cwchar>

Platform * platform = nullptr; // never used by the code under test
bool g_triggerClicked = false; // what every GuiTrigger reports from isClicked()

// ---- controller
bool InputController::up() const { return false; }
bool InputController::down() const { return false; }
bool InputController::left() const { return false; }
bool InputController::right() const { return false; }
bool InputController::isSecondaryPressed() const { return false; }

// ---- triggers and sounds
GuiTrigger::GuiTrigger() {}
void GuiTrigger::setPrimaryTrigger(int) {}
bool GuiTrigger::isClicked(const InputController *) const { return g_triggerClicked; }
bool GuiTrigger::isHeld(const InputController *) const { return false; }
void GuiSound::play() {}

// ---- text
static int16_t g_pixelSize = 20;

wchar_t * GuiTextRenderer::charToWideChar(const char * p)
{
	const size_t n = strlen(p);
	wchar_t * w = new wchar_t[n + 1];
	for(size_t i = 0; i <= n; i++)
		w[i] = (unsigned char)p[i];
	return w;
}
void GuiTextRenderer::setPixelSize(int16_t px) { g_pixelSize = px; }
#ifndef HOST_CHAR_WIDTH_PERCENT
#define HOST_CHAR_WIDTH_PERCENT 50 // average glyph width as a percentage of the pixel size
#endif
uint16_t GuiTextRenderer::getWidth(const wchar_t * t) { return (uint16_t)(wcslen(t) * (size_t)g_pixelSize * HOST_CHAR_WIDTH_PERCENT / 100); }
uint16_t GuiTextRenderer::getHeight(const wchar_t *) { return (uint16_t)(g_pixelSize + 4); }
uint16_t GuiTextRenderer::drawText(int16_t, int16_t, const wchar_t *, PixelColor, uint32_t) { return 0; }

GuiTextTranslator::GuiTextTranslator() {}
const char * GuiTextTranslator::getText(const char * msgid) const { return msgid; }

static char g_rendererStorage[sizeof(GuiTextRenderer)];
GuiTextRenderer * fontSystem = reinterpret_cast<GuiTextRenderer *>(g_rendererStorage); // its stubbed methods never touch it
