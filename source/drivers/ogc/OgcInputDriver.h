/****************************************************************************
 * Platform Abstraction Layer (OGC driver)
 * Daryl Borth 2026
 * OgcInputDriver.h
 ***************************************************************************/
#pragma once
#include "../InputDriver.h"

//!GC/Wii InputDriver: PAD (GameCube) / WPAD (Wiimote, Nunchuk, Classic,
//!Wii U Pro Controller) plus a WiiDRC channel that lets a Wii app read a
//!Wii U GamePad.
//!\ingroup grp_ogc
class OgcInputDriver : public InputDriver
{
	public:
		OgcInputDriver();
		~OgcInputDriver() override;

		void init() override;
		void shutdown() override;
		void update() override;
		void setRumble(int channel, bool rumble) override;
		void setContinuousRumble(int channel, bool continuous) override;

	private:
		bool rumbleRequest[4];
		int menuRumbleFrames[4];     // frames left in the current menu "tick" (0 = idle)
		int menuRumbleGapFrames[4];  // frames left in the enforced silent gap after a tick
		bool continuousRumble[4];    // setContinuousRumble() state
		int continuousRumbleCount[4];// consecutive frames the motor has been driven
		int silenceFrames[4];        // frames left in the forced rest after a long continuous run
};
