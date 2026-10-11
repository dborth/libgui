/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * shapesdemo.h
 * A page of shapes drawn straight from GuiMesh, showing what the
 * ImageRenderer::drawMesh() backends can do: rounded rectangles, outlines,
 * gradients, soft shadows, arcs and polygons, with no image assets.
 ***************************************************************************/
#ifndef _SHAPESDEMO_H_
#define _SHAPESDEMO_H_

#include "libgui/Gui.h"

//!Draws the shape gallery. The geometry is built once, in absolute design-canvas
//!coordinates (the element's own position and alignment are not used).
class GuiShapesDemo : public GuiElement
{
	public:
		GuiShapesDemo();
		void draw() override;
	private:
		GuiMesh mesh;
};

#endif
