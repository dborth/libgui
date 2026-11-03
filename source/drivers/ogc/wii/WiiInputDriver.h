/****************************************************************************
 * Platform Abstraction Layer (Wii driver)
 * Daryl Borth 2026
 * WiiInputDriver.h
 *
 * The Wii's input: everything on a GameCube (OgcInputDriver) plus the
 * Wiimote and its extensions, the Wii U GamePad (as a Wii U's vWii/Wii VC
 * sees it), and the console's power/reset buttons.
 ***************************************************************************/
#pragma once

#include "../OgcInputDriver.h"

//!Wii InputDriver: a GameCube's PAD input (OgcInputDriver) plus WPAD (Wiimote,
//!Nunchuk, Classic Controller, Wii U Pro Controller) and a libwiidrc channel
//!that lets a Wii app read a Wii U GamePad.
//!\ingroup grp_ogc
class WiiInputDriver : public OgcInputDriver
{
	public:
		~WiiInputDriver() override;

	protected:
		void initHardware() override;
		void scanHardware() override;
		void readChannel(int channel, InputPadData & pad) override;
		bool isSystemRumbleAllowed() const override;
		void driveRumble(int channel, bool on) override;

	private:
		void readWiimote(int channel, InputPadData & pad);
		void readGamePad(InputPadData & pad);

};
