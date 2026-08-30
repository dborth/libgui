/****************************************************************************
 * libgui - drivers/ogc
 * Daryl Borth 2009-2026
 * OgcVideoDriver.cpp
 ***************************************************************************/
#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <ogc/machine/processor.h>

#include "OgcVideoDriver.h"
#include "../../libgui/Gui.h"

#define DEFAULT_FIFO_SIZE 256 * 1024
static Mtx GXmodelView2D;

OgcVideoDriver::OgcVideoDriver()
    : screenWidth(0), screenHeight(0), frameTimer(0), whichfb(0), vmode(nullptr), gp_fifo(nullptr)
{

}

OgcVideoDriver::~OgcVideoDriver()
{
	delete imageRenderer;
	delete glyphRenderer;
}

void OgcVideoDriver::init(int width, int height)
{
	VIDEO_Init();

	#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9 && (*(u32*)(0xCD8005A0) >> 16) == 0xCAFE) // Wii U
	{
	write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);
	}
	#endif

	vmode = VIDEO_GetPreferredMode(nullptr); // get default video mode

	#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		vmode->viWidth = 678;
	else
		vmode->viWidth = 672;

	if((vmode->viTVMode >> 2) == VI_NTSC)
	{
		vmode->viXOrigin = (VI_MAX_WIDTH_NTSC - vmode->viWidth) / 2;
		vmode->viYOrigin = (VI_MAX_HEIGHT_NTSC - vmode->viHeight) / 2;
	}
	else
	{
		vmode->viXOrigin = (VI_MAX_WIDTH_PAL - vmode->viWidth) / 2;
		vmode->viYOrigin = (VI_MAX_HEIGHT_PAL - vmode->viHeight) / 2;
	}
	#endif

	VIDEO_Configure (vmode);

	xfb[0] = (uint32_t *) SYS_AllocateFramebuffer (vmode);
	xfb[1] = (uint32_t *) SYS_AllocateFramebuffer (vmode);
	DCInvalidateRange(xfb[0], VIDEO_GetFrameBufferSize(vmode));
	DCInvalidateRange(xfb[1], VIDEO_GetFrameBufferSize(vmode));
	xfb[0] = (uint32_t *) MEM_K0_TO_K1 (xfb[0]);
	xfb[1] = (uint32_t *) MEM_K0_TO_K1 (xfb[1]);

	VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);

	VIDEO_SetBlack (FALSE);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
	if (vmode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync ();

	screenWidth = width;
	screenHeight = height;

	// Initialize GX
	GXColor background = { 0, 0, 0, 0xff };
	gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear (background, GX_MAX_Z24);
	GX_SetDispCopyGamma (GX_GM_1_0);
	GX_SetCullMode (GX_CULL_NONE);

	resetVideoMenu();

	imageRenderer = new OgcImageRenderer();
	glyphRenderer = new OgcGlyphRenderer();
}

void OgcVideoDriver::shutdown()
{
	GX_AbortFrame();
	GX_Flush();

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
}

void OgcVideoDriver::render()
{
	whichfb ^= 1; // flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitForFlush();
	++frameTimer;
}

void OgcVideoDriver::clearScreen(const PixelColor& color)
{
    GXColor background = { color.r, color.g, color.b, color.a };
    GX_SetCopyClear(background, GX_MAX_Z24);
}

void OgcVideoDriver::resetVideoMenu()
{
	Mtx44 p;
	float yscale;
	uint32_t xfbHeight;

	// clears the bg to color and clears the z buffer
	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, GX_MAX_Z24);

	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache ();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(p,0,screenHeight-1,0,screenWidth-1,0,300);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
}

void* OgcImageRenderer::createTexture(int width, int height)
{
	int padWidth = width + (4 - width % 4) % 4;
	int padHeight = height + (4 - height % 4) % 4;
	int len = (padWidth * padHeight) * 4;
	if (len % 32) len += (32 - len % 32);
	return memalign(32, len);
}

void OgcImageRenderer::loadTextureData(void* texture, const uint8_t* rgba, int width, int height)
{
	if(!texture || !rgba) return;
	uint8_t* dst = (uint8_t*)texture;
	int padWidth = width + (4 - width % 4) % 4;
	int padHeight = height + (4 - height % 4) % 4;

	for (int y = 0; y < padHeight; y++) {
		for (int x = 0; x < padWidth; x++) {
			uint32_t offset = ((((y >> 2) * (padWidth >> 2) + (x >> 2)) << 5) + ((y & 3) << 2) + (x & 3)) << 1;
			if (y >= height || x >= width) {
				dst[offset] = 0; dst[offset+1] = 255; dst[offset+32] = 255; dst[offset+33] = 255;
			} else {
				const uint8_t* src = rgba + (y * width + x) * 4;
				dst[offset]   = src[3]; // A
				dst[offset+1] = src[0]; // R
				dst[offset+32] = src[1]; // G
				dst[offset+33] = src[2]; // B
			}
		}
	}

	int len = (padWidth * padHeight) * 2;
	if (len % 32) len += (32 - len % 32);
	DCFlushRange(dst, len);
}

void OgcImageRenderer::destroyTexture(void * texture)
{
	if(texture)
		free(texture);
}

void OgcImageRenderer::drawTexture(void * texture, float xpos, float ypos, uint16_t width, uint16_t height, float degrees, float scaleX, float scaleY, uint8_t alpha)
{
	if(!texture)
		return;

	GXTexObj texObj;

	GX_InitTexObj(&texObj, texture, width, height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width  >>= 1;
	height >>= 1;

	guMtxScale(m1, scaleX, scaleY, 1.0);
	guVector axis = (guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
	GX_Position3f32(-width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 0);

	GX_Position3f32(width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 0);

	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 1);

	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

void OgcImageRenderer::drawRectangle(float x, float y, float width, float height, PixelColor color)
{
	long n = 4;
	float x2 = x+width;
	float y2 = y+height;
	guVector v[] = {{x,y,0.0f}, {x2,y,0.0f}, {x2,y2,0.0f}, {x,y2,0.0f}, {x,y,0.0f}};

	GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT0, n);
	for(long i=0; i<n; ++i)
	{
		GX_Position3f32(v[i].x, v[i].y,  v[i].z);
		GX_Color4u8(color.r, color.g, color.b, color.a);
	}
	GX_End();
}
