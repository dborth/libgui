/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiTextRenderer.h
 ***************************************************************************/
#pragma once

#include <stdint.h>
#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "Gui.h"

// Legacy Text Styling Constants
#define GUI_TEXT_NULL               0x0000
#define GUI_TEXT_JUSTIFY_LEFT       0x0001
#define GUI_TEXT_JUSTIFY_CENTER     0x0002
#define GUI_TEXT_JUSTIFY_RIGHT      0x0004
#define GUI_TEXT_JUSTIFY_MASK       0x000f

#define GUI_TEXT_ALIGN_TOP          0x0010
#define GUI_TEXT_ALIGN_MIDDLE       0x0020
#define GUI_TEXT_ALIGN_BOTTOM       0x0040
#define GUI_TEXT_ALIGN_BASELINE     0x0080
#define GUI_TEXT_ALIGN_GLYPH_TOP    0x0100
#define GUI_TEXT_ALIGN_GLYPH_MIDDLE 0x0200
#define GUI_TEXT_ALIGN_GLYPH_BOTTOM 0x0400
#define GUI_TEXT_ALIGN_MASK         0x0ff0

#define GUI_TEXT_STYLE_UNDERLINE    0x1000
#define GUI_TEXT_STYLE_STRIKE       0x2000
#define GUI_TEXT_STYLE_MASK         0xf000

const PixelColor black = {0, 0, 0, 255};

//!Per-pixel-size font metrics used for text-block alignment/positioning.
struct FontOffset {
	int16_t ascender;
	int16_t descender;
	int16_t max;
	int16_t min;
};

//!Cached per-glyph metrics and rasterized texture, keyed by (pixel size, char code) in GuiTextRenderer::fontData.
struct GlyphData {
	int16_t renderOffsetX;
	uint16_t glyphAdvanceX;
	uint16_t glyphAdvanceY;
	uint32_t glyphIndex;

	uint16_t textureWidth;
	uint16_t textureHeight;

	int16_t renderOffsetY;
	int16_t renderOffsetMax;
	int16_t renderOffsetMin;

	void* texture; // Abstracted texture pointer
};

//!FreeType2-based glyph shaping/caching. Shapes and caches glyphs per
//!pixel size, then delegates only the final rasterized-quad draw to a
//!GlyphRenderer - GuiText calls through this rather than touching
//!FreeType or a platform texture directly.
class GuiTextRenderer {
private:
	FT_Library ftLibrary;
	FT_Face ftFace;
	int16_t currentPixelSize;
	bool ftKerningEnabled;

	GlyphRenderer* renderer;

	struct ftData {
		FontOffset align;
		std::map<wchar_t, GlyphData> charMap;
	};

	std::map<int16_t, ftData> fontData;

	// Internal Calculations
	int16_t getStyleOffsetWidth(uint16_t width, uint32_t format);
	int16_t getStyleOffsetHeight(FontOffset* offset, uint32_t format);
	void drawTextFeature(int16_t x, int16_t y, uint16_t width, FontOffset* offsetData, uint32_t format, const PixelColor& color);

	// Font Management
	void unloadFont();
	GlyphData* cacheGlyphData(wchar_t charCode, int16_t pixelSize);

public:
	//!\param fontBuffer TTF/OTF font data - must remain valid for the
	//!lifetime of this GuiTextRenderer, FreeType keeps a pointer into it
	//!\param bufferSize Length of fontBuffer in bytes
	//!\param glyphRenderer Platform renderer rasterized glyph quads are drawn through
	GuiTextRenderer(const uint8_t* fontBuffer, FT_Long bufferSize, GlyphRenderer* glyphRenderer);
	~GuiTextRenderer();

	//!Selects the pixel size subsequent drawText()/getWidth()/getHeight()
	//!calls use. Shaped glyphs are cached per size, so switching sizes
	//!repeatedly doesn't re-shape glyphs already seen at that size.
	void setPixelSize(int16_t pixelSize);

	// Core Drawing Signatures
	//!Draws text at (x, y) using the current pixel size.
	//!\param x Left edge, in pixels
	//!\param y Top edge, in pixels
	//!\param text Text to draw
	//!\param color Text color
	//!\param renderFlags Bitmask of GUI_TEXT_JUSTIFY_*/GUI_TEXT_ALIGN_*/GUI_TEXT_STYLE_* flags
	//!\return the drawn text's width in pixels
	uint16_t drawText(int16_t x, int16_t y, const wchar_t* text, PixelColor color = black, uint32_t renderFlags = 0);
	//!\overload
	uint16_t drawText(int16_t x, int16_t y, const char* text, PixelColor color = black, uint32_t renderFlags = 0);

	// Dimensions & Offsets
	//!\return text width in pixels at the current pixel size
	uint16_t getWidth(const wchar_t* text);
	//!\overload
	uint16_t getWidth(const char* text);
	//!\return text height in pixels at the current pixel size
	uint16_t getHeight(const wchar_t* text);
	//!\overload
	uint16_t getHeight(const char* text);

	//!\param text Text to measure
	//!\param offset Filled with the ascender/descender/max/min metrics for text at the current pixel size
	void getOffset(const wchar_t* text, FontOffset* offset);

	// Utilities
	//!\return a newly-allocated wide-char copy of a UTF-8 string p - caller owns the result (delete[])
	static wchar_t* charToWideChar(const char* p);
};

extern GuiTextRenderer *fontSystem;
