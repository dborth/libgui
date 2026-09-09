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
class OgcInputDriver : public InputDriver
{
public:
    OgcInputDriver();
    ~OgcInputDriver() override;

    void init() override;
    void shutdown() override;
    void update() override;
    void setRumble(int channel, bool rumble) override;

private:
    int rumbleCount[4];
    bool rumbleRequest[4];
};
