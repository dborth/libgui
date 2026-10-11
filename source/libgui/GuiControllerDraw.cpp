/****************************************************************************
 * libgui
 * Daryl Borth 2026
 * GuiControllerDraw.cpp
 ***************************************************************************/

#include "GuiControllerDraw.h"
#include <cmath>
#include <algorithm>

static PixelColor rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	PixelColor c = { r, g, b, a };
	return c;
}

static PixelColor withAlpha(PixelColor c, uint8_t a)
{
	c.a = a;
	return c;
}

static PixelColor mixToWhite(PixelColor c, float t)
{
	c.r = (uint8_t)(c.r + (255 - c.r) * t);
	c.g = (uint8_t)(c.g + (255 - c.g) * t);
	c.b = (uint8_t)(c.b + (255 - c.b) * t);
	return c;
}

void GuiBuildControllerBody(GuiMesh& mesh, const GuiControllerLayout& layout, float scale)
{
	const GuiTheme& theme = GuiTheme::current();
	const float outline = std::max(1.5f, 2.0f * scale);
	const GuiFill outlineFill = GuiFill::solid(theme.outlineStrong);
	const GuiFill bodyFill = GuiFill::linear(rgb(30, 38, 52), rgb(22, 28, 38), 90);

	std::vector<std::vector<GuiPoint> > pieces;
	for(size_t i = 0; i < layout.body.size(); i++)
	{
		const GuiBodyPiece& b = layout.body[i];
		GuiRect r = { b.x * scale, b.y * scale, b.w * scale, b.h * scale };
		pieces.push_back(GuiRoundRectPolygon(r, b.radius * scale, b.angle));
	}

	// outlines first, fills after: where pieces overlap, the fill of one covers the outline of the other
	for(size_t i = 0; i < pieces.size(); i++)
	{
		mesh.addPolygon(pieces[i], outlineFill);
		mesh.strokePolygon(pieces[i], outline, STROKE::OUTSIDE, outlineFill);
	}
	for(size_t i = 0; i < pieces.size(); i++)
		mesh.addPolygon(pieces[i], bodyFill);

	// controls that never change appearance
	for(size_t i = 0; i < layout.spots.size(); i++)
		if(layout.spots[i].id == 0)
			GuiBuildControllerSpot(mesh, layout.spots[i], CTL_STATE::NORMAL, scale);
}

void GuiBuildControllerSpot(GuiMesh& mesh, const GuiControlSpot& spot, CTL_STATE state, float scale)
{
	const GuiTheme& theme = GuiTheme::current();
	const bool lit = state == CTL_STATE::LIT || state == CTL_STATE::LIT_HIGHLIGHT;
	const bool hl = state == CTL_STATE::HIGHLIGHT || state == CTL_STATE::LIT_HIGHLIGHT;

	const float stroke = std::max(1.2f, 1.5f * scale);
	GuiRect r = { spot.x * scale, spot.y * scale, spot.w * scale, spot.h * scale };

	PixelColor fill = spot.shape == CTL_SHAPE::STICK ? rgb(14, 18, 24) : theme.surface;
	PixelColor edge = theme.outlineStrong;
	if(lit)
	{
		fill = spot.shape == CTL_SHAPE::STICK ? rgb(14, 18, 24) : theme.good;
		edge = mixToWhite(theme.good, 0.35f);
	}
	if(hl)
		edge = theme.accent;

	const float edgeWidth = hl ? stroke * 1.8f : stroke;
	if(hl)
	{
		const float blur = 8.0f * std::max(scale, 0.8f);
		mesh.addShadow(r, spot.shape == CTL_SHAPE::ROUNDRECT ? GuiCorners(spot.radius * scale) : GuiCorners(10000.0f), blur, 1.0f, 0.0f, 0.0f, withAlpha(theme.accent, 110));
	}

	if(spot.shape == CTL_SHAPE::ROUNDRECT)
	{
		mesh.addRoundRect(r, GuiCorners(spot.radius * scale), GuiFill::solid(fill));
		mesh.strokeRoundRect(r, GuiCorners(spot.radius * scale), edgeWidth, STROKE::INSIDE, GuiFill::solid(edge));
	}
	else
	{
		mesh.addEllipse(r, GuiFill::solid(fill));
		mesh.strokeEllipse(r, edgeWidth, STROKE::INSIDE, GuiFill::solid(edge));
		if(spot.shape == CTL_SHAPE::STICK)
		{
			// a second ring inside, as on the real stick's gate
			const float inset = r.w * 0.11f;
			GuiRect inner = { r.x + inset, r.y + inset, r.w - 2.0f * inset, r.h - 2.0f * inset };
			mesh.strokeEllipse(inner, std::max(1.0f, stroke * 0.7f), STROKE::INSIDE, GuiFill::solid(withAlpha(theme.outlineStrong, 150)));
		}
	}
}

void GuiBuildControllerStickCap(GuiMesh& mesh, const GuiControlSpot& spot, float x, float y, bool lit, float scale)
{
	const GuiTheme& theme = GuiTheme::current();
	const float d = spot.w * scale * 0.56f;
	const float travel = (spot.w * scale - d) * 0.5f * 0.85f;
	// keep the cap inside the ring when the stick is pushed diagonally
	float dx = x, dy = y;
	const float len = sqrtf(dx * dx + dy * dy);
	if(len > 1.0f)
	{
		dx /= len;
		dy /= len;
	}
	const float cx = (spot.x + spot.w * 0.5f) * scale + dx * travel;
	const float cy = (spot.y + spot.h * 0.5f) * scale + dy * travel;
	GuiRect r = { cx - d * 0.5f, cy - d * 0.5f, d, d };

	const PixelColor fill = lit ? theme.good : rgb(40, 50, 68);
	const PixelColor edge = lit ? mixToWhite(theme.good, 0.35f) : theme.outlineStrong;
	mesh.addEllipse(r, GuiFill::linear(mixToWhite(fill, 0.12f), fill, 90));
	mesh.strokeEllipse(r, std::max(1.2f, 1.5f * scale), STROKE::INSIDE, GuiFill::solid(edge));
}
