/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * controllerdraw.h
 *
 * The pure geometry behind ControllerView: how a controller and its
 * controls look in each state. No elements, no platform; tested on a host.
 ***************************************************************************/
#pragma once
#include "libgui/GuiPaint.h"
#include "libgui/GuiTheme.h"
#include "controllerlayout.h"

//!The look a control is drawn in.
enum class CTL_STATE {
	NORMAL,
	LIT,				//!< Pressed
	HIGHLIGHT,			//!< Being edited / pointed at
	LIT_HIGHLIGHT
};

//!Builds the resting drawing of a controller (silhouette, then every decorative control) in local
//!coordinates, with the layout multiplied by `scale`. The silhouette's outline is made by drawing each
//!piece's outline first and the fills after, so overlapping pieces merge into one shape.
void BuildControllerBody(GuiMesh& mesh, const ControllerLayout& layout, float scale);

//!Builds one control in one state (an analog stick's cap is separate, see BuildControllerStickCap).
void BuildControllerSpot(GuiMesh& mesh, const ControlSpot& spot, CTL_STATE state, float scale);

//!Builds an analog stick's cap, displaced by (x, y) (each -1 to 1) from the middle of its ring.
void BuildControllerStickCap(GuiMesh& mesh, const ControlSpot& spot, float x, float y, bool lit, float scale);
