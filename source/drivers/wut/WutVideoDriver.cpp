/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutVideoDriver.cpp
 ***************************************************************************/
#include <cmath>
#include <cstring>
#include <malloc.h>

#include <gx2/clear.h>
#include <gx2/context.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/sampler.h>
#include <gx2/surface.h>
#include <gx2/texture.h>
#include <whb/gfx.h>
#include <whb/proc.h>

#include "WutVideoDriver.h"
#include "shaders/Texture2DShader.h"
#include "shaders/ColorShader.h"

namespace
{
	inline float DegToRad(float degrees)
	{
		return degrees * (3.14159265358979323846f / 180.0f);
	}

	void PixelRectToNdc(float x, float y, float w, float h, float scaleX, float scaleY, int designWidth, int designHeight, float offset[3], float scale[3])
	{
		float centerPxX = x + w * 0.5f;
		float centerPxY = y + h * 0.5f;

		offset[0] = (centerPxX / designWidth) * 2.0f - 1.0f;
		offset[1] = 1.0f - (centerPxY / designHeight) * 2.0f;
		offset[2] = 0.0f;

		scale[0] = (w * scaleX) / designWidth;
		scale[1] = (h * scaleY) / designHeight;
		scale[2] = 1.0f;
	}
}

namespace
{
	const int kDesignWidth = 640;
	const int kDesignHeight = 480;
}

/****************************************************************************
 * WutVideoDriver
 ***************************************************************************/

WutVideoDriver::WutVideoDriver()
	: screenWidth(0), screenHeight(0), frameTimer(0), clearColor{0, 0, 0, 255}
	, imageRenderer(nullptr), glyphRenderer(nullptr)
{
}

WutVideoDriver::~WutVideoDriver()
{
	delete imageRenderer;
	delete glyphRenderer;
}

void WutVideoDriver::init()
{
	WHBProcInit();
	WHBGfxInit();

	screenWidth = kDesignWidth;
	screenHeight = kDesignHeight;

	imageRenderer = new WutImageRenderer(this);
	glyphRenderer = new WutGlyphRenderer(this);

	prepareFrame();
}

void WutVideoDriver::shutdown()
{
	WHBGfxShutdown();
	WHBProcShutdown();
}

void WutVideoDriver::prepareFrame()
{
	WHBGfxBeginRender();
	WHBGfxBeginRenderTV();
	WHBGfxClearColor(clearColor.r / 255.0f, clearColor.g / 255.0f, clearColor.b / 255.0f, clearColor.a / 255.0f);

	GX2SetDepthOnlyControl(GX2_DISABLE, GX2_DISABLE, GX2_COMPARE_FUNC_ALWAYS);
	GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, GX2_DISABLE, GX2_ENABLE);
	GX2SetCullOnlyControl(GX2_FRONT_FACE_CCW, GX2_DISABLE, GX2_DISABLE);
	GX2SetBlendControl(GX2_RENDER_TARGET_0,
		GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD,
		GX2_DISABLE, GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA, GX2_BLEND_COMBINE_MODE_ADD);
}

void WutVideoDriver::render()
{
	WHBGfxFinishRenderTV();
	WHBGfxFinishRender();

	frameTimer++;

	prepareFrame();
}

void WutVideoDriver::clearScreen(const PixelColor& color)
{
	clearColor = color;
}

/****************************************************************************
 * WutImageRenderer
 ***************************************************************************/

WutImageRenderer::WutImageRenderer(WutVideoDriver * driver_)
	: driver(driver_)
{
	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);
}

void * WutImageRenderer::createTexture(const uint8_t * rgba, int width, int height)
{
	if(!rgba || width <= 0 || height <= 0)
		return nullptr;

	GX2Texture * texture = new GX2Texture();
	GX2InitTexture(texture, width, height, 1, 0, GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8, GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_LINEAR_ALIGNED);

	texture->surface.image = memalign(texture->surface.alignment, texture->surface.imageSize);
	if(!texture->surface.image)
	{
		delete texture;
		return nullptr;
	}

	uint8_t * dst = static_cast<uint8_t *>(texture->surface.image);
	const uint32_t dstStride = texture->surface.pitch * 4;
	const uint32_t srcStride = static_cast<uint32_t>(width) * 4;
	for(int y = 0; y < height; y++)
		memcpy(dst + y * dstStride, rgba + y * srcStride, srcStride);

	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture->surface.image, texture->surface.imageSize);

	return texture;
}

void WutImageRenderer::destroyTexture(void * texture)
{
	if(!texture)
		return;

	GX2Texture * tex = static_cast<GX2Texture *>(texture);
	if(tex->surface.image)
		free(tex->surface.image);
	delete tex;
}

void WutImageRenderer::drawTexture(void * texture, float xpos, float ypos, uint16_t width, uint16_t height, float degrees, float scaleX, float scaleY, uint8_t alpha)
{
	if(!texture)
		return;

	float offset[3];
	float scale[3];
	PixelRectToNdc(xpos, ypos, width, height, scaleX, scaleY, kDesignWidth, kDesignHeight, offset, scale);

	float colorIntensity[4] = { 1.0f, 1.0f, 1.0f, alpha / 255.0f };

	Texture2DShader * shader = Texture2DShader::instance();
	shader->setShaders();
	shader->setAttributeBuffer();
	shader->setAngle(DegToRad(degrees));
	shader->setOffset(offset);
	shader->setScale(scale);
	shader->setColorIntensity(colorIntensity);
	shader->clearBlur();
	shader->setTextureAndSampler(static_cast<GX2Texture *>(texture), &sampler);
	shader->draw(GX2_PRIMITIVE_MODE_QUADS, 4);
}

void WutImageRenderer::drawRectangle(float x, float y, float width, float height, PixelColor color)
{
	float offset[3];
	float scale[3];
	PixelRectToNdc(x, y, width, height, 1.0f, 1.0f, kDesignWidth, kDesignHeight, offset, scale);

	uint8_t colorVtxs[ColorShader::cuColorVtxsSize];
	for(int i = 0; i < 4; i++)
	{
		colorVtxs[i * 4 + 0] = color.r;
		colorVtxs[i * 4 + 1] = color.g;
		colorVtxs[i * 4 + 2] = color.b;
		colorVtxs[i * 4 + 3] = color.a;
	}

	float colorIntensity[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	ColorShader * shader = ColorShader::instance();
	shader->setShaders();
	shader->setAttributeBuffer(colorVtxs);
	shader->setAngle(0.0f);
	shader->setOffset(offset);
	shader->setScale(scale);
	shader->setColorIntensity(colorIntensity);
	shader->draw(GX2_PRIMITIVE_MODE_QUADS, 4);
}

/****************************************************************************
 * WutGlyphRenderer
 ***************************************************************************/

WutGlyphRenderer::WutGlyphRenderer(WutVideoDriver * driver_)
	: driver(driver_)
{
	GX2InitSampler(&sampler, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);
}

void * WutGlyphRenderer::createTexture(uint16_t width, uint16_t height)
{
	GX2Texture * texture = new GX2Texture();
	GX2InitTexture(texture, width == 0 ? 1 : width, height == 0 ? 1 : height, 1, 0, GX2_SURFACE_FORMAT_UNORM_R8, GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_LINEAR_ALIGNED);

	texture->surface.image = memalign(texture->surface.alignment, texture->surface.imageSize);
	if(!texture->surface.image)
	{
		delete texture;
		return nullptr;
	}
	memset(texture->surface.image, 0x00, texture->surface.imageSize);

	// Broadcast the single R8 channel to R,G,B,A on sample (compMap's four
	// 8-bit fields each select source component 0/R) - same trick as the
	// Wii driver's GX_TF_I4 intensity textures, so Texture2DShader's
	// texture*colorIntensity modulate produces anti-aliased, colorable
	// glyphs without a dedicated single-channel shader.
	texture->compMap = 0x00000000;
	GX2InitTextureRegs(texture);

	return texture;
}

void WutGlyphRenderer::loadTextureData(void * texturePtr, FT_Bitmap * bitmap)
{
	if(!texturePtr || !bitmap || !bitmap->buffer)
		return;

	GX2Texture * texture = static_cast<GX2Texture *>(texturePtr);
	if(!texture->surface.image)
		return;

	uint8_t * dst = static_cast<uint8_t *>(texture->surface.image);
	uint8_t * src = bitmap->buffer;

	uint32_t copyWidth = (bitmap->width < texture->surface.width) ? bitmap->width : texture->surface.width;
	uint32_t copyHeight = (bitmap->rows < texture->surface.height) ? bitmap->rows : texture->surface.height;

	for(uint32_t y = 0; y < copyHeight; y++)
		memcpy(dst + y * texture->surface.pitch, src + y * bitmap->width, copyWidth);

	GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, texture->surface.image, texture->surface.imageSize);
}

void WutGlyphRenderer::destroyTexture(void * texturePtr)
{
	if(!texturePtr)
		return;

	GX2Texture * texture = static_cast<GX2Texture *>(texturePtr);
	if(texture->surface.image)
		free(texture->surface.image);
	delete texture;
}

void WutGlyphRenderer::drawQuad(void * texturePtr, int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color)
{
	if(!texturePtr)
		return;

	float offset[3];
	float scale[3];
	PixelRectToNdc(screenX, screenY, width, height, 1.0f, 1.0f, kDesignWidth, kDesignHeight, offset, scale);

	float colorIntensity[4] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };

	Texture2DShader * shader = Texture2DShader::instance();
	shader->setShaders();
	shader->setAttributeBuffer();
	shader->setAngle(0.0f);
	shader->setOffset(offset);
	shader->setScale(scale);
	shader->setColorIntensity(colorIntensity);
	shader->clearBlur();
	shader->setTextureAndSampler(static_cast<GX2Texture *>(texturePtr), &sampler);
	shader->draw(GX2_PRIMITIVE_MODE_QUADS, 4);
}

void WutGlyphRenderer::drawFeature(int16_t screenX, int16_t screenY, uint16_t width, uint16_t height, const PixelColor& color)
{
	float offset[3];
	float scale[3];
	PixelRectToNdc(screenX, screenY, width, height, 1.0f, 1.0f, kDesignWidth, kDesignHeight, offset, scale);

	uint8_t colorVtxs[ColorShader::cuColorVtxsSize];
	for(int i = 0; i < 4; i++)
	{
		colorVtxs[i * 4 + 0] = color.r;
		colorVtxs[i * 4 + 1] = color.g;
		colorVtxs[i * 4 + 2] = color.b;
		colorVtxs[i * 4 + 3] = color.a;
	}
	float colorIntensity[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	ColorShader * shader = ColorShader::instance();
	shader->setShaders();
	shader->setAttributeBuffer(colorVtxs);
	shader->setAngle(0.0f);
	shader->setOffset(offset);
	shader->setScale(scale);
	shader->setColorIntensity(colorIntensity);
	shader->draw(GX2_PRIMITIVE_MODE_QUADS, 4);
}
