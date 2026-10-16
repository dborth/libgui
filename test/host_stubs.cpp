// Stand-ins for the few platform-dependent pieces the real widgets reach, so the container, builder and
// window classes can run on a host machine. Text is measured as 0.5 x its pixel size per character.
#include "../source/libgui/Gui.h"
#include <cwchar>
#include <vector>

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
uint32_t GuiTextRenderer::metricsGeneration = 0; // the stub renderer's metrics never change

// Width of the first n characters under the same model as getWidth() above.
static int hostPrefixWidth(size_t n) { return (int)(uint16_t)(n * (size_t)g_pixelSize * HOST_CHAR_WIDTH_PERCENT / 100); }

int GuiTextRenderer::fitCount(const wchar_t * t, int maxWidth)
{
	int best = 0;
	for(int i = 0; t[i]; i++)
		if(hostPrefixWidth((size_t)i + 1) <= maxWidth) best = i + 1;
	return best;
}

// Same line construction and trimming as the real scrollWindow(); only the per-prefix widths differ.
// Uses locals rather than the renderer's scratch members, which don't exist in the stub renderer.
void GuiTextRenderer::scrollWindow(const wchar_t * text, uint32_t textlen, uint32_t pos, int maxWidth, wchar_t * out)
{
	uint32_t dynlen = textlen > pos ? textlen - pos : 0;
	const bool spaced = dynlen + 2 < textlen;
	if(spaced) dynlen += 2;
	const uint32_t extra = textlen > dynlen + 1 ? textlen - 1 - dynlen : 0;
	const uint32_t total = dynlen + extra;

	std::vector<wchar_t> line(total + 1);
	uint32_t n = 0;
	for(uint32_t i = pos; i < textlen; i++) line[n++] = text[i];
	if(spaced) { line[n++] = L' '; line[n++] = L' '; }
	for(uint32_t i = 0; i < extra; i++) line[n++] = text[i];
	line[n] = 0;

	uint32_t len;
	if(hostPrefixWidth(dynlen) > maxWidth)
	{
		len = dynlen;
		while(len > 0 && hostPrefixWidth(len) > maxWidth) --len;
	}
	else
	{
		len = dynlen;
		while(hostPrefixWidth(len) < maxWidth && len + 1 < textlen) ++len;
		const uint32_t drop = hostPrefixWidth(len) > maxWidth ? 2 : 1;
		len = len > drop ? len - drop : 0;
	}
	for(uint32_t i = 0; i < len; i++) out[i] = line[i];
	out[len] = 0;
}

uint16_t GuiTextRenderer::getHeight(const wchar_t *) { return (uint16_t)(g_pixelSize + 4); }
uint16_t GuiTextRenderer::drawText(int16_t, int16_t, const wchar_t *, PixelColor, uint32_t) { return 0; }

GuiTextTranslator::GuiTextTranslator() {}
const char * GuiTextTranslator::getText(const char * msgid) const { return msgid; }

static char g_rendererStorage[sizeof(GuiTextRenderer)];
GuiTextRenderer * fontSystem = reinterpret_cast<GuiTextRenderer *>(g_rendererStorage); // its stubbed methods never touch it
