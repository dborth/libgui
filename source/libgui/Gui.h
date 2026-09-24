/*!
 * \file Gui.h
 * \brief Umbrella header for the platform-agnostic libgui core.
 *
 * Include this one header to get every core UI class (GuiElement, GuiWindow,
 * GuiButton, GuiImage, GuiText, GuiSound, GuiFileBrowser, GuiKeyboard,
 * GuiOptionBrowser, ...), the alignment/state/scroll enums, and the
 * platform driver interfaces they are built on. Nothing reachable from here
 * includes a platform SDK header.
 */

#pragma once

#include <cstdlib>
#include <cstring>
#include <vector>
#include <exception>
#include <cwchar>
#include <cmath>

#include "../filelist.h"
#include "../drivers/Platform.h"
#include "../drivers/InputData.h"
#include "../drivers/InputController.h"

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

#include "GuiTrigger.h"
#include "GuiElement.h"
#include "GuiWindow.h"
#include "GuiTextRenderer.h"
#include "GuiTextTranslator.h"
#include "GuiText.h"
#include "GuiSound.h"
#include "GuiImageData.h"
#include "GuiImage.h"
#include "GuiButton.h"
#include "GuiFileBrowser.h"
#include "GuiKeyboard.h"
#include "GuiOptionBrowser.h"
