/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * shapesdemo.cpp
 ***************************************************************************/
#include "shapesdemo.h"

static PixelColor rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	PixelColor c = { r, g, b, a };
	return c;
}

GuiShapesDemo::GuiShapesDemo() :
	// a one-physical-pixel anti-aliasing fringe, whatever the output resolution
	mesh(1.0f / platform->getVideo()->getUIScale())
{
	setSize(platform->getVideo()->getScreenWidth(), platform->getVideo()->getScreenHeight());

	const PixelColor panel = rgb(28, 35, 48), edge = rgb(43, 54, 71), line = rgb(74, 90, 115);
	const PixelColor accent = rgb(255, 179, 71), teal = rgb(63, 208, 176), ink = rgb(232, 237, 244);
	const GuiFill cardFill = GuiFill::linear(rgb(36, 46, 64), panel, 90);

	// a card and a selected card (glow plus accent outline)
	GuiRect c1 = { 24, 24, 180, 90 };
	mesh.addShadow(c1, GuiCorners(12), 14, 0, 0, 4, rgb(0, 0, 0, 150));
	mesh.addRoundRect(c1, GuiCorners(12), cardFill);
	mesh.strokeRoundRect(c1, GuiCorners(12), 2, STROKE::INSIDE, GuiFill::solid(edge));
	GuiRect c2 = { 224, 24, 180, 90 };
	mesh.addShadow(c2, GuiCorners(12), 16, 2, 0, 0, rgb(255, 179, 71, 110));
	mesh.addRoundRect(c2, GuiCorners(12), cardFill);
	mesh.strokeRoundRect(c2, GuiCorners(12), 2, STROKE::INSIDE, GuiFill::solid(accent));

	// capsule buttons, resting and highlighted
	GuiRect b1 = { 424, 24, 190, 40 };
	mesh.addRoundRect(b1, GuiCorners(20), GuiFill::linear(line, edge, 90));
	mesh.strokeRoundRect(b1, GuiCorners(20), 2, STROKE::INSIDE, GuiFill::solid(line));
	GuiRect b2 = { 424, 74, 190, 40 };
	mesh.addRoundRect(b2, GuiCorners(20), GuiFill::linear(rgb(255, 200, 110), rgb(255, 150, 40), 90));
	mesh.strokeRoundRect(b2, GuiCorners(20), 2, STROKE::INSIDE, GuiFill::solid(rgb(255, 220, 160)));

	// a tab (mixed corner radii), a progress bar, and round buttons
	GuiRect tab = { 24, 140, 120, 36 };
	mesh.addRoundRect(tab, GuiCorners(12, 12, 0, 0), GuiFill::solid(panel));
	mesh.strokeRoundRect(tab, GuiCorners(12, 12, 0, 0), 2, STROKE::INSIDE, GuiFill::solid(line));
	GuiRect bar = { 164, 150, 140, 12 };
	mesh.addRoundRect(bar, GuiCorners(6), GuiFill::solid(edge));
	GuiRect barFill = { 164, 150, 98, 12 };
	mesh.addRoundRect(barFill, GuiCorners(6), GuiFill::linear(teal, rgb(40, 150, 130), 0));
	for(int i = 0; i < 5; i++)
	{
		float s = 14.0f + i * 8.0f;
		GuiRect q = { 330.0f + i * 56.0f - s / 2, 158.0f - s / 2, s, s };
		mesh.addEllipse(q, GuiFill::solid(panel));
		mesh.strokeEllipse(q, 2, STROKE::INSIDE, GuiFill::solid(i == 2 ? teal : line));
	}

	// progress rings, a polygon, a chevron, and lines down to half a pixel
	mesh.strokeArc({ 60, 250 }, 30, -90, 270, 8, edge);
	mesh.strokeArc({ 60, 250 }, 30, -90, 210, 8, teal);
	mesh.strokeArc({ 150, 250 }, 30, 0, 360, 1.0f, line);
	std::vector<GuiPoint> tri = { { 220, 280 }, { 260, 280 }, { 240, 222 } };
	mesh.addPolygon(tri, GuiFill::solid(accent));
	std::vector<GuiPoint> chev = { { 290, 222 }, { 330, 222 }, { 350, 250 }, { 330, 278 }, { 290, 278 }, { 310, 250 } };
	mesh.addPolygon(chev, GuiFill::linear(teal, rgb(40, 120, 200), 0));
	mesh.strokePolygon(chev, 2, STROKE::OUTSIDE, GuiFill::solid(ink));
	mesh.addLine({ 380, 230 }, { 470, 270 }, 3, GuiFill::solid(ink));
	mesh.addLine({ 380, 270 }, { 470, 230 }, 1, GuiFill::solid(ink));
	mesh.addLine({ 380, 250 }, { 470, 250 }, 0.5f, GuiFill::solid(ink));

	// a controller built from the same few blocks
	const float ox = 300, oy = 300;
	GuiRect body = { ox + 10, oy + 34, 310, 120 };
	mesh.addShadow(body, GuiCorners(60), 18, 0, 0, 6, rgb(0, 0, 0, 160));
	mesh.addRoundRect(body, GuiCorners(60), GuiFill::linear(rgb(52, 64, 86), rgb(30, 38, 52), 90));
	mesh.strokeRoundRect(body, GuiCorners(60), 2, STROKE::INSIDE, GuiFill::solid(line));
	mesh.addRoundRect({ ox + 30, oy + 10, 56, 22 }, GuiCorners(11), GuiFill::solid(line));
	mesh.addRoundRect({ ox + 244, oy + 10, 56, 22 }, GuiCorners(11), GuiFill::solid(line));
	GuiRect stick = { ox + 58, oy + 58, 44, 44 };
	mesh.addEllipse(stick, GuiFill::solid(rgb(14, 18, 24)));
	mesh.strokeEllipse(stick, 2, STROKE::INSIDE, GuiFill::solid(line));
	mesh.addEllipse({ ox + 72, oy + 70, 16, 16 }, GuiFill::solid(teal));
	mesh.addRoundRect({ ox + 66, oy + 108, 12, 36 }, GuiCorners(3), GuiFill::solid(rgb(14, 18, 24)));
	mesh.addRoundRect({ ox + 54, oy + 120, 36, 12 }, GuiCorners(3), GuiFill::solid(rgb(14, 18, 24)));
	const float bx[4] = { 250, 224, 276, 250 }, by[4] = { 66, 92, 92, 118 };
	for(int i = 0; i < 4; i++)
	{
		GuiRect q = { ox + bx[i] - 13, oy + by[i] - 13, 26, 26 };
		mesh.addEllipse(q, GuiFill::solid(i == 2 ? teal : rgb(35, 45, 61)));
		mesh.strokeEllipse(q, 2, STROKE::INSIDE, GuiFill::solid(i == 2 ? teal : line));
	}
}

void GuiShapesDemo::draw()
{
	if(!this->isVisible())
		return;

	platform->getVideo()->getImageRenderer()->drawMesh(mesh.vertices().data(), mesh.size());
	this->updateEffects();
}
