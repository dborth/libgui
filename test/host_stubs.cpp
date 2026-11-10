// Stand-ins for the few platform-dependent pieces the real widgets reach, so the container, builder and
// window classes can run on a host machine. Text is measured as 0.5 x its pixel size per character.
#include "../source/libgui/Gui.h"
#include <cwchar>
#include <vector>

Platform * platform = nullptr; // never used by the code under test
bool g_triggerClicked = false; // what every GuiTrigger reports from isClicked() ...
bool g_perController = false;  // ... unless this is set: then a primary trigger reports bit n of g_primaryMask for the controller on
int g_primaryMask = 0;         // channel n, and a secondary (B) trigger bit n of g_secondaryMask
int g_secondaryMask = 0;

// ---- controller (the real InputController needs a platform; update(nullptr) in these tests relies on the stubs not reading `this`)
InputController::InputController(int ch) : channel(ch), sideways(false), scrollTimer(0.0f), internalScrollTimer(0.0f) {}
void InputController::update(const InputPadData& data, float) { currentData = data; }
bool InputController::up() const { return false; }
bool InputController::down() const { return false; }
bool InputController::left() const { return false; }
bool InputController::right() const { return false; }
bool InputController::isSecondaryPressed() const { return false; }

// ---- triggers and sounds
GuiTrigger::GuiTrigger() : type(TRIGGER_TYPE::SIMPLE), action(TRIGGER_ACTION::NONE), chan(-1), conditionMask(0) {}
void GuiTrigger::setPrimaryTrigger(int) { action = TRIGGER_ACTION::PRIMARY; }
void GuiTrigger::setSecondaryTrigger(int) { action = TRIGGER_ACTION::SECONDARY; }
bool GuiTrigger::isClicked(const InputController * c) const
{
	if(!g_perController)
		return g_triggerClicked;
	const int mask = action == TRIGGER_ACTION::SECONDARY ? g_secondaryMask : g_primaryMask;
	return c && c->getChannel() >= 0 && ((mask >> c->getChannel()) & 1) != 0;
}
bool GuiTrigger::isHeld(const InputController *) const { return false; }

// ---- text
static int16_t g_pixelSize = 20;
std::vector<std::wstring> g_drawnText; // the text of each drawText() call (its runs joined), in order; tests clear it

// Characters in a list of runs (a NUL ends its run early, as in the real renderer)
static size_t hostChars(const GuiTextRun * runs, int count)
{
	size_t n = 0;
	for(int r = 0; r < count; r++)
		for(uint32_t i = 0; i < runs[r].len && runs[r].text[i]; i++)
			n++;
	return n;
}

// One byte, one character (the real converter reads UTF-8; the tests' text is ASCII)
size_t GuiTextRenderer::utf8ToWide(const char * p, wchar_t * out, size_t outCap)
{
	const size_t limit = out ? (outCap ? outCap - 1 : 0) : (size_t)-1;
	size_t n = 0;
	while(p && p[n] && n < limit)
	{
		if(out) out[n] = (unsigned char)p[n];
		n++;
	}
	if(out && outCap) out[n] = 0;
	return n;
}
void GuiTextRenderer::setPixelSize(int16_t px) { g_pixelSize = px; }
#ifndef HOST_CHAR_WIDTH_PERCENT
#define HOST_CHAR_WIDTH_PERCENT 50 // average glyph width as a percentage of the pixel size
#endif
// Width of n characters under the model above: each is HOST_CHAR_WIDTH_PERCENT of the pixel size wide
static int hostPrefixWidth(size_t n) { return (int)(uint16_t)(n * (size_t)g_pixelSize * HOST_CHAR_WIDTH_PERCENT / 100); }

uint16_t GuiTextRenderer::getWidth(const GuiTextRun * runs, int count) { return (uint16_t)hostPrefixWidth(hostChars(runs, count)); }
uint32_t GuiTextRenderer::metricsGeneration = 0; // the stub renderer's metrics never change

int GuiTextRenderer::fitCount(const GuiTextRun * runs, int count, int maxWidth)
{
	const size_t total = hostChars(runs, count);
	int best = 0;
	for(size_t i = 1; i <= total; i++)
		if(hostPrefixWidth(i) <= maxWidth) best = (int)i;
	return best;
}

uint16_t GuiTextRenderer::getLineHeight() { return (uint16_t)(g_pixelSize + 4); }
void GuiTextRenderer::drawText(int16_t, int16_t, const GuiTextRun * runs, int count, PixelColor, uint32_t)
{
	std::wstring joined;
	for(int r = 0; r < count; r++)
		for(uint32_t i = 0; i < runs[r].len && runs[r].text[i]; i++)
			joined += runs[r].text[i];
	g_drawnText.push_back(joined);
}

// (the constructor is constexpr and defined in the header; only the lookup is stubbed)
const char * GuiTextTranslator::getText(const char * msgid) const { return msgid; }

static char g_rendererStorage[sizeof(GuiTextRenderer)];
GuiTextRenderer * fontSystem = reinterpret_cast<GuiTextRenderer *>(g_rendererStorage); // its stubbed methods never touch it
