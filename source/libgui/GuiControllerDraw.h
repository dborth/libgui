/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiControllerDraw.h
 *
 * The pure geometry behind GuiControllerView: how a controller and its
 * controls look in each state. No elements, no platform; tested on a host.
 ***************************************************************************/
#pragma once
#include "GuiPaint.h"
#include "GuiTheme.h"
#include "GuiControllerLayout.h"

//!The look a control is drawn in.
//!\ingroup grp_controller
enum class CTL_STATE {
	NORMAL,
	LIT,				//!< Pressed
	HIGHLIGHT,			//!< Being edited / pointed at
	LIT_HIGHLIGHT
};

//!Small control icons for lists and hints (the "A" in a circle, a bumper, a D-pad arrow).
//!\ingroup grp_controller
enum class GLYPH {
	BUTTON,		//!< A light circle (face buttons; the letter goes on it)
	BUMPER,		//!< A light rounded rectangle (L, R, ZL, ZR)
	UP,			//!< D-pad arrows: a circle with a triangle
	DOWN,
	LEFT,
	RIGHT,
	PLUS,
	MINUS,
	STICK,		//!< A circle with an inner ring (stick click; the letter goes on it)
	CAPTURE,	//!< A rounded square with a ring
	HOME
};

//!Gets the natural size of a glyph.
//!\ingroup grp_controller
void GuiGlyphSize(GLYPH kind, float& w, float& h);

//!Builds a glyph filling a w x h box at the origin, in the theme's light-on-dark colors. Letters are text and are not part of the mesh.
//!\ingroup grp_controller
void GuiBuildGlyph(GuiMesh& mesh, GLYPH kind, float w, float h);

//!Builds the resting drawing of a controller (silhouette, then every decorative control) in local
//!coordinates, with the layout multiplied by `scale`. The silhouette's outline is made by drawing each
//!piece's outline first and the fills after, so overlapping pieces merge into one shape.
//!\ingroup grp_controller
void GuiBuildControllerBody(GuiMesh& mesh, const GuiControllerLayout& layout, float scale);

//!Builds one control in one state (an analog stick's cap is separate, see GuiBuildControllerStickCap).
//!\ingroup grp_controller
void GuiBuildControllerSpot(GuiMesh& mesh, const GuiControlSpot& spot, CTL_STATE state, float scale);

//!Builds an analog stick's cap, displaced by (x, y) (each -1 to 1) from the middle of its ring.
//!\ingroup grp_controller
void GuiBuildControllerStickCap(GuiMesh& mesh, const GuiControlSpot& spot, float x, float y, bool lit, float scale);

