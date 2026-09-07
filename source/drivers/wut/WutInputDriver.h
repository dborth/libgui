/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutInputDriver.h
 ***************************************************************************/
#pragma once

#include "../InputDriver.h"

//!Wii U InputDriver: VPAD for the GamePad (stick, buttons, and touch,
//!channel 0 only) plus KPAD/WPAD for up to 4 Wiimotes/Nunchuks/Classic/
//!Pro Controllers. GamePad touch is mapped onto the unified cursor/button
//!fields; IR pointer position is smoothed to counter KPADReadEx sampling
//!faster/noisier than the UI update rate.
class WutInputDriver : public InputDriver {
	public:
		WutInputDriver();
		~WutInputDriver() override;

		void init() override;
		void shutdown() override;
		void update() override;
		void setRumble(int channel, bool rumble) override;

	private:
		int rumbleCount[4];
		bool rumbleRequest[4];

		bool drcTouchedPrev;
		float drcLastTouchX;
		float drcLastTouchY;

		// IR pointer smoothing state (per Wiimote channel)
		float irSmoothX[4];
		float irSmoothY[4];
		bool  irSmoothInit[4];
};
