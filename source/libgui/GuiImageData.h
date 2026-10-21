/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiImageData.h
 ***************************************************************************/
#pragma once

#include "../drivers/Mutex.h"

//!Decodes compressed image data (PNG) into a platform-native texture created
//!from it. Currently designed for use only with PNG files.
//!\ingroup grp_core
class GuiImageData
{
	public:
		//!Constructor
		GuiImageData();
		//!Constructor
		//!Converts the PNG format image data to a platform-native texture,
		//!allocated and owned by this object.
		//!\param i Source image data (PNG)
		//!\param w Max image width (0 = not set)
		//!\param h Max image height (0 = not set)
		GuiImageData(const uint8_t * i, int w=0, int h=0);
		//!Constructor
		//!Converts the PNG format image data to a platform-native texture,
		//!decoded directly into a caller-supplied buffer.
		//!\param i Source image data (PNG)
		//!\param dst Destination texture buffer, owned by the caller
		//!\param maxw Max image width (0 = not set)
		//!\param maxh Max image height (0 = not set)
		GuiImageData(const uint8_t * i, uint8_t * dst, int maxw=0, int maxh=0);
		//!Constructor
		//!Populates an image directly with an already-created platform-native
		//!texture
		//!\param t Platform-native texture
		//!\param w Image width
		//!\param h Image height
		//!\param takeOwnership If true (default), this object will destroy
		//!\c t when it is destroyed or replaced.
		GuiImageData(void * t, int w, int h, bool takeOwnership = true);
		//!Destructor
		~GuiImageData();
		//!Decodes new PNG data into this object's own texture, reusing the
		//!existing allocation whenever it's already large enough for the new
		//!image instead of freeing and reallocating.
		//!\param pngData Source image data (PNG)
		//!\param maxw Max image width (0 = not set)
		//!\param maxh Max image height (0 = not set)
		//!\return true on success
		bool reload(const uint8_t * pngData, int maxw = 0, int maxh = 0);
		//!Loads a picture into a box with rounded corners: the image is centred in the
		//!box (moved by the offsets) at its own size, anything that falls outside the box
		//!or the rounded corners is cropped, and the mask edge is anti-aliased. Replaces
		//!this object's texture (reusing it when it is already large enough); its size
		//!becomes the box size.
		//!Use it for a picture that sits on a code-drawn button, so it can't spill over
		//!the button's border or corners.
		//!\param pngData Source image data (PNG)
		//!\param boxW Box width, in design-canvas pixels
		//!\param boxH Box height, in design-canvas pixels
		//!\param radius Corner radius of the box (clamped to half the shorter side)
		//!\param inset Shrinks the mask on every side, eg. to stay inside a stroke
		//!\param offsetX Moves the image horizontally from centre (negative = left)
		//!\param offsetY Moves the image vertically from centre (negative = up)
		//!\return true on success
		bool loadClipped(const uint8_t * pngData, int boxW, int boxH, float radius, float inset = 0.0f, float offsetX = 0.0f, float offsetY = 0.0f);
		//!Gets the attached platform-native texture
		//!\return opaque texture handle
		void * getTexture() { return texture; }
		//!Gets the image width
		//!\return image width
		int getWidth() { return width; }
		//!Gets the image height
		//!\return image height
		int getHeight() { return height; }
		//!Sets the scratch buffer that every decode (the PNG-decoding
		//!constructors, reload() and loadClipped()) will stage its raw, uncompressed pixel
		//!rows into while it runs.
		//!\param buffer Scratch buffer, owned and sized by the caller
		//!\param size Size of buffer, in bytes
		static void setDecodeScratch(void * buffer, unsigned int size);
		//!\return the mutex guarding the shared decode scratch buffer set
		//!by setDecodeScratch() - callers decoding from more than one
		//!thread must hold this for the duration of a decode.
		static Mutex & scratchLock();
	protected:
		void * texture; //!< Attached platform-native texture
		int height; //!< Height of image
		int width; //!< Width of image
	private:
		bool ownsTexture; //!< Whether this object may destroy/replace texture
		int capWidth; //!< Width texture was allocated to hold, if ownsTexture (0 otherwise)
		int capHeight; //!< Height texture was allocated to hold, if ownsTexture (0 otherwise)
		//!Decodes a PNG buffer, reusing the existing owned texture if it's
		//!already large enough, otherwise (re)allocating one.
		bool decodeImage(const uint8_t * pngData, int * width, int * height, int maxw, int maxh);
};
