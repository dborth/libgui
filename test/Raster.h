// Tiny software rasteriser + PNG writer for host-side geometry tests.
// One sample per pixel at the pixel centre, straight-alpha "over" blending,
// like the GPU does with no multisampling.
#pragma once
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "../source/drivers/Mesh.h"

struct Image {
	int w, h;
	std::vector<float> px; // RGB 0..255
	Image(int w_, int h_, PixelColor bg) : w(w_), h(h_), px((size_t)w_ * h_ * 3)
	{
		for(int i = 0; i < w * h; i++) { px[i*3] = bg.r; px[i*3+1] = bg.g; px[i*3+2] = bg.b; }
	}
};

static void drawTriangles(Image& im, const std::vector<MeshVertex>& v, float ox = 0, float oy = 0)
{
	for(size_t i = 0; i + 2 < v.size(); i += 3)
	{
		const MeshVertex &a = v[i], &b = v[i+1], &c = v[i+2];
		float ax = a.x + ox, ay = a.y + oy, bx = b.x + ox, by = b.y + oy, cx = c.x + ox, cy = c.y + oy;
		float den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy);
		if(fabsf(den) < 1e-9f) continue;
		int x0 = std::max(0, (int)floorf(std::min(ax, std::min(bx, cx))));
		int x1 = std::min(im.w - 1, (int)ceilf(std::max(ax, std::max(bx, cx))));
		int y0 = std::max(0, (int)floorf(std::min(ay, std::min(by, cy))));
		int y1 = std::min(im.h - 1, (int)ceilf(std::max(ay, std::max(by, cy))));
		for(int y = y0; y <= y1; y++) for(int x = x0; x <= x1; x++)
		{
			float px = x + 0.5f, py = y + 0.5f;
			float l1 = ((by - cy) * (px - cx) + (cx - bx) * (py - cy)) / den;
			float l2 = ((cy - ay) * (px - cx) + (ax - cx) * (py - cy)) / den;
			float l3 = 1 - l1 - l2;
			if(l1 < 0 || l2 < 0 || l3 < 0) continue;
			float al = (l1 * a.c.a + l2 * b.c.a + l3 * c.c.a) / 255.0f;
			float r = l1 * a.c.r + l2 * b.c.r + l3 * c.c.r;
			float g = l1 * a.c.g + l2 * b.c.g + l3 * c.c.g;
			float bl = l1 * a.c.b + l2 * b.c.b + l3 * c.c.b;
			float* d = &im.px[((size_t)y * im.w + x) * 3];
			d[0] = r * al + d[0] * (1 - al);
			d[1] = g * al + d[1] * (1 - al);
			d[2] = bl * al + d[2] * (1 - al);
		}
	}
}

static uint32_t crcTable[256];
static void initCrc() { for(uint32_t n = 0; n < 256; n++) { uint32_t c = n; for(int k = 0; k < 8; k++) c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1; crcTable[n] = c; } }
static uint32_t crc(const uint8_t* b, size_t n, uint32_t c = 0xffffffffu) { for(size_t i = 0; i < n; i++) c = crcTable[(c ^ b[i]) & 255] ^ (c >> 8); return c; }
static void put32(std::vector<uint8_t>& o, uint32_t v) { o.push_back(v >> 24); o.push_back(v >> 16); o.push_back(v >> 8); o.push_back(v); }
static void chunk(FILE* f, const char* type, const std::vector<uint8_t>& data)
{
	std::vector<uint8_t> o; put32(o, (uint32_t)data.size());
	std::vector<uint8_t> td(type, type + 4); td.insert(td.end(), data.begin(), data.end());
	o.insert(o.end(), td.begin(), td.end());
	put32(o, ~crc(td.data(), td.size()));
	fwrite(o.data(), 1, o.size(), f);
}

// Writes the image magnified by `zoom` (nearest neighbour) as an uncompressed-deflate PNG.
static bool writePng(const Image& im, const char* path, int zoom = 1)
{
	initCrc();
	int W = im.w * zoom, H = im.h * zoom;
	std::vector<uint8_t> raw;
	for(int y = 0; y < H; y++)
	{
		raw.push_back(0);
		for(int x = 0; x < W; x++)
		{
			const float* p = &im.px[((size_t)(y / zoom) * im.w + (x / zoom)) * 3];
			for(int k = 0; k < 3; k++) raw.push_back((uint8_t)std::min(255.0f, std::max(0.0f, p[k] + 0.5f)));
		}
	}
	std::vector<uint8_t> z = { 0x78, 0x01 };
	uint32_t a = 1, b = 0;
	for(uint8_t v : raw) { a = (a + v) % 65521; b = (b + a) % 65521; }
	for(size_t pos = 0; pos < raw.size(); pos += 65535)
	{
		size_t n = std::min<size_t>(65535, raw.size() - pos);
		z.push_back(pos + n >= raw.size() ? 1 : 0);
		z.push_back(n & 255); z.push_back(n >> 8); z.push_back(~n & 255); z.push_back((~n >> 8) & 255);
		z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
	}
	put32(z, (b << 16) | a);
	FILE* f = fopen(path, "wb");
	if(!f) return false;
	const uint8_t sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	fwrite(sig, 1, 8, f);
	std::vector<uint8_t> ihdr; put32(ihdr, W); put32(ihdr, H);
	ihdr.push_back(8); ihdr.push_back(2); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
	chunk(f, "IHDR", ihdr); chunk(f, "IDAT", z); chunk(f, "IEND", {});
	fclose(f);
	return true;
}
