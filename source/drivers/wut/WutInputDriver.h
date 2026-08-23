/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutInputDriver.h
 ***************************************************************************/
#pragma once
#include "../InputDriver.h"

class WutInputDriver : public InputDriver
{
public:
    WutInputDriver();
    ~WutInputDriver() override;

    void init() override;
    void shutdown() override;
    void update(float deltaTime) override;
    void setRumble(int channel, bool rumble) override;

private:
    int rumbleCount[4];
    bool rumbleRequest[4];
};

