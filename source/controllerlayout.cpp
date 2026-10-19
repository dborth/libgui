/****************************************************************************
 * libgui Template
 * Daryl Borth 2026
 * controllerlayout.cpp
 ***************************************************************************/

#include "controllerlayout.h"

const ControlSpot * ControllerLayout::find(int id) const
{
	if(id == 0)
		return nullptr;
	for(size_t i = 0; i < spots.size(); i++)
		if(spots[i].id == id)
			return &spots[i];
	return nullptr;
}

static ControlSpot circle(int id, float cx, float cy, float d, const char * label)
{
	return ControlSpot{ id, CTL_SHAPE::CIRCLE, cx - d * 0.5f, cy - d * 0.5f, d, d, 0.0f, label };
}

static ControlSpot rrect(int id, float x, float y, float w, float h, float radius, const char * label)
{
	return ControlSpot{ id, CTL_SHAPE::ROUNDRECT, x, y, w, h, radius, label };
}

static ControlSpot stick(int id, float cx, float cy, float d)
{
	return ControlSpot{ id, CTL_SHAPE::STICK, cx - d * 0.5f, cy - d * 0.5f, d, d, 0.0f, nullptr };
}

static ControllerLayout buildPro()
{
	ControllerLayout l;
	l.width = 400.0f;
	l.height = 290.0f;

	// silhouette: a wide body and two grips leaning outward
	l.body.push_back(BodyPiece{ 52.0f, 46.0f, 296.0f, 128.0f, 60.0f, 0.0f });
	l.body.push_back(BodyPiece{ 60.0f, 132.0f, 78.0f, 136.0f, 38.0f, 15.0f });
	l.body.push_back(BodyPiece{ 262.0f, 132.0f, 78.0f, 136.0f, 38.0f, -15.0f });

	// shoulder buttons and triggers, stacked at the top corners
	l.spots.push_back(rrect(INPUT_TRIGGER_ZL, 62.0f, 6.0f, 92.0f, 24.0f, 12.0f, "ZL"));
	l.spots.push_back(rrect(INPUT_TRIGGER_L, 52.0f, 30.0f, 104.0f, 22.0f, 11.0f, "L"));
	l.spots.push_back(rrect(INPUT_TRIGGER_ZR, 246.0f, 6.0f, 92.0f, 24.0f, 12.0f, "ZR"));
	l.spots.push_back(rrect(INPUT_TRIGGER_R, 244.0f, 30.0f, 104.0f, 22.0f, 11.0f, "R"));

	// small buttons between the sticks
	l.spots.push_back(circle(INPUT_BTN_MINUS, 168.0f, 74.0f, 17.0f, "-"));
	l.spots.push_back(circle(INPUT_BTN_PLUS, 232.0f, 74.0f, 17.0f, "+"));
	l.spots.push_back(rrect(CTL_CAPTURE, 170.0f, 98.0f, 16.0f, 16.0f, 3.0f, nullptr));
	l.spots.push_back(circle(INPUT_BTN_HOME, 222.0f, 106.0f, 17.0f, nullptr));

	// sticks
	l.spots.push_back(stick(INPUT_THUMB_L, 112.0f, 100.0f, 66.0f));
	l.spots.push_back(stick(INPUT_THUMB_R, 252.0f, 152.0f, 66.0f));

	// D-pad: four arms around a plate (the plate is decoration)
	l.spots.push_back(rrect(0, 140.0f, 140.0f, 20.0f, 20.0f, 3.0f, nullptr));
	l.spots.push_back(rrect(INPUT_BTN_UP, 140.0f, 118.0f, 20.0f, 25.0f, 4.0f, nullptr));
	l.spots.push_back(rrect(INPUT_BTN_DOWN, 140.0f, 157.0f, 20.0f, 25.0f, 4.0f, nullptr));
	l.spots.push_back(rrect(INPUT_BTN_LEFT, 118.0f, 140.0f, 25.0f, 20.0f, 4.0f, nullptr));
	l.spots.push_back(rrect(INPUT_BTN_RIGHT, 157.0f, 140.0f, 25.0f, 20.0f, 4.0f, nullptr));

	// face buttons
	l.spots.push_back(circle(INPUT_BTN_X, 294.0f, 66.0f, 28.0f, "X"));
	l.spots.push_back(circle(INPUT_BTN_Y, 266.0f, 94.0f, 28.0f, "Y"));
	l.spots.push_back(circle(INPUT_BTN_A, 322.0f, 94.0f, 28.0f, "A"));
	l.spots.push_back(circle(INPUT_BTN_B, 294.0f, 122.0f, 28.0f, "B"));
	return l;
}

const ControllerLayout& ProControllerLayout()
{
	static const ControllerLayout layout = buildPro();
	return layout;
}
