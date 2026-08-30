/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutVideoDriver.h
 ***************************************************************************/
#pragma once

#include <gx2/sampler.h>
#include <gx2/texture.h>
#include "../VideoDriver.h"

class WutVideoDriver : public VideoDriver
{
	public:
		WutVideoDriver();
		~WutVideoDriver() override;

		void init(int width, int height) override;
		void shutdown() override;
		void render() override;
		void clearScreen(const PixelColor& color) override;

		int getScreenWidth() const override { return screenWidth; }
		int getScreenHeight() const override { return screenHeight; }
		uint32_t getFrameTimer() override { return frameTimer; }

		ImageRenderer* getImageRenderer() override { return imageRenderer; }
		GlyphRenderer* getGlyphRenderer() override { return glyphRenderer; }

	private:
		// Binds the TV context state and resets the per-frame render
		// state (viewport/scissor/blend/depth/cull) that WHBGfxInit()
		// doesn't set on its own. Called once at the end of init() so
		// the first frame's draws land somewhere valid, then again at
		// the top of every render() pass.
		void prepareFrame();

		int screenWidth;
		int screenHeight;
		uint32_t frameTimer;
		PixelColor clearColor;

		ImageRenderer * imageRenderer;
		GlyphRenderer * glyphRenderer;
};

class WutImageRenderer : public ImageRenderer
{
	public:
		WutImageRenderer(WutVideoDriver * driver);

		void * createTexture(int width, int height) override;
		void loadTextureData(void * texture, const uint8_t * rgba, int width, int height) override;
		void destroyTexture(void * texture) override;
		void drawTexture(void * texture, float xpos, float ypos, uint16_t width, uint16_t height, float degrees, float scaleX, float scaleY, uint8_t alpha) override;
		void drawRectangle(float x, float y, float width, float height, PixelColor color) override;

	private:
		WutVideoDriver * driver;
		GX2Sampler sampler;
};

class WutGlyphRenderer : public GlyphRenderer
{
	public:
		WutGlyphRenderer(WutVideoDriver * driver);

		void* createTexture(uint16_t width, uint16_t height) override;
		void loadTextureData(void* texture, FT_Bitmap* bitmap) override;
		void destroyTexture(void* texture) override;

		void drawQuad(void* texture, int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color) override;
		void drawFeature(int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color) override;

	private:
		WutVideoDriver * driver;
		GX2Sampler sampler;
};