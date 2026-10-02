/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * VideoDriver.h
 *
 * Platform video backend GuiImage and GuiText delegates to. Exactly one driver
 * implements this and assigns the single global instance below
 ***************************************************************************/
#pragma once

#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

//!8-bit-per-channel RGBA color used by the video interfaces.
//!\ingroup grp_pal
typedef struct {
	uint8_t r;			/*!< Red color component. */
	uint8_t g;			/*!< Green color component. */
	uint8_t b;			/*!< Blue color component. */
	uint8_t a;			/*!< Alpha component. If a function does not use the alpha value, it is safely ignored. */
} PixelColor;

class ImageRenderer;
class GlyphRenderer;

//!Platform video backend owning the frame lifecycle (init/render/clear,
//!screen size, refresh rate, delta time) and handing out an ImageRenderer
//!and a GlyphRenderer. GuiImage/GuiText never touch a platform texture
//!type directly, only these two renderers.
//!\ingroup grp_pal
class VideoDriver
{
	public:
		virtual ~VideoDriver() = default;

		//!\param width Design canvas width in pixels
		//!\param height Design canvas height in pixels
		virtual void init(int width, int height) = 0;
		//!Releases the video backend. Call after every other user of the renderers has finished.
		virtual void shutdown() = 0;

		//! Flushes the current frame to the screen and swaps buffers
		virtual void render() = 0;

		//! Clears the current frame buffer
		virtual void clearScreen(const PixelColor& color) = 0;

		//!\return the design canvas width in pixels (as passed to init())
		virtual int getScreenWidth() const = 0;
		//!\return the design canvas height in pixels (as passed to init())
		virtual int getScreenHeight() const = 0;
		//!\return the number of frames rendered so far (incremented once per render())
		virtual uint32_t getFrameTimer() = 0;
		//!\return the display refresh rate in Hz (50 for PAL/576i output, otherwise 60)
		virtual int getRefreshRate() const = 0;
		//!\return the nominal frame period in seconds (1/getRefreshRate()); not a measured time
		virtual float getDeltaTime() const = 0;

		//! Ratio of physical render-target pixels to design-canvas pixels
		//! (the width/height passed to init()). Greater than 1.0 wherever
		//! the driver upscales the design canvas to fill a higher-resolution.
		virtual float getUIScale() const { return 1.0f; }

		//!\return the renderer GuiImage/GuiImageData draw through
		virtual ImageRenderer* getImageRenderer() = 0;
		//!\return the renderer GuiTextRenderer draws glyphs through
		virtual GlyphRenderer* getGlyphRenderer() = 0;
};

//!Platform image/texture backend GuiImageData and GuiImage delegate to.
//!Exactly one driver implements this and assigns the single
//!global instance below. GuiImageData/GuiImage never touch any platform
//!texture type directly, only imageSystem.
//!\ingroup grp_pal
class ImageRenderer
{
	public:
		virtual ~ImageRenderer() = default;
		//! Allocates an empty hardware-aligned texture buffer
		virtual void * createTexture(int width, int height) = 0;
		//! Loads raw RGBA8 pixels into the pre-allocated texture
		virtual void loadTextureData(void * texture, const uint8_t * rgba, int width, int height) = 0;
		//!Destroys a texture created by createTexture.
		virtual void destroyTexture(void * texture) = 0;
		//!Draws a texture created by createTexture.
		virtual void drawTexture(void * texture, float xpos, float ypos, uint16_t width, uint16_t height, float degrees, float scaleX, float scaleY, uint8_t alpha) = 0;
		//!Draws a solid-color rectangle (alpha-blended using color.a).
		virtual void drawRectangle(float x, float y, float width, float height, PixelColor color) = 0;
};

//!Draws glyph textures and solid rectangles on behalf of GuiTextRenderer. Implemented per platform.
//!\ingroup grp_pal
class GlyphRenderer {
	public:
		virtual ~GlyphRenderer() = default;

		//!Allocates an empty single-channel glyph texture. Returns an opaque handle.
		virtual void* createTexture(uint16_t width, uint16_t height) = 0;
		//!Uploads a rasterized FreeType glyph bitmap into a texture from createTexture().
		virtual void loadTextureData(void* texture, FT_Bitmap* bitmap) = 0;
		//!Destroys a texture created by createTexture().
		virtual void destroyTexture(void* texture) = 0;

		//!Draws a glyph texture as a quad tinted with color.
		virtual void drawQuad(void* texture, int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color) = 0;
		//!Draws a solid-color rectangle used for text decoration (underline/strike-through).
		virtual void drawFeature(int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color) = 0;
};
