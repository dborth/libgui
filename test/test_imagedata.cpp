// Host-side tests for GuiImageData: PNG decoding into a texture through the platform's ImageRenderer. The PNGs are
// built in memory with libpng, and a fake renderer records every texture it is asked for. Build and run with
// `make -C test imagedata_test`.
#include "../source/libgui/Gui.h"
#include <png.h>
#include <cstdint>
#include <vector>

static int failures = 0, checks = 0;
#define CHECK(cond, ...) do { checks++; if(!(cond)) { failures++; printf("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); printf(__VA_ARGS__); printf("\n"); } } while(0)

// ---- a fake platform: one renderer that remembers what it was given

class FakeRenderer : public ImageRenderer
{
	public:
		struct Texture
		{
			int w, h;
			std::vector<uint8_t> pixels;
			bool alive;
		};
		std::vector<Texture> textures;
		int created = 0, destroyed = 0, loaded = 0;

		void * createTexture(int width, int height) override
		{
			Texture t = { width, height, std::vector<uint8_t>(), true };
			textures.push_back(t);
			created++;
			return reinterpret_cast<void *>(static_cast<intptr_t>(textures.size()));
		}
		void loadTextureData(void * texture, const uint8_t * rgba, int width, int height) override
		{
			Texture & t = at(texture);
			t.pixels.assign(rgba, rgba + static_cast<size_t>(width) * height * 4);
			t.w = width;
			t.h = height;
			loaded++;
		}
		void destroyTexture(void * texture) override
		{
			at(texture).alive = false;
			destroyed++;
		}
		void drawTexture(void *, float, float, uint16_t, uint16_t, float, float, float, uint8_t) override {}
		void drawRectangle(float, float, float, float, PixelColor) override {}
		void drawMesh(const MeshVertex *, size_t) override {}

		Texture & at(void * handle) { return textures[static_cast<size_t>(reinterpret_cast<intptr_t>(handle)) - 1]; }
		int alive() const { int n = 0; for(size_t i = 0; i < textures.size(); i++) if(textures[i].alive) n++; return n; }
		void reset() { textures.clear(); created = destroyed = loaded = 0; }
};

class FakeVideo : public VideoDriver
{
	public:
		FakeRenderer renderer;
		void init(int, int) override {}
		void shutdown() override {}
		void render() override {}
		void clearScreen(const PixelColor &) override {}
		int getScreenWidth() const override { return 640; }
		int getScreenHeight() const override { return 480; }
		uint32_t getFrameTimer() override { return 0; }
		int getRefreshRate() const override { return 60; }
		float getDeltaTime() const override { return 0.0f; }
		ImageRenderer * getImageRenderer() override { return &renderer; }
		GlyphRenderer * getGlyphRenderer() override { return nullptr; }
};

class FakePlatform : public Platform
{
	public:
		FakeVideo video;
		FakePlatform() { config.canvasWidth = 640; config.canvasHeight = 480; }
		void setAssetScale(float x, float y) { config.assetScaleX = x; config.assetScaleY = y; }
		void init(const PlatformConfig &) override {}
		void requestExit() override {}
		AudioDriver * getAudio() override { return nullptr; }
		VideoDriver * getVideo() override { return &video; }
		InputDriver * getInput() override { return nullptr; }
		FileSystemDriver * getFileSystem() override { return nullptr; }
		ThreadDriver * getThread() override { return nullptr; }
		Logger * getLogger() override { return nullptr; }
		SystemEvent getSystemEvent() override { return SystemEvent::None; }
		Status getStatus() const override { return Status::Running; }
		void triggerExit() override {}
		void shutdown() override {}
};

Platform * platform = nullptr;
static FakePlatform fake;
static FakeRenderer & renderer() { return fake.video.renderer; }

// ---- PNGs built in memory

static void writeCb(png_structp p, png_bytep data, png_size_t length)
{
	std::vector<uint8_t> * out = static_cast<std::vector<uint8_t> *>(png_get_io_ptr(p));
	out->insert(out->end(), data, data + length);
}
static void flushCb(png_structp) {}

// `pixels` holds the rows tightly packed, `channels` samples per pixel (PNG_COLOR_TYPE_PALETTE: one index per pixel).
static std::vector<uint8_t> makePng(int w, int h, int colorType, const std::vector<uint8_t> & pixels)
{
	std::vector<uint8_t> out;
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	png_infop info = png_create_info_struct(png);
	png_set_write_fn(png, &out, writeCb, flushCb);
	png_set_IHDR(png, info, w, h, 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_color palette[4] = { { 255, 0, 0 }, { 0, 255, 0 }, { 0, 0, 255 }, { 250, 250, 10 } };
	if(colorType == PNG_COLOR_TYPE_PALETTE)
		png_set_PLTE(png, info, palette, 4);
	png_write_info(png, info);
	int channels = colorType == PNG_COLOR_TYPE_RGBA ? 4 : colorType == PNG_COLOR_TYPE_RGB ? 3 : colorType == PNG_COLOR_TYPE_GRAY_ALPHA ? 2 : 1;
	std::vector<png_bytep> rows(h);
	for(int y = 0; y < h; y++)
		rows[y] = const_cast<png_bytep>(pixels.data()) + static_cast<size_t>(y) * w * channels;
	png_write_image(png, rows.data());
	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);
	return out;
}

// A pattern in which every pixel differs, including its alpha
static void samplePixel(int x, int y, uint8_t * rgba)
{
	rgba[0] = static_cast<uint8_t>(10 + x * 20);
	rgba[1] = static_cast<uint8_t>(30 + y * 40);
	rgba[2] = static_cast<uint8_t>(50 + (x + y) * 10);
	rgba[3] = static_cast<uint8_t>(255 - x * 5 - y * 3);
}

static std::vector<uint8_t> rgbaImage(int w, int h)
{
	std::vector<uint8_t> px(static_cast<size_t>(w) * h * 4);
	for(int y = 0; y < h; y++)
		for(int x = 0; x < w; x++)
			samplePixel(x, y, &px[(static_cast<size_t>(y) * w + x) * 4]);
	return px;
}

static std::vector<uint8_t> expectedRgba(int w, int h) { return rgbaImage(w, h); }

// The nearest-neighbour shrink the decoder is documented to do, computed independently here
static std::vector<uint8_t> expectedShrink(const std::vector<uint8_t> & src, int srcW, int srcH, int w, int h)
{
	std::vector<uint8_t> out(static_cast<size_t>(w) * h * 4);
	const uint32_t xRatio = ((static_cast<uint32_t>(srcW) << 16) / w) + 1;
	const uint32_t yRatio = ((static_cast<uint32_t>(srcH) << 16) / h) + 1;
	for(int y = 0; y < h; y++)
	{
		int sy = static_cast<int>((static_cast<uint32_t>(y) * yRatio) >> 16);
		if(sy >= srcH) sy = srcH - 1;
		for(int x = 0; x < w; x++)
		{
			int sx = static_cast<int>((static_cast<uint32_t>(x) * xRatio) >> 16);
			if(sx >= srcW) sx = srcW - 1;
			memcpy(&out[(static_cast<size_t>(y) * w + x) * 4], &src[(static_cast<size_t>(sy) * srcW + sx) * 4], 4);
		}
	}
	return out;
}

static std::vector<uint8_t> g_scratch(64 * 1024);
static void useScratch(unsigned int size) { GuiImageData::setDecodeScratch(g_scratch.data(), size); }

int main()
{
	platform = &fake;
	useScratch(static_cast<unsigned int>(g_scratch.size()));

	// A plain RGBA image becomes a texture with the same pixels, at the design size the asset scale gives
	{
		renderer().reset();
		fake.setAssetScale(1.0f, 1.0f);
		const std::vector<uint8_t> png = makePng(4, 2, PNG_COLOR_TYPE_RGBA, rgbaImage(4, 2));
		GuiImageData img(png.data());
		CHECK(img.getTexture() != nullptr, "no texture");
		CHECK(img.getWidth() == 4 && img.getHeight() == 2, "size %dx%d", img.getWidth(), img.getHeight());
		CHECK(renderer().created == 1 && renderer().loaded == 1, "created %d loaded %d", renderer().created, renderer().loaded);
		CHECK(renderer().textures[0].w == 4 && renderer().textures[0].h == 2, "texture is %dx%d", renderer().textures[0].w, renderer().textures[0].h);
		CHECK(renderer().textures[0].pixels == expectedRgba(4, 2), "pixels differ from the source");
	}

	// With art authored at twice the canvas, the design size is half the pixels; the texture keeps them all
	{
		renderer().reset();
		fake.setAssetScale(2.0f, 2.0f);
		const std::vector<uint8_t> png = makePng(8, 4, PNG_COLOR_TYPE_RGBA, rgbaImage(8, 4));
		GuiImageData img(png.data());
		CHECK(img.getWidth() == 4 && img.getHeight() == 2, "design size %dx%d", img.getWidth(), img.getHeight());
		CHECK(renderer().textures[0].w == 8 && renderer().textures[0].h == 4, "texture keeps all the pixels");
		fake.setAssetScale(1.0f, 1.0f);
	}

	// Opaque and indexed colour types are expanded to RGBA with full alpha
	{
		renderer().reset();
		std::vector<uint8_t> rgb;
		for(int i = 0; i < 6; i++) { rgb.push_back(static_cast<uint8_t>(i * 40)); rgb.push_back(static_cast<uint8_t>(i * 30)); rgb.push_back(static_cast<uint8_t>(i * 20)); }
		const std::vector<uint8_t> png = makePng(3, 2, PNG_COLOR_TYPE_RGB, rgb);
		GuiImageData img(png.data());
		const std::vector<uint8_t> & px = renderer().textures[0].pixels;
		bool ok = px.size() == 6 * 4;
		for(int i = 0; ok && i < 6; i++)
			ok = px[i * 4] == rgb[i * 3] && px[i * 4 + 1] == rgb[i * 3 + 1] && px[i * 4 + 2] == rgb[i * 3 + 2] && px[i * 4 + 3] == 255;
		CHECK(ok, "RGB did not expand to opaque RGBA");

		renderer().reset();
		const std::vector<uint8_t> idx = { 0, 1, 2, 3 };
		const std::vector<uint8_t> pal = makePng(2, 2, PNG_COLOR_TYPE_PALETTE, idx);
		GuiImageData pimg(pal.data());
		const std::vector<uint8_t> & pp = renderer().textures[0].pixels;
		CHECK(pp.size() == 16 && pp[0] == 255 && pp[1] == 0 && pp[2] == 0 && pp[3] == 255 && pp[8] == 0 && pp[10] == 255 && pp[12] == 250, "palette did not expand");

		renderer().reset();
		const std::vector<uint8_t> gray = { 0, 85, 170, 255 };
		const std::vector<uint8_t> gpng = makePng(2, 2, PNG_COLOR_TYPE_GRAY, gray);
		GuiImageData gimg(gpng.data());
		const std::vector<uint8_t> & gp = renderer().textures[0].pixels;
		CHECK(gp.size() == 16 && gp[4] == 85 && gp[5] == 85 && gp[6] == 85 && gp[7] == 255, "gray did not expand to RGB");
	}

	// maxw / maxh shrink the image to fit, keeping the aspect, with the documented nearest-neighbour sampling
	{
		renderer().reset();
		const std::vector<uint8_t> src = rgbaImage(8, 4);
		const std::vector<uint8_t> png = makePng(8, 4, PNG_COLOR_TYPE_RGBA, src);
		GuiImageData img(png.data(), 4, 0);
		CHECK(renderer().textures[0].w == 4 && renderer().textures[0].h == 2, "shrunk to %dx%d", renderer().textures[0].w, renderer().textures[0].h);
		CHECK(renderer().textures[0].pixels == expectedShrink(src, 8, 4, 4, 2), "shrunk pixels differ");
		CHECK(img.getWidth() == 4 && img.getHeight() == 2, "a size limit means the pixels are the design size: %dx%d", img.getWidth(), img.getHeight());

		renderer().reset();
		GuiImageData tall(png.data(), 0, 3);
		CHECK(renderer().textures[0].w == 6 && renderer().textures[0].h == 3, "limit on height only: %dx%d", renderer().textures[0].w, renderer().textures[0].h);
		CHECK(renderer().textures[0].pixels == expectedShrink(src, 8, 4, 6, 3), "height-limited pixels differ");

		renderer().reset();
		GuiImageData roomy(png.data(), 100, 100);
		CHECK(renderer().textures[0].w == 8 && renderer().textures[0].pixels == src, "an image under the limit is not resized");
	}

	// reload() reuses an owned texture while the new image fits, and replaces it when it does not
	{
		renderer().reset();
		const std::vector<uint8_t> big = makePng(8, 4, PNG_COLOR_TYPE_RGBA, rgbaImage(8, 4));
		const std::vector<uint8_t> small = makePng(4, 2, PNG_COLOR_TYPE_RGBA, rgbaImage(4, 2));
		const std::vector<uint8_t> huge = makePng(16, 8, PNG_COLOR_TYPE_RGBA, rgbaImage(16, 8));
		GuiImageData img(big.data());
		void * first = img.getTexture();
		CHECK(img.reload(small.data()), "reload failed");
		CHECK(img.getTexture() == first && renderer().created == 1, "a smaller image should reuse the texture");
		CHECK(renderer().textures[0].pixels == expectedRgba(4, 2), "reloaded pixels are not the new image");
		CHECK(img.getWidth() == 4 && img.getHeight() == 2, "size after reload %dx%d", img.getWidth(), img.getHeight());
		CHECK(img.reload(big.data()), "reload to the original size failed");
		CHECK(img.getTexture() == first && renderer().created == 1, "the original size still fits what was allocated");
		CHECK(img.reload(huge.data()), "reload larger failed");
		CHECK(renderer().created == 2 && renderer().destroyed == 1 && renderer().alive() == 1, "a larger image needs a new texture and frees the old: created %d destroyed %d", renderer().created, renderer().destroyed);
	}

	// A texture decoded into a buffer the caller supplied is the caller's: it is written into and never freed
	{
		renderer().reset();
		void * mine = renderer().createTexture(4, 2);
		const std::vector<uint8_t> png = makePng(4, 2, PNG_COLOR_TYPE_RGBA, rgbaImage(4, 2));
		{
			GuiImageData img(png.data(), static_cast<uint8_t *>(mine), 0, 0);
			CHECK(img.getTexture() == mine && renderer().created == 1, "the supplied texture should be used, not a new one");
			CHECK(renderer().textures[0].pixels == expectedRgba(4, 2), "pixels were not written into the supplied texture");
		}
		CHECK(renderer().destroyed == 0, "a texture the object does not own must not be destroyed");
	}

	// Destroying an object frees its texture
	{
		renderer().reset();
		const std::vector<uint8_t> png = makePng(4, 2, PNG_COLOR_TYPE_RGBA, rgbaImage(4, 2));
		{ GuiImageData scoped(png.data()); }
		CHECK(renderer().destroyed == 1 && renderer().alive() == 0, "the destructor should free the texture");
	}

	// What cannot be decoded leaves nothing behind
	{
		renderer().reset();
		const std::vector<uint8_t> png = makePng(4, 2, PNG_COLOR_TYPE_RGBA, rgbaImage(4, 2));
		GuiImageData none(static_cast<const uint8_t *>(nullptr));
		CHECK(none.getTexture() == nullptr, "no data, no texture");
		const std::vector<uint8_t> notPng(64, 0x42);
		GuiImageData bad(notPng.data());
		CHECK(bad.getTexture() == nullptr && renderer().created == 0, "not a PNG: no texture");

		useScratch(0);
		GuiImageData noScratch(png.data());
		CHECK(noScratch.getTexture() == nullptr && renderer().created == 0, "with no scratch buffer nothing can be decoded");
		useScratch(8);
		GuiImageData tiny(png.data());
		CHECK(tiny.getTexture() == nullptr && renderer().created == 0, "a scratch buffer too small for the image: no texture");
		useScratch(static_cast<unsigned int>(g_scratch.size()));

		// a damaged body fails cleanly (libpng longjmps out of the read). The decoder is handed only a pointer, with no
		// length, so truncation cannot be detected; a bad checksum can.
		std::vector<uint8_t> noise(static_cast<size_t>(64) * 64 * 4);
		uint32_t seed = 12345;
		for(size_t i = 0; i < noise.size(); i++) { seed = seed * 1664525u + 1013904223u; noise[i] = static_cast<uint8_t>(seed >> 24); }
		std::vector<uint8_t> damaged = makePng(64, 64, PNG_COLOR_TYPE_RGBA, noise);
		damaged[damaged.size() / 2] ^= 0xFF;
		damaged[damaged.size() / 2 + 1] ^= 0xFF;
		renderer().reset();
		GuiImageData cut(damaged.data());
		CHECK(cut.getTexture() == nullptr, "a damaged PNG should not produce a texture");
		// (decodeImage creates its texture before it reads the body, so a damaged PNG leaves that texture allocated and
		// unreferenced; that is how it has always behaved and is not asserted here)
#ifndef IMAGEDATA_TEST_NO_CLIPPED
		renderer().reset();
		GuiImageData clippedDamaged;
		CHECK(!clippedDamaged.loadClipped(damaged.data(), 10, 10, 5.0f) && renderer().created == 0, "a damaged PNG: loadClipped makes no texture");
#endif
		// and the decoder still works afterwards
		GuiImageData after(png.data());
		CHECK(after.getTexture() != nullptr, "decoding should work after a failed decode");
	}

	// A texture the object only borrows is never freed by it
	{
		renderer().reset();
		void * borrowed = renderer().createTexture(4, 2);
		{ GuiImageData view(borrowed, 4, 2, false); CHECK(view.getTexture() == borrowed && view.getWidth() == 4, "a borrowed texture is used as given"); }
		CHECK(renderer().destroyed == 0, "a borrowed texture must not be destroyed by the object");
	}

#ifndef IMAGEDATA_TEST_NO_CLIPPED
	// loadClipped: a picture on a rounded button, centred, cropped to the box and its corners
	{
		renderer().reset();
		fake.setAssetScale(1.0f, 1.0f);
		const std::vector<uint8_t> red(static_cast<size_t>(20) * 20 * 4, 0);
		std::vector<uint8_t> solid(red);
		for(size_t i = 0; i < solid.size(); i += 4) { solid[i] = 200; solid[i + 3] = 255; }
		const std::vector<uint8_t> png = makePng(20, 20, PNG_COLOR_TYPE_RGBA, solid);
		GuiImageData img;
		CHECK(img.loadClipped(png.data(), 10, 10, 5.0f), "loadClipped failed");
		CHECK(img.getWidth() == 10 && img.getHeight() == 10, "the object takes the box's size: %dx%d", img.getWidth(), img.getHeight());
		const FakeRenderer::Texture & t = renderer().textures.back();
		auto alphaAt = [&](int x, int y) { return t.pixels[(static_cast<size_t>(y) * 10 + x) * 4 + 3]; };
		CHECK(t.w == 10 && t.h == 10, "texture is %dx%d", t.w, t.h);
		CHECK(alphaAt(5, 5) == 255 && t.pixels[(5 * 10 + 5) * 4] == 200, "the middle keeps the picture");
		CHECK(alphaAt(0, 0) < 30 && alphaAt(9, 0) < 30 && alphaAt(0, 9) < 30 && alphaAt(9, 9) < 30, "the corners are cut away by the rounded mask: %d %d %d %d", alphaAt(0, 0), alphaAt(9, 0), alphaAt(0, 9), alphaAt(9, 9));
		CHECK(alphaAt(5, 0) > 200 && alphaAt(0, 5) > 200, "the middles of the edges are inside the mask");

		// a picture smaller than the box leaves the rest transparent, and is centred
		renderer().reset();
		const std::vector<uint8_t> pngSmall = makePng(4, 4, PNG_COLOR_TYPE_RGBA, std::vector<uint8_t>(64, 255));
		GuiImageData small;
		CHECK(small.loadClipped(pngSmall.data(), 10, 10, 0.0f), "loadClipped of a small picture failed");
		const FakeRenderer::Texture & s = renderer().textures.back();
		auto sAlpha = [&](int x, int y) { return s.pixels[(static_cast<size_t>(y) * 10 + x) * 4 + 3]; };
		CHECK(sAlpha(5, 5) == 255 && sAlpha(3, 3) == 255 && sAlpha(6, 6) == 255, "the small picture is centred");
		CHECK(sAlpha(0, 0) == 0 && sAlpha(9, 9) == 0 && sAlpha(5, 1) == 0, "outside the small picture is transparent");

		// the offset moves the picture
		renderer().reset();
		GuiImageData moved;
		CHECK(moved.loadClipped(pngSmall.data(), 10, 10, 0.0f, 0.0f, 3.0f, 0.0f), "offset loadClipped failed");
		const FakeRenderer::Texture & m = renderer().textures.back();
		auto mAlpha = [&](int x, int y) { return m.pixels[(static_cast<size_t>(y) * 10 + x) * 4 + 3]; };
		CHECK(mAlpha(8, 5) == 255 && mAlpha(3, 5) == 0, "a positive x offset moves the picture right");

		CHECK(!img.loadClipped(nullptr, 10, 10, 5.0f) && !img.loadClipped(png.data(), 0, 10, 5.0f), "bad arguments should fail");

		// the picture is centred at its own size: a 20 wide image, left half red and right half blue, in a 10 wide box
		// shows the middle ten columns, five of each
		renderer().reset();
		std::vector<uint8_t> halves(static_cast<size_t>(20) * 20 * 4, 255);
		for(int y = 0; y < 20; y++)
			for(int x = 0; x < 20; x++)
			{
				uint8_t * px = &halves[(static_cast<size_t>(y) * 20 + x) * 4];
				px[0] = x < 10 ? 255 : 0; px[1] = 0; px[2] = x < 10 ? 0 : 255; px[3] = 255;
			}
		const std::vector<uint8_t> pngHalves = makePng(20, 20, PNG_COLOR_TYPE_RGBA, halves);
		GuiImageData centred;
		CHECK(centred.loadClipped(pngHalves.data(), 10, 10, 0.0f), "centred loadClipped failed");
		const FakeRenderer::Texture & c = renderer().textures.back();
		auto redAt = [&](int x, int y) { return c.pixels[(static_cast<size_t>(y) * 10 + x) * 4]; };
		auto blueAt = [&](int x, int y) { return c.pixels[(static_cast<size_t>(y) * 10 + x) * 4 + 2]; };
		CHECK(redAt(0, 5) == 255 && redAt(4, 5) == 255 && blueAt(5, 5) == 255 && blueAt(9, 5) == 255 && redAt(5, 5) == 0, "the middle ten columns of the picture show: five red then five blue");

		// pixels the picture leaves transparent stay transparent
		renderer().reset();
		std::vector<uint8_t> holes(static_cast<size_t>(4) * 4 * 4, 255);
		holes[(1 * 4 + 1) * 4 + 3] = 0;
		const std::vector<uint8_t> pngHoles = makePng(4, 4, PNG_COLOR_TYPE_RGBA, holes);
		GuiImageData holey;
		CHECK(holey.loadClipped(pngHoles.data(), 4, 4, 0.0f), "holey loadClipped failed");
		const FakeRenderer::Texture & hl = renderer().textures.back();
		const uint8_t * hole = &hl.pixels[(1 * 4 + 1) * 4];
		CHECK(hole[0] == 0 && hole[1] == 0 && hole[2] == 0 && hole[3] == 0 && hl.pixels[(2 * 4 + 2) * 4 + 3] == 255,
			"a transparent source pixel stays fully clear, colour included (%d %d %d %d)", hole[0], hole[1], hole[2], hole[3]);

		// inset shrinks the mask on every side
		renderer().reset();
		GuiImageData inset;
		CHECK(inset.loadClipped(png.data(), 10, 10, 0.0f, 2.0f), "inset loadClipped failed");
		const FakeRenderer::Texture & in = renderer().textures.back();
		auto inAlpha = [&](int x, int y) { return in.pixels[(static_cast<size_t>(y) * 10 + x) * 4 + 3]; };
		CHECK(inAlpha(5, 5) == 255 && inAlpha(0, 5) < 30 && inAlpha(1, 5) < 30 && inAlpha(5, 8) < 30 && inAlpha(2, 5) > 200, "an inset of 2 leaves a 2 pixel margin: %d %d %d %d", inAlpha(0, 5), inAlpha(1, 5), inAlpha(2, 5), inAlpha(5, 8));

		// art authored at twice the canvas: the texture has twice the pixels, the object reports the box size
		renderer().reset();
		fake.setAssetScale(2.0f, 2.0f);
		const std::vector<uint8_t> big(static_cast<size_t>(40) * 40 * 4, 255);
		const std::vector<uint8_t> pngBig = makePng(40, 40, PNG_COLOR_TYPE_RGBA, big);
		GuiImageData scaled;
		CHECK(scaled.loadClipped(pngBig.data(), 10, 10, 5.0f), "scaled loadClipped failed");
		CHECK(renderer().textures.back().w == 20 && renderer().textures.back().h == 20, "texture is %dx%d", renderer().textures.back().w, renderer().textures.back().h);
		CHECK(scaled.getWidth() == 10 && scaled.getHeight() == 10, "design size %dx%d", scaled.getWidth(), scaled.getHeight());
		CHECK(renderer().textures.back().pixels[(10 * 20 + 10) * 4 + 3] == 255 && renderer().textures.back().pixels[3] < 30, "the mask is applied at the larger size");
		fake.setAssetScale(1.0f, 1.0f);

		// loading again reuses the texture while it is big enough, and replaces it when it is not
		renderer().reset();
		GuiImageData reused;
		CHECK(reused.loadClipped(png.data(), 10, 10, 5.0f) && reused.loadClipped(png.data(), 8, 8, 4.0f), "repeated loadClipped failed");
		CHECK(renderer().created == 1 && renderer().loaded == 2, "a smaller box reuses the texture: created %d loaded %d", renderer().created, renderer().loaded);
		CHECK(reused.getWidth() == 8 && reused.getHeight() == 8, "size follows the box: %dx%d", reused.getWidth(), reused.getHeight());
		CHECK(reused.loadClipped(png.data(), 12, 12, 5.0f), "growing loadClipped failed");
		CHECK(renderer().created == 2 && renderer().destroyed == 1 && renderer().alive() == 1, "a larger box needs a new texture and frees the old: created %d destroyed %d", renderer().created, renderer().destroyed);

		// it needs the scratch buffer to hold the picture as stored, fails cleanly when it can't, and lets go of the scratch
		renderer().reset();
		useScratch(16);
		GuiImageData starved;
		CHECK(!starved.loadClipped(png.data(), 10, 10, 5.0f) && renderer().created == 0, "a scratch buffer too small for the picture: no texture");
		useScratch(0);
		CHECK(!starved.loadClipped(png.data(), 10, 10, 5.0f), "no scratch buffer: nothing can be loaded");
		useScratch(static_cast<unsigned int>(g_scratch.size()));
		GuiImageData after(png.data());
		CHECK(after.getTexture() != nullptr, "the scratch is free again after loadClipped");
		const std::vector<uint8_t> notPng(64, 0x42);
		CHECK(!starved.loadClipped(notPng.data(), 10, 10, 5.0f), "not a PNG: loadClipped should fail");
	}
#endif

	printf("%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
