/****************************************************************************
 * Platform Abstraction Layer (WUT driver)
 * Daryl Borth 2026
 * WutVideoDriver.h
 ***************************************************************************/
#pragma once

#include <vector>
#include <gx2/sampler.h>
#include <gx2/texture.h>
#include "../VideoDriver.h"
#include "../Mutex.h"

//!A mesh kept in GPU-readable vertex buffers across frames (ImageRenderer::createMesh()).
//!Positions are NDC for an instance at the design canvas origin at scale 1, so a draw only
//!sets the shader's offset and scale to place it.
struct WutMesh
{
	float * positions;		//!<3 floats per vertex (x, y, 0)
	uint8_t * colors;		//!<RGBA8 per vertex
	uint32_t count;			//!<Number of vertices
	WutMesh * next;			//!<Link in the retired list
};

//!One recorded UI draw (image, glyph quad or flat rectangle). The renderers
//!record these instead of drawing immediately, and WutVideoDriver replays the
//!whole list once for the TV and once for the GamePad (see flushDrawQueue()).
//!\ingroup grp_wut
struct WutDrawCmd
{
	//!Which shader a queued draw goes through
	enum class Kind : uint8_t
	{
		Texture, //!<textured quad through Texture2DShader
		Color,   //!<flat-color quad through ColorShader
		Mesh,    //!<triangle list from the mesh arena through ColorShader (see WutVideoDriver::queueMesh())
		MeshInstance //!<a WutMesh through ColorShader, placed by the shader-side offset/scale (see WutVideoDriver::queueMeshInstance())
	};

	Kind kind; //!< Which shader this draw uses
	const GX2Texture * texture;   //!<Texture only
	const GX2Sampler * sampler;   //!<Texture only
	float angle;                  //!<Texture only, radians
	float offset[3];              //!<NDC position of the quad's center
	float scale[3];               //!<NDC half-extents of the quad
	float colorIntensity[4]; //!< RGBA tint (Texture) or fill color (Color)
	uint32_t first;          //!<Mesh only: index of the first vertex in the arena
	uint32_t count;          //!<Mesh only: number of vertices
	const WutMesh * mesh;    //!<MeshInstance only
};

//!Wii U VideoDriver: GX2 + libwhb's WHBGfx* helpers. Every frame's draws
//!are submitted twice - once for the TV, once for the GamePad - so the same
//!UI always reaches both screens; there's no separate dual-display mode.
//!UI draws are recorded into a list and replayed per target (one context
//!switch each) rather than switching targets around every single draw.
//!\ingroup grp_wut
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

		int getRefreshRate() const override;
		float getDeltaTime() const override;
		float getUIScale() const override { return uiScale; }

		ImageRenderer* getImageRenderer() override { return imageRenderer; }
		GlyphRenderer* getGlyphRenderer() override { return glyphRenderer; }

		//!False once the OS has taken away the foreground (HOME menu overlay,
		//!forced exit, etc.) - GX2 is off-limits at that point, so every
		//!draw/render entry point below checks this first and no-ops rather
		//!than issuing a GX2 call into a context we no longer own.
		bool isForeground() const;

		//!Records a UI draw for the next flushDrawQueue(). Called by the
		//!renderers below; must be called from the thread that draws.
		void queueDraw(const WutDrawCmd& cmd);

		//!Copies a triangle list into this frame's vertex arena (converting positions
		//!to NDC) and records a Mesh draw. A mesh that does not fit in what is left of the
		//!arena this frame is dropped. Same threading rule as queueDraw().
		//!\param vertices Triangle list in design-canvas pixels
		//!\param count Number of vertices
		void queueMesh(const MeshVertex* vertices, size_t count);

		//!Records a draw of a mesh made by WutImageRenderer::createMesh(). Nothing is copied: the
		//!mesh must stay alive until the draw has run, which destroyMesh() guarantees by
		//!handing it to retireMesh().
		//!\param mesh Mesh to draw
		//!\param x Where the mesh's (0,0) lands, design-canvas pixels
		//!\param y Where the mesh's (0,0) lands, design-canvas pixels
		//!\param scaleX Horizontal stretch about (x,y)
		//!\param scaleY Vertical stretch about (x,y)
		void queueMeshInstance(const WutMesh* mesh, float x, float y, float scaleX, float scaleY);

		//!Sets a mesh aside to be freed once the GPU is done with every frame that might draw it.
		//!Safe from any thread.
		void retireMesh(WutMesh* mesh);

		//!Replays every recorded UI draw into the TV context, then the same
		//!list into the GamePad context, and empties the list. Does nothing
		//!when the list is empty. render() calls this; anything that frees a
		//!texture a queued draw may reference, or that draws directly with
		//!GX2 and must keep its order relative to queued UI draws, must call
		//!it first. Leaves the GamePad context bound.
		void flushDrawQueue();

	private:
		static const size_t cuMaxQueuedDraws = 4096;
		std::vector<WutDrawCmd> drawQueue;

		// Vertex arena for Mesh draws. Written when a mesh is queued, made visible to the
		// GPU in flushDrawQueue(), read by both the TV and GamePad replays, and rewound
		// once per frame in prepareFrame() - never mid-frame, because a draw submitted
		// earlier in the frame may still be reading it (same rule as ColorShader's slots).
		static const uint32_t cuMeshArenaVertices = 1u << 18;
		float * meshPositions = nullptr;	// 3 floats (NDC x, y, 0) per vertex
		uint8_t * meshColors = nullptr;		// RGBA8 per vertex
		uint32_t meshUsed = 0;				// vertices written this frame
		uint32_t meshFlushed = 0;			// vertices already invalidated for the GPU

		// Meshes whose owners let go of them, waiting for the GPU to finish with them
		Mutex * retireLock = nullptr;
		WutMesh * retiredMeshes = nullptr;
		void freeRetiredMeshes(bool drainGpu = false);
		void replayDrawQueue() const; // draws drawQueue into whichever context is currently bound

		// Binds the TV context state and resets the per-frame render
		// state (viewport/scissor/blend/depth/cull) that WHBGfxInit()
		// doesn't set on its own. Called once at the end of init() so
		// the first frame's draws land somewhere valid, then again at
		// the top of every render() pass.
		void prepareFrame();

		// Queries GX2's current TV scan mode/aspect ratio and derives the
		// physical TV and DRC target dims
		void computeUIScale();

		int screenWidth;
		int screenHeight;
		float uiScale = 1.0f;
		uint32_t frameTimer;
		PixelColor clearColor;

		ImageRenderer * imageRenderer;
		GlyphRenderer * glyphRenderer;
};

//!GX2-backed ImageRenderer for GuiImage/GuiImageData, using Texture2DShader.
//!\ingroup grp_wut
class WutImageRenderer : public ImageRenderer
{
	public:
		//!\param driver The video driver draws are queued on
		WutImageRenderer(WutVideoDriver * driver);

		void * createTexture(int width, int height) override;
		void loadTextureData(void * texture, const uint8_t * rgba, int width, int height) override;
		void destroyTexture(void * texture) override;
		void drawTexture(void * texture, float xpos, float ypos, uint16_t width, uint16_t height, float degrees, float scaleX, float scaleY, uint8_t alpha) override;
		void drawRectangle(float x, float y, float width, float height, PixelColor color) override;
		void drawMesh(const MeshVertex* vertices, size_t count) override;
		void* createMesh(const MeshVertex* vertices, size_t count) override;
		void destroyMesh(void* mesh) override;
		void drawMeshInstance(void* mesh, float x, float y, float scaleX, float scaleY) override;

	private:
		WutVideoDriver * driver;
		GX2Sampler sampler;
};

//!GX2-backed GlyphRenderer for GuiTextRenderer, using Texture2DShader for
//!glyph quads and ColorShader for solid "feature" rectangles.
//!\ingroup grp_wut
class WutGlyphRenderer : public GlyphRenderer
{
	public:
		//!\param driver The video driver draws are queued on
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
