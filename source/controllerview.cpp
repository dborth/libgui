/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * controllerview.cpp
 ***************************************************************************/

#include "libgui/Gui.h"
#include "controllerview.h"
#include <cmath>
#include <algorithm>

static PixelColor withAlpha(PixelColor c, uint8_t a)
{
	c.a = a;
	return c;
}

// ---- the element

ControllerView::ControllerView(const ControllerLayout& l, float s) :
	layout(&l), scale(s), leader(false), leaderX(0), leaderY(0), meshFeather(-1.0f), dirty(true), originX(0), originY(0), alpha(255)
{
	rebuild();
}

ControllerView::~ControllerView()
{
	clearLabels();
}

void ControllerView::clearLabels()
{
	for(size_t i = 0; i < labels.size(); i++)
		delete labels[i];
	labels.clear();
	labelLit.clear();
}

void ControllerView::rebuild()
{
	// (a hair is taken off first: 400 * 0.6f is 240.000004 in floats and must not become 241)
	width = (int)ceilf(layout->width * scale - 0.01f);
	height = (int)ceilf(layout->height * scale - 0.01f);

	spots.assign(layout->spots.size(), SpotCache());
	for(size_t i = 0; i < spots.size(); i++)
		for(int k = 0; k < 4; k++)
			spots[i].built[k] = false;
	stickX.assign(layout->spots.size(), 0.0f);
	stickY.assign(layout->spots.size(), 0.0f);

	// labels are text children centered on their control
	clearLabels();
	const GuiTheme& theme = GuiTheme::current();
	for(size_t i = 0; i < layout->spots.size(); i++)
	{
		const ControlSpot& s = layout->spots[i];
		GuiText * t = nullptr;
		if(s.label)
		{
			t = new GuiText(s.label, (int)(15.0f * std::max(scale, 0.7f)), theme.textMuted);
			t->setParent(this);
			t->setAlignment(ALIGN_H::CENTRE, ALIGN_V::MIDDLE);
			const float cx = (s.x + s.w * 0.5f) * scale - width * 0.5f;
			const float cy = (s.y + s.h * 0.5f) * scale - height * 0.5f;
			t->setPosition((int)floorf(cx + 0.5f), (int)floorf(cy + 0.5f));
		}
		labels.push_back(t);
		labelLit.push_back(false);
	}
	dirty = true;
}

void ControllerView::setLayout(const ControllerLayout& l)
{
	layout = &l;
	highlighted.clear();
	rebuild();
}

void ControllerView::setScale(float s)
{
	scale = s;
	rebuild();
}

void ControllerView::setHighlight(int id)
{
	highlighted.clear();
	if(id != 0)
		highlighted.push_back(id);
}

void ControllerView::setHighlight(const std::vector<int>& ids)
{
	highlighted = ids;
}

void ControllerView::setLeader(bool on, float fromX, float fromY)
{
	leader = on;
	leaderX = fromX;
	leaderY = fromY;
}

void ControllerView::setStick(int id, float x, float y)
{
	for(size_t i = 0; i < layout->spots.size(); i++)
	{
		if(layout->spots[i].id == id && layout->spots[i].shape == CTL_SHAPE::STICK)
		{
			stickX[i] = x;
			stickY[i] = y;
		}
	}
}

bool ControllerView::getSpotCentre(int id, float& x, float& y) const
{
	const ControlSpot * s = layout->find(id);
	if(!s)
		return false;
	x = originX + (s->x + s->w * 0.5f) * scale;
	y = originY + (s->y + s->h * 0.5f) * scale;
	return true;
}

void ControllerView::emit(const GuiMesh& m)
{
	const std::vector<MeshVertex>& src = m.vertices();
	for(size_t i = 0; i < src.size(); i++)
	{
		MeshVertex v = src[i];
		v.x += originX;
		v.y += originY;
		if(alpha < 255)
			v.c.a = (uint8_t)((v.c.a * alpha + 127) / 255);
		scratch.push_back(v);
	}
}

void ControllerView::draw()
{
	if(!this->isVisible())
		return;

	alpha = this->getAlpha();
	if(alpha > 0)
	{
		VideoDriver * video = platform->getVideo();
		const float uiScale = video->getUIScale();
		const float feather = uiScale > 0.0f ? 1.0f / uiScale : 1.0f;
		if(dirty || feather != meshFeather)
		{
			body = GuiMesh(feather);
			BuildControllerBody(body, *layout, scale);
			for(size_t i = 0; i < spots.size(); i++)
				for(int k = 0; k < 4; k++)
				{
					spots[i].mesh[k] = GuiMesh(feather);
					spots[i].built[k] = false;
				}
			meshFeather = feather;
			dirty = false;
		}

		originX = (float)this->getLeft();
		originY = (float)this->getTop();
		scratch.clear();
		emit(body);

		// resting controls first, highlighted ones on top so their glow is not painted over
		for(int pass = 0; pass < 2; pass++)
		{
			for(size_t i = 0; i < layout->spots.size(); i++)
			{
				const ControlSpot& s = layout->spots[i];
				if(s.id == 0)
					continue;
				const bool hl = std::find(highlighted.begin(), highlighted.end(), s.id) != highlighted.end();
				if((pass == 1) != hl)
					continue;
				const bool isLit = lit && lit(s.id);
				const int k = (isLit ? 1 : 0) + (hl ? 2 : 0);
				if(!spots[i].built[k])
				{
					BuildControllerSpot(spots[i].mesh[k], s, (CTL_STATE)k, scale);
					spots[i].built[k] = true;
				}
				emit(spots[i].mesh[k]);

				if(labels[i] && labelLit[i] != isLit)
				{
					labels[i]->setColor(isLit ? GuiTheme::current().textOnAccent : GuiTheme::current().textMuted);
					labelLit[i] = isLit;
				}
			}
		}

		// stick caps move, so they are built each frame
		for(size_t i = 0; i < layout->spots.size(); i++)
		{
			const ControlSpot& s = layout->spots[i];
			if(s.shape != CTL_SHAPE::STICK)
				continue;
			GuiMesh cap(feather);
			BuildControllerStickCap(cap, s, stickX[i], stickY[i], lit && lit(s.id), scale);
			emit(cap);
		}

		// a line from a list row to the highlighted control
		if(leader && !highlighted.empty())
		{
			float tx, ty;
			if(getSpotCentre(highlighted[0], tx, ty))
			{
				const GuiTheme& theme = GuiTheme::current();
				GuiMesh line(feather);
				line.addLine({ leaderX, leaderY }, { tx, ty }, 1.6f, GuiFill::solid(withAlpha(theme.accent, 200)));
				GuiRect dot = { tx - 3.5f, ty - 3.5f, 7.0f, 7.0f };
				line.addEllipse(dot, GuiFill::solid(theme.accent));
				// the mesh is already in canvas coordinates: draw it as it is
				const std::vector<MeshVertex>& src = line.vertices();
				for(size_t i = 0; i < src.size(); i++)
				{
					MeshVertex v = src[i];
					if(alpha < 255)
						v.c.a = (uint8_t)((v.c.a * alpha + 127) / 255);
					scratch.push_back(v);
				}
			}
		}

		if(!scratch.empty())
			video->getImageRenderer()->drawMesh(scratch.data(), scratch.size());

		for(size_t i = 0; i < labels.size(); i++)
			if(labels[i])
				labels[i]->draw();
	}

	this->updateEffects();
}
