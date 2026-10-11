/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiControllerLayout.h
 *
 * A controller described as data: a silhouette made of rounded pieces and a
 * table of controls (buttons, sticks, triggers), each with a position, shape
 * and id. GuiControllerView draws any layout, lights controls up and
 * highlights them; supporting another controller means adding a table, not
 * drawing code. Control ids are the INPUT_BTN_* / INPUT_TRIGGER_* /
 * INPUT_THUMB_* bits from InputData.h, so an input state maps straight onto
 * what lights up. Platform-agnostic.
 ***************************************************************************/
#pragma once
#include <cstddef>
#include <vector>
#include "../drivers/InputData.h"

//!A control with no input bit of its own on any supported hardware (the Switch Pro / Wii U capture button).
//!\ingroup grp_controller
enum { GUI_CTL_CAPTURE = (1 << 24) };

//!How a control is drawn.
//!\ingroup grp_controller
enum class CTL_SHAPE {
	CIRCLE,		//!< A round button
	ROUNDRECT,	//!< A rounded rectangle: bumpers, triggers, D-pad arms, small square buttons
	STICK		//!< An analog stick: an outer ring with a cap that can move
};

//!One control on a controller drawing. Coordinates are in the layout's own units.
//!\ingroup grp_controller
struct GuiControlSpot {
	int id;				/*!< Input bit this control stands for; 0 for decoration (never lit or highlighted). */
	CTL_SHAPE shape;
	float x, y, w, h;	/*!< Bounding box. */
	float radius;		/*!< Corner radius for ROUNDRECT. */
	const char * label;	/*!< Text drawn on the control ("A", "ZL", "+"), or null. */
};

//!One rounded rectangle of the silhouette (the pieces are merged into one outline).
//!\ingroup grp_controller
struct GuiBodyPiece {
	float x, y, w, h;	/*!< Rectangle before rotation. */
	float radius;
	float angle;		/*!< Rotation about its center, degrees clockwise. */
};

//!A controller: its drawing size, silhouette and controls.
//!\ingroup grp_controller
struct GuiControllerLayout {
	float width, height;
	std::vector<GuiBodyPiece> body;
	std::vector<GuiControlSpot> spots;

	//!Finds a control by id (null if the layout has none)
	const GuiControlSpot * find(int id) const;
};

//!A Pro-style gamepad: two sticks, D-pad, ABXY, L/R/ZL/ZR, plus, minus, home and capture
//!(Switch Pro, Wii U Pro, and a good stand-in for the Classic Controller Pro).
//!\ingroup grp_controller
const GuiControllerLayout& GuiProControllerLayout();
