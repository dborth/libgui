/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiImageData.cpp
 ***************************************************************************/

#include <png.h>
#include <setjmp.h>
#include <cmath>
#include <cstdlib>
#include <memory>

#include "Gui.h"

namespace {
	uint8_t * scratchBuffer = nullptr;
	unsigned int scratchBufferSize = 0;

	Mutex & getScratchMutex()
	{
		static Mutex scratchMutex;
		return scratchMutex;
	}

	void ErrorCb(png_structp png_ptr, png_const_charp) { longjmp(png_jmpbuf(png_ptr), 1); }
	void WarningCb(png_structp, png_const_charp) {}
}

void GuiImageData::setDecodeScratch(void * buffer, unsigned int size)
{
	scratchBuffer = static_cast<uint8_t *>(buffer);
	scratchBufferSize = buffer ? size : 0;
}

Mutex & GuiImageData::scratchLock()
{
	return getScratchMutex();
}

GuiImageData::GuiImageData()
{
	texture = nullptr;
	width = 0;
	height = 0;
	ownsTexture = false;
	capWidth = 0;
	capHeight = 0;
}

GuiImageData::GuiImageData(const uint8_t * i, int maxw, int maxh)
{
	texture = nullptr;
	width = 0;
	height = 0;
	ownsTexture = false;
	capWidth = 0;
	capHeight = 0;

	if(i)
		decodeImage(i, &width, &height, maxw, maxh);
}

GuiImageData::GuiImageData(const uint8_t * i, uint8_t * dst, int maxw, int maxh)
{
	texture = dst;
	width = 0;
	height = 0;
	ownsTexture = false;
	capWidth = 0;
	capHeight = 0;

	if(i) {
		decodeImage(i, &width, &height, maxw, maxh);
	}
}

GuiImageData::GuiImageData(void * tex, int w, int h, bool takeOwnership)
{
	texture = tex;
	width = w;
	height = h;
	ownsTexture = takeOwnership && tex;
	capWidth = ownsTexture ? w : 0;
	capHeight = ownsTexture ? h : 0;
}

GuiImageData::~GuiImageData()
{
	if(ownsTexture && texture)
	{
		platform->getVideo()->getImageRenderer()->destroyTexture(texture);
	}
	texture = nullptr;
}

//!Read cursor over an in-memory PNG, used as the libpng read callback's I/O pointer.
struct PngMemoryData
{
	const uint8_t * data; //!< Start of the PNG file in memory
	size_t offset;        //!< Bytes consumed so far
};

static void ReadPngDataCb(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PngMemoryData * memData = static_cast<PngMemoryData *>(png_get_io_ptr(png_ptr));
	if(!memData)
		return;

	memcpy(data, memData->data + memData->offset, length);
	memData->offset += length;
}

bool GuiImageData::reload(const uint8_t * pngData, int maxw, int maxh)
{
	if(!pngData)
		return false;

	int w = 0, h = 0;
	return decodeImage(pngData, &w, &h, maxw, maxh);
}

bool GuiImageData::decodeImage(const uint8_t * pngData, int * outWidth, int * outHeight, int maxw, int maxh)
{
	if(!pngData)
		return false;

	MutexLock scratchGuard(getScratchMutex());
	uint8_t * const localScratchBuffer = scratchBuffer;
	const unsigned int localScratchBufferSize = scratchBufferSize;

	if(!localScratchBuffer || localScratchBufferSize == 0)
		return false;

	if(png_sig_cmp(static_cast<png_const_bytep>(pngData), 0, 8))
		return false;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, ErrorCb, WarningCb);
	if(!png_ptr)
		return false;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		return false;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		return false;
	}

	PngMemoryData memData = { pngData, 0 };
	png_set_read_fn(png_ptr, &memData, ReadPngDataCb);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 srcW, srcH;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &srcW, &srcH, &bit_depth, &color_type, &interlace_type, nullptr, nullptr);

	png_uint_32 w = srcW, h = srcH;
	bool needsResize = (maxw > 0 && static_cast<int>(srcW) > maxw) || (maxh > 0 && static_cast<int>(srcH) > maxh);
	if(needsResize)
	{
		double wScale = maxw > 0 ? static_cast<double>(maxw) / srcW : 1e30;
		double hScale = maxh > 0 ? static_cast<double>(maxh) / srcH : 1e30;
		double scale = wScale < hScale ? wScale : hScale;
		w = static_cast<png_uint_32>(srcW * scale);
		h = static_cast<png_uint_32>(srcH * scale);
		if(w < 1) w = 1;
		if(h < 1) h = 1;
	}

	if(bit_depth == 16)
		png_set_strip_16(png_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);
	unsigned int rowBytes = png_get_rowbytes(png_ptr, info_ptr);

	unsigned long long rowPtrBytes = static_cast<unsigned long long>(srcH) * sizeof(png_bytep);
	unsigned long long srcRgbaBytes = static_cast<unsigned long long>(rowBytes) * srcH;
	unsigned long long resizedRgbaBytes = needsResize ? static_cast<unsigned long long>(w) * h * 4 : 0;
	unsigned long long totalScratchBytes = rowPtrBytes + srcRgbaBytes + resizedRgbaBytes;
	if(totalScratchBytes > localScratchBufferSize)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		return false;
	}

	bool haveUsableTexture = texture && (!ownsTexture || (static_cast<int>(w) <= capWidth && static_cast<int>(h) <= capHeight));

	void * newTexture = texture;
	if(!haveUsableTexture)
	{
		newTexture = platform->getVideo()->getImageRenderer()->createTexture(w, h);
		if(!newTexture)
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
			return false;
		}
	}

	png_bytep * row_pointers = reinterpret_cast<png_bytep *>(localScratchBuffer);
	uint8_t * srcRgba = localScratchBuffer + rowPtrBytes;

	for(png_uint_32 i = 0; i < srcH; i++)
		row_pointers[i] = srcRgba + (static_cast<size_t>(i) * rowBytes);

	png_read_image(png_ptr, row_pointers);

	const uint8_t * finalRgba = srcRgba;
	if(needsResize)
	{
		uint8_t * resizedRgba = srcRgba + srcRgbaBytes;
		uint32_t xRatio = ((srcW << 16) / w) + 1;
		uint32_t yRatio = ((srcH << 16) / h) + 1;

		for(png_uint_32 y = 0; y < h; y++)
		{
			png_uint_32 sy = (y * yRatio) >> 16;
			if(sy >= srcH) sy = srcH - 1;
			const uint8_t * srcRow = srcRgba + static_cast<size_t>(sy) * rowBytes;
			uint8_t * dstRow = resizedRgba + static_cast<size_t>(y) * w * 4;

			for(png_uint_32 x = 0; x < w; x++)
			{
				png_uint_32 sx = (x * xRatio) >> 16;
				if(sx >= srcW) sx = srcW - 1;
				memcpy(dstRow + x * 4, srcRow + sx * 4, 4);
			}
		}

		finalRgba = resizedRgba;
	}

	platform->getVideo()->getImageRenderer()->loadTextureData(newTexture, finalRgba, w, h);

	if(!haveUsableTexture)
	{
		if(ownsTexture && texture)
			platform->getVideo()->getImageRenderer()->destroyTexture(texture);
		texture = newTexture;
		ownsTexture = true;
		capWidth = w;
		capHeight = h;
	}

	int designW = static_cast<int>(w);
	int designH = static_cast<int>(h);
	if(maxw == 0 && maxh == 0)
	{
		const PlatformConfig& config = platform->getConfig();
		designW = static_cast<int>(w / config.assetScaleX + 0.5f);
		designH = static_cast<int>(h / config.assetScaleY + 0.5f);
		if(designW < 1) designW = 1;
		if(designH < 1) designH = 1;
	}

	width = designW;
	height = designH;
	if(outWidth) *outWidth = designW;
	if(outHeight) *outHeight = designH;

	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
	return true;
}

//!Decodes a PNG at its own size into tightly packed RGBA8 rows in the shared scratch buffer, for loadClipped().
//!The caller holds the scratch mutex; the pixels are valid until it is released.
static const uint8_t * decodeToScratch(const uint8_t * pngData, int * outW, int * outH)
{
	uint8_t * const localScratchBuffer = scratchBuffer;
	const unsigned int localScratchBufferSize = scratchBufferSize;

	if(!pngData || !localScratchBuffer || localScratchBufferSize == 0)
		return nullptr;

	if(png_sig_cmp(static_cast<png_const_bytep>(pngData), 0, 8))
		return nullptr;

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, ErrorCb, WarningCb);
	if(!png_ptr)
		return nullptr;

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		return nullptr;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		return nullptr;
	}

	PngMemoryData memData = { pngData, 0 };
	png_set_read_fn(png_ptr, &memData, ReadPngDataCb);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 srcW, srcH;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &srcW, &srcH, &bit_depth, &color_type, &interlace_type, nullptr, nullptr);

	if(bit_depth == 16)
		png_set_strip_16(png_ptr);
	if(color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	if(color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	if(color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);
	unsigned int rowBytes = png_get_rowbytes(png_ptr, info_ptr);

	unsigned long long rowPtrBytes = static_cast<unsigned long long>(srcH) * sizeof(png_bytep);
	unsigned long long srcRgbaBytes = static_cast<unsigned long long>(rowBytes) * srcH;
	if(rowPtrBytes + srcRgbaBytes > localScratchBufferSize)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		return nullptr;
	}

	png_bytep * row_pointers = reinterpret_cast<png_bytep *>(localScratchBuffer);
	uint8_t * srcRgba = localScratchBuffer + rowPtrBytes;

	for(png_uint_32 i = 0; i < srcH; i++)
		row_pointers[i] = srcRgba + (static_cast<size_t>(i) * rowBytes);

	png_read_image(png_ptr, row_pointers);
	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

	*outW = static_cast<int>(srcW);
	*outH = static_cast<int>(srcH);
	return srcRgba;
}

bool GuiImageData::loadClipped(const uint8_t * pngData, int boxW, int boxH, float radius, float inset, float offsetX, float offsetY)
{
	if(!pngData || boxW < 1 || boxH < 1)
		return false;

	// Work in the decoded image's own pixels (which are assetScale times the design size),
	// but evaluate the mask in design pixels so the corner stays round when the two axes
	// are scaled differently.
	const PlatformConfig& config = platform->getConfig();
	const float sx = config.assetScaleX;
	const float sy = config.assetScaleY;
	const int dstW = static_cast<int>(boxW * sx + 0.5f);
	const int dstH = static_cast<int>(boxH * sy + 0.5f);

	std::unique_ptr<uint8_t, decltype(&free)> out(static_cast<uint8_t *>(calloc(static_cast<size_t>(dstW) * dstH, 4)), free);
	if(!out)
		return false;

	{
		MutexLock scratchGuard(getScratchMutex());

		int srcWidth = 0, srcHeight = 0;
		const uint8_t * srcPixels = decodeToScratch(pngData, &srcWidth, &srcHeight);
		if(!srcPixels)
			return false;

		// source pixel that lands on destination (0,0): centred, then moved by the offset
		const int shiftX = static_cast<int>(offsetX * sx + (offsetX < 0 ? -0.5f : 0.5f));
		const int shiftY = static_cast<int>(offsetY * sy + (offsetY < 0 ? -0.5f : 0.5f));
		const int originX = (srcWidth - dstW) / 2 - shiftX;
		const int originY = (srcHeight - dstH) / 2 - shiftY;

		// rounded-rectangle signed distance, in design pixels, for a box shrunk by `inset`
		const float halfW = boxW * 0.5f - inset;
		const float halfH = boxH * 0.5f - inset;
		float r = radius - inset;
		if(r > halfW) r = halfW;
		if(r > halfH) r = halfH;
		if(r < 0.0f) r = 0.0f;
		const float fringe = sx < sy ? sx : sy; // physical pixels per design pixel

		for(int y = 0; y < dstH; ++y)
		{
			const int srcY = y + originY;
			if(srcY < 0 || srcY >= srcHeight)
				continue;

			const float qy = fabsf((y + 0.5f) / sy - boxH * 0.5f) - (halfH - r);

			for(int x = 0; x < dstW; ++x)
			{
				const int srcX = x + originX;
				if(srcX < 0 || srcX >= srcWidth)
					continue;

				const uint8_t * s = srcPixels + (static_cast<size_t>(srcY) * srcWidth + srcX) * 4;
				if(s[3] == 0)
					continue;

				const float qx = fabsf((x + 0.5f) / sx - boxW * 0.5f) - (halfW - r);
				const float ox = qx > 0.0f ? qx : 0.0f;
				const float oy = qy > 0.0f ? qy : 0.0f;
				const float inner = qx > qy ? qx : qy;
				const float dist = sqrtf(ox * ox + oy * oy) + (inner < 0.0f ? inner : 0.0f) - r;

				float coverage = 0.5f - dist * fringe;
				if(coverage <= 0.0f)
					continue;
				if(coverage > 1.0f)
					coverage = 1.0f;

				uint8_t * d = out.get() + (static_cast<size_t>(y) * dstW + x) * 4;
				d[0] = s[0];
				d[1] = s[1];
				d[2] = s[2];
				d[3] = static_cast<uint8_t>(s[3] * coverage + 0.5f);
			}
		}
	}

	// the same texture rule decodeImage() follows: reuse an allocation that is big enough, else make a new one
	ImageRenderer * renderer = platform->getVideo()->getImageRenderer();
	const bool haveUsableTexture = texture && (!ownsTexture || (dstW <= capWidth && dstH <= capHeight));

	void * newTexture = texture;
	if(!haveUsableTexture)
	{
		newTexture = renderer->createTexture(dstW, dstH);
		if(!newTexture)
			return false;
	}

	renderer->loadTextureData(newTexture, out.get(), dstW, dstH);

	if(!haveUsableTexture)
	{
		if(ownsTexture && texture)
			renderer->destroyTexture(texture);
		texture = newTexture;
		ownsTexture = true;
		capWidth = dstW;
		capHeight = dstH;
	}

	width = boxW;
	height = boxH;
	return true;
}
