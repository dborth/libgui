/****************************************************************************
 * libgui
 * Daryl Borth 2009-2026
 *
 * input.cpp
 * Hardware Translation Layer (Driver)
 * Wii/GameCube controller management and normalization
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>

#include "menu.h"
#include "video.h"
#include "input.h"
#include "wiidrc.h"
#include "libgui/GuiInputController.h"

static int rumbleCount[4] = {0, 0, 0, 0};

static inline float clampf(float v, float lo, float hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/****************************************************************************
 * Hardware Mapping Helpers
 * Translates raw libogc hardware bits to our generic UI masks
 ***************************************************************************/
static uint32_t MapPADToGeneric(uint32_t pad_btns)
{
	uint32_t mask = GUI_BTN_NONE;
	if (pad_btns & PAD_BUTTON_A)      mask |= GUI_BTN_A;
	if (pad_btns & PAD_BUTTON_B)      mask |= GUI_BTN_B;
	if (pad_btns & PAD_BUTTON_X)      mask |= GUI_BTN_X;
	if (pad_btns & PAD_BUTTON_Y)      mask |= GUI_BTN_Y;
	if (pad_btns & PAD_BUTTON_UP)     mask |= GUI_BTN_UP;
	if (pad_btns & PAD_BUTTON_DOWN)   mask |= GUI_BTN_DOWN;
	if (pad_btns & PAD_BUTTON_LEFT)   mask |= GUI_BTN_LEFT;
	if (pad_btns & PAD_BUTTON_RIGHT)  mask |= GUI_BTN_RIGHT;
	if (pad_btns & PAD_BUTTON_START)  mask |= GUI_BTN_PLUS;
	if (pad_btns & PAD_TRIGGER_L)     mask |= GUI_TRIGGER_L;
	if (pad_btns & PAD_TRIGGER_R)     mask |= GUI_TRIGGER_R;
	if (pad_btns & PAD_TRIGGER_Z)     mask |= GUI_TRIGGER_ZR;
	return mask;
}

#ifdef HW_RVL
static uint32_t MapWPADToGeneric(uint32_t wpad_btns)
{
	uint32_t mask = GUI_BTN_NONE;
	if (wpad_btns & (WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A)) mask |= GUI_BTN_A;
	if (wpad_btns & (WPAD_BUTTON_B | WPAD_CLASSIC_BUTTON_B)) mask |= GUI_BTN_B;

	if (wpad_btns & WPAD_BUTTON_1) mask |= GUI_BTN_1;
	if (wpad_btns & WPAD_BUTTON_2) mask |= GUI_BTN_2;

	if (wpad_btns & WPAD_CLASSIC_BUTTON_X) mask |= GUI_BTN_X;
	if (wpad_btns & WPAD_CLASSIC_BUTTON_Y) mask |= GUI_BTN_Y;

	if (wpad_btns & (WPAD_BUTTON_UP | WPAD_CLASSIC_BUTTON_UP))       mask |= GUI_BTN_UP;
	if (wpad_btns & (WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_DOWN))   mask |= GUI_BTN_DOWN;
	if (wpad_btns & (WPAD_BUTTON_LEFT | WPAD_CLASSIC_BUTTON_LEFT))   mask |= GUI_BTN_LEFT;
	if (wpad_btns & (WPAD_BUTTON_RIGHT | WPAD_CLASSIC_BUTTON_RIGHT)) mask |= GUI_BTN_RIGHT;

	if (wpad_btns & (WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS))   mask |= GUI_BTN_PLUS;
	if (wpad_btns & (WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_MINUS)) mask |= GUI_BTN_MINUS;
	if (wpad_btns & (WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME))   mask |= GUI_BTN_HOME;

	if (wpad_btns & (WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_ZL)) mask |= GUI_TRIGGER_L;
	if (wpad_btns & (WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_ZR)) mask |= GUI_TRIGGER_R;

	return mask;
}

static uint32_t MapWiiUGamepadToGeneric(uint32_t drc_btns)
{
	uint32_t mask = GUI_BTN_NONE;
	if (drc_btns & WIIDRC_BUTTON_A) mask |= GUI_BTN_A;
	if (drc_btns & WIIDRC_BUTTON_B) mask |= GUI_BTN_B;
	if (drc_btns & WIIDRC_BUTTON_X) mask |= GUI_BTN_X;
	if (drc_btns & WIIDRC_BUTTON_Y) mask |= GUI_BTN_Y;
	if (drc_btns & WIIDRC_BUTTON_UP) mask |= GUI_BTN_UP;
	if (drc_btns & WIIDRC_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (drc_btns & WIIDRC_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (drc_btns & WIIDRC_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (drc_btns & WIIDRC_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (drc_btns & WIIDRC_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (drc_btns & WIIDRC_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (drc_btns & WIIDRC_BUTTON_L) mask |= GUI_TRIGGER_L;
	if (drc_btns & WIIDRC_BUTTON_R) mask |= GUI_TRIGGER_R;
	if (drc_btns & WIIDRC_BUTTON_ZL) mask |= GUI_TRIGGER_ZL;
	if (drc_btns & WIIDRC_BUTTON_ZR) mask |= GUI_TRIGGER_ZR;
	return mask;
}

/****************************************************************************
 * Analog Normalization Helpers
 ***************************************************************************/
static float NormalizeWPADAnalog(int pos, int min, int max, int center)
{
	if (min == max) return 0.0f;

	// Handle broken 3rd party controller calibration data
	if ((min >= center) || (max <= center)) {
		min = 0; max = 64; center = 32; // Generic fallback
	}

	int offset = pos - center;
	if (offset > 0) {
		return clampf((float)offset / (float)(max - center), 0.0f, 1.0f);
	} else {
		return clampf((float)offset / (float)(center - min), -1.0f, 0.0f);
	}
}
#endif

/****************************************************************************
 * UpdatePads
 * Scans all controllers, combines states, and updates controllers
 ***************************************************************************/
void UpdatePads()
{
	PAD_ScanPads();
	#ifdef HW_RVL
	WPAD_ScanPads();
	WiiDRC_ScanPads();
	#endif

	// For now, assuming a standard 60Hz UI update loop (16.6ms).
	float deltaTime = 1.0f / 60.0f;

	for(int i = 0; i < 4; i++)
	{
		if(!userInput[i]) continue;

		GuiInputPadData padData;

		// Process GameCube Controller
		padData.buttons_d |= MapPADToGeneric(PAD_ButtonsDown(i));
		padData.buttons_h |= MapPADToGeneric(PAD_ButtonsHeld(i));
		padData.buttons_r |= MapPADToGeneric(PAD_ButtonsUp(i));

		// Normalize GC sticks
		float padStickX = clampf((float)PAD_StickX(i) / 128.0f, -1.0f, 1.0f);
		float padStickY = clampf((float)PAD_StickY(i) / 128.0f, -1.0f, 1.0f);
		float padSubX   = clampf((float)PAD_SubStickX(i) / 128.0f, -1.0f, 1.0f);
		float padSubY   = clampf((float)PAD_SubStickY(i) / 128.0f, -1.0f, 1.0f);

		#ifdef HW_DOL
		padData.stickX = padStickX;
		padData.stickY = padStickY;
		padData.substickX = padSubX;
		padData.substickY = padSubY;
		#else
		// Process Wiimote/Classic
		padData.buttons_d |= MapWPADToGeneric(WPAD_ButtonsDown(i));
		padData.buttons_h |= MapWPADToGeneric(WPAD_ButtonsHeld(i));
		padData.buttons_r |= MapWPADToGeneric(WPAD_ButtonsUp(i));

		WPADData * wpad = WPAD_Data(i);
		float wpadStickX = 0.0f, wpadStickY = 0.0f;
		float wpadSubX = 0.0f, wpadSubY = 0.0f;

		if (wpad != nullptr) {
			// IR Processing
			if (wpad->ir.valid) {
				padData.validPointer = true;
				padData.isTouch = false;
				padData.cursor_x = wpad->ir.x;
				padData.cursor_y = wpad->ir.y;
				padData.cursor_angle = wpad->ir.angle;
			}

			// Extract WPAD Analog Sticks (Nunchuk & Classic)
			if (wpad->exp.type == WPAD_EXP_NUNCHUK) {
				joystick_t* js = &wpad->exp.nunchuk.js;
				wpadStickX = NormalizeWPADAnalog(js->pos.x, js->min.x, js->max.x, js->center.x);
				wpadStickY = NormalizeWPADAnalog(js->pos.y, js->min.y, js->max.y, js->center.y);
			}
			else if (wpad->exp.type == WPAD_EXP_CLASSIC) {
				joystick_t* ljs = &wpad->exp.classic.ljs;
				joystick_t* rjs = &wpad->exp.classic.rjs;
				wpadStickX = NormalizeWPADAnalog(ljs->pos.x, ljs->min.x, ljs->max.x, ljs->center.x);
				wpadStickY = NormalizeWPADAnalog(ljs->pos.y, ljs->min.y, ljs->max.y, ljs->center.y);
				wpadSubX = NormalizeWPADAnalog(rjs->pos.x, rjs->min.x, rjs->max.x, rjs->center.x);
				wpadSubY = NormalizeWPADAnalog(rjs->pos.y, rjs->min.y, rjs->max.y, rjs->center.y);
			}

			// Detect Wiimote orientation via accelerometer gravity vector
			if (wpad->exp.type == WPAD_EXP_NONE) {
				bool isHorizontal = (fabs(wpad->gforce.x) > fabs(wpad->gforce.y));
				userInput[i]->setSideways(isHorizontal);
			} else {
				userInput[i]->setSideways(false);
			}
		}

		// Process Wii U Gamepad
		float drcStickX = 0.0f, drcStickY = 0.0f;
		float drcSubX = 0.0f, drcSubY = 0.0f;

		if(i == 0 && WiiDRC_Inited() && WiiDRC_Connected()) {
			padData.buttons_d |= MapWiiUGamepadToGeneric(WiiDRC_ButtonsDown());
			padData.buttons_h |= MapWiiUGamepadToGeneric(WiiDRC_ButtonsHeld());
			padData.buttons_r |= MapWiiUGamepadToGeneric(WiiDRC_ButtonsUp());
			drcStickX = clampf((float)WiiDRC_lStickX() / 128.0f, -1.0f, 1.0f);
			drcStickY = clampf((float)WiiDRC_lStickY() / 128.0f, -1.0f, 1.0f);
			drcSubX   = clampf((float)WiiDRC_rStickX() / 128.0f, -1.0f, 1.0f);
			drcSubY   = clampf((float)WiiDRC_rStickY() / 128.0f, -1.0f, 1.0f);
		}

		// Merge Analog Sticks (Priority Magnitude Logic)
		// Takes the stick with the strongest input to prevent resting drift
		// from secondary plugged-in controllers from overriding active ones.
		auto MergeAnalog = [](float gc, float wpad, float drc) -> float {
			float max_val = 0.0f;
			if (std::abs(gc) > std::abs(max_val)) max_val = gc;
			if (std::abs(wpad) > std::abs(max_val)) max_val = wpad;
			if (std::abs(drc) > std::abs(max_val)) max_val = drc;
			return max_val;
		};

		padData.stickX = MergeAnalog(padStickX, wpadStickX, drcStickX);
		padData.stickY = MergeAnalog(padStickY, wpadStickY, drcStickY);
		padData.substickX = MergeAnalog(padSubX, wpadSubX, drcSubX);
		padData.substickY = MergeAnalog(padSubY, wpadSubY, drcSubY);
		#endif

		// Push the finalized, merged payload to the controller abstraction
		userInput[i]->update(padData, deltaTime);
	}
}

/****************************************************************************
 * SetupPads
 * Allocates controllers and initializes hardware
 ***************************************************************************/
void SetupPads()
{
	PAD_Init();

	#ifdef HW_RVL
	WPAD_Init();
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, screenwidth, screenheight);
	#endif

	for(int i = 0; i < 4; i++)
	{
		if(!userInput[i]) {
			userInput[i] = new GuiInputController(i);
		}
	}
}

/****************************************************************************
 * ShutoffRumble
 ***************************************************************************/
void ShutoffRumble()
{
	#ifdef HW_RVL
	for(int i = 0; i < 4; i++)
	{
		WPAD_Rumble(i, 0);
		rumbleCount[i] = 0;
	}
	#endif
}

/****************************************************************************
 * DoRumble
 ***************************************************************************/
void DoRumble(int i)
{
	#ifdef HW_RVL
	if(rumbleRequest[i] && rumbleCount[i] < 3)
	{
		WPAD_Rumble(i, 1); // Rumble ON
		rumbleCount[i]++;
	}
	else if(rumbleRequest[i])
	{
		rumbleCount[i] = 12;
		rumbleRequest[i] = 0;
	}
	else
	{
		if(rumbleCount[i])
			rumbleCount[i]--;
		WPAD_Rumble(i, 0); // Rumble OFF
	}
	#endif
}
