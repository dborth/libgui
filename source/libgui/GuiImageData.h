/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 * GuiImageData.h
 ***************************************************************************/
#pragma once

//!Decodes compressed image data (PNG) into a generic RGBA8 buffer, and
//!(if imageSystem is set) an attached platform-native texture created
//!from it. Currently designed for use only with PNG files.
class GuiImageData
{
	public:
		//!Constructor
		//!Converts the image data to RGBA8 - expects PNG format
		//!\param i Image data
		//!\param w Max image width (0 = not set)
		//!\param h Max image height (0 = not set)
		GuiImageData(const uint8_t * i, int w=0, int h=0);
		//!Destructor
		~GuiImageData();
		//!Gets a pointer to the generic RGBA8 image data
		//!\return pointer to image data
		uint8_t * getImage();
		//!Gets the attached platform-native texture (see IImageRenderer),
		//!or nullptr if none is attached (e.g. no imageSystem was set at
		//!construction time).
		//!\return opaque texture handle
		void * getTexture();
		//!Gets the image width
		//!\return image width
		int getWidth();
		//!Gets the image height
		//!\return image height
		int getHeight();
	protected:
		uint8_t * data; //!< Generic row-major RGBA8 image data (see imagedecode.h)
		void * texture; //!< Attached platform-native texture (see IImageRenderer), owned by this object
		int height; //!< Height of image
		int width; //!< Width of image
};
