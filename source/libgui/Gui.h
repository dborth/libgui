/*!\mainpage libgui Documentation
 *
 * \section Introduction
 * libgui is a GUI library originally created for the Wii/GameCube, created to 
 * help structure the design of a complicated GUI interface, and to enable an 
 * author to create a sophisticated, feature-rich GUI. It was originally conceived 
 * and written after I started to design a GUI for Snes9x GX, and found libwiisprite 
 * and GRRLIB inadequate for the purpose. It uses GX for drawing, and makes use
 * of PNGU for displaying images and GuiTextRenderer for text. It was designed to
 * be flexible and is easy to modify - don't be afraid to change the way it
 * works or expand it to suit your GUI's purposes! If you do, and you think
 * your changes might benefit others, please share them so they might be
 * added to the project!
 *
 * \section Quickstart
 * Start from the supplied template example. For more advanced uses, see the
 * source code for Snes9x GX, FCE Ultra GX, and Visual Boy Advance GX.

 * \section Contact
 * If you have any suggestions for the library or documentation, or want to
 * contribute, please visit the libgui website:
 * http://code.google.com/p/libgui/

 * \section Credits
 * This library was wholly designed and written by Tantric. Thanks to the
 * authors of PNGU and FreeType, of which this library makes use. Thanks
 * also to the authors of GRRLIB and libwiisprite for laying the foundations.
 *
*/
#pragma once

#include <gccore.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <exception>
#include <wchar.h>
#include <math.h>
#include <asndlib.h>

#include "filelist.h"
#include "pngu.h"
#include "oggplayer.h"

enum class ALIGN_V {
	TOP,
	BOTTOM,
	MIDDLE
};

enum class ALIGN_H {
	LEFT,
	RIGHT,
	CENTRE
};

enum class STATE {
	DEFAULT,
	SELECTED,
	CLICKED,
	HELD,
	DISABLED
};

enum class SCROLL {
	NONE,
	HORIZONTAL
};

typedef struct _gui_color {
 	u8 r;			/*!< Red color component. */
 	u8 g;			/*!< Green color component. */
 	u8 b;			/*!< Blue alpha component. */
	u8 a;			/*!< Alpha component. If a function does not use the alpha value, it is safely ignored. */
} GuiColor;

#include "../video.h"

#include "GuiInput.h"
#include "GuiInputController.h"
#include "GuiTrigger.h"
#include "GuiElement.h"
#include "GuiWindow.h"
#include "GuiTextRenderer.h"
#include "GuiText.h"
#include "GuiSound.h"
#include "GuiImageData.h"
#include "GuiImage.h"
#include "GuiTooltip.h"
#include "GuiButton.h"
#include "GuiFileBrowser.h"
#include "GuiKeyboard.h"
#include "GuiOptionBrowser.h"
