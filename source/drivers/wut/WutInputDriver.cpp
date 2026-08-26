/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutInputDriver.cpp
 ***************************************************************************/
#include "WutInputDriver.h"
#include "../../libgui/GuiInputController.h"
#include "../../libgui/GuiInput.h"

#include <vpad/input.h>
#include <padscore/kpad.h>
#include <padscore/wpad.h>
#include <cmath>
#include <algorithm>

static uint8_t vpadRumblePattern[15] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static inline float clampf(float v, float lo, float hi) {
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/****************************************************************************
 * Hardware Mapping Helpers
 ***************************************************************************/
static uint32_t MapVPADToGeneric(uint32_t vpad_btns) {
	uint32_t mask = GUI_BTN_NONE;
	if (vpad_btns & VPAD_BUTTON_A) mask |= GUI_BTN_A;
	if (vpad_btns & VPAD_BUTTON_B) mask |= GUI_BTN_B;
	if (vpad_btns & VPAD_BUTTON_X) mask |= GUI_BTN_X;
	if (vpad_btns & VPAD_BUTTON_Y) mask |= GUI_BTN_Y;
	if (vpad_btns & VPAD_BUTTON_UP) mask |= GUI_BTN_UP;
	if (vpad_btns & VPAD_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (vpad_btns & VPAD_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (vpad_btns & VPAD_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (vpad_btns & VPAD_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (vpad_btns & VPAD_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (vpad_btns & VPAD_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (vpad_btns & VPAD_BUTTON_L) mask |= GUI_TRIGGER_L;
	if (vpad_btns & VPAD_BUTTON_R) mask |= GUI_TRIGGER_R;
	if (vpad_btns & VPAD_BUTTON_ZL) mask |= GUI_TRIGGER_ZL;
	if (vpad_btns & VPAD_BUTTON_ZR) mask |= GUI_TRIGGER_ZR;
	if (vpad_btns & VPAD_BUTTON_STICK_L) mask |= GUI_THUMB_L;
	if (vpad_btns & VPAD_BUTTON_STICK_R) mask |= GUI_THUMB_R;
	return mask;
}

static uint32_t MapKPADCoreToGeneric(uint32_t kpad_btns) {
	uint32_t mask = GUI_BTN_NONE;
	if (kpad_btns & WPAD_BUTTON_A) mask |= GUI_BTN_A;
	if (kpad_btns & WPAD_BUTTON_B) mask |= GUI_BTN_B;
	if (kpad_btns & WPAD_BUTTON_1) mask |= GUI_BTN_1;
	if (kpad_btns & WPAD_BUTTON_2) mask |= GUI_BTN_2;
	if (kpad_btns & WPAD_BUTTON_UP) mask |= GUI_BTN_UP;
	if (kpad_btns & WPAD_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (kpad_btns & WPAD_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (kpad_btns & WPAD_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (kpad_btns & WPAD_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (kpad_btns & WPAD_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (kpad_btns & WPAD_BUTTON_HOME) mask |= GUI_BTN_HOME;
	return mask;
}

static uint32_t MapKPADProToGeneric(uint32_t pro_btns) {
	uint32_t mask = GUI_BTN_NONE;
	if (pro_btns & WPAD_PRO_BUTTON_A) mask |= GUI_BTN_A;
	if (pro_btns & WPAD_PRO_BUTTON_B) mask |= GUI_BTN_B;
	if (pro_btns & WPAD_PRO_BUTTON_X) mask |= GUI_BTN_X;
	if (pro_btns & WPAD_PRO_BUTTON_Y) mask |= GUI_BTN_Y;
	if (pro_btns & WPAD_PRO_BUTTON_UP) mask |= GUI_BTN_UP;
	if (pro_btns & WPAD_PRO_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (pro_btns & WPAD_PRO_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (pro_btns & WPAD_PRO_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (pro_btns & WPAD_PRO_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (pro_btns & WPAD_PRO_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (pro_btns & WPAD_PRO_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (pro_btns & WPAD_PRO_TRIGGER_L) mask |= GUI_TRIGGER_L;
	if (pro_btns & WPAD_PRO_TRIGGER_R) mask |= GUI_TRIGGER_R;
	if (pro_btns & WPAD_PRO_TRIGGER_ZL) mask |= GUI_TRIGGER_ZL;
	if (pro_btns & WPAD_PRO_TRIGGER_ZR) mask |= GUI_TRIGGER_ZR;
	if (pro_btns & WPAD_PRO_BUTTON_STICK_L) mask |= GUI_THUMB_L;
	if (pro_btns & WPAD_PRO_BUTTON_STICK_R) mask |= GUI_THUMB_R;
	return mask;
}

static uint32_t MapKPADClassicToGeneric(uint32_t cls_btns) {
	uint32_t mask = GUI_BTN_NONE;
	if (cls_btns & WPAD_CLASSIC_BUTTON_A) mask |= GUI_BTN_A;
	if (cls_btns & WPAD_CLASSIC_BUTTON_B) mask |= GUI_BTN_B;
	if (cls_btns & WPAD_CLASSIC_BUTTON_X) mask |= GUI_BTN_X;
	if (cls_btns & WPAD_CLASSIC_BUTTON_Y) mask |= GUI_BTN_Y;
	if (cls_btns & WPAD_CLASSIC_BUTTON_UP) mask |= GUI_BTN_UP;
	if (cls_btns & WPAD_CLASSIC_BUTTON_DOWN) mask |= GUI_BTN_DOWN;
	if (cls_btns & WPAD_CLASSIC_BUTTON_LEFT) mask |= GUI_BTN_LEFT;
	if (cls_btns & WPAD_CLASSIC_BUTTON_RIGHT) mask |= GUI_BTN_RIGHT;
	if (cls_btns & WPAD_CLASSIC_BUTTON_PLUS) mask |= GUI_BTN_PLUS;
	if (cls_btns & WPAD_CLASSIC_BUTTON_MINUS) mask |= GUI_BTN_MINUS;
	if (cls_btns & WPAD_CLASSIC_BUTTON_HOME) mask |= GUI_BTN_HOME;
	if (cls_btns & WPAD_CLASSIC_BUTTON_L) mask |= GUI_TRIGGER_L;
	if (cls_btns & WPAD_CLASSIC_BUTTON_R) mask |= GUI_TRIGGER_R;
	if (cls_btns & WPAD_CLASSIC_BUTTON_ZL) mask |= GUI_TRIGGER_ZL;
	if (cls_btns & WPAD_CLASSIC_BUTTON_ZR) mask |= GUI_TRIGGER_ZR;
	return mask;
}

WutInputDriver::WutInputDriver() {
	for (int i = 0; i < 4; i++) {
		rumbleCount[i] = 0;
		rumbleRequest[i] = false;
	}
}

WutInputDriver::~WutInputDriver() {
	shutdown();
}

void WutInputDriver::init() {
	KPADInit();
	VPADInit();
	InitUserInputControllers();
}

void WutInputDriver::shutdown() {
	for (int i = 0; i < 4; i++) {
		WPADControlMotor((WPADChan)i, FALSE);
		rumbleCount[i] = 0;
		rumbleRequest[i] = false;
	}
	VPADStopMotor(VPAD_CHAN_0);
}

void WutInputDriver::setRumble(int channel, bool rumble) {
	if (channel >= 0 && channel < 4) {
		rumbleRequest[channel] = rumble;
	}
}

void WutInputDriver::update(float deltaTime) {
	for(int i = 3; i >= 0; i--) {
		GuiInputPadData padData;

		// VPAD Processing (GamePad, Channel 0 Only)
		if (i == 0) {
			VPADStatus vpadStatus;
			VPADReadError vpadError;
			VPADRead(VPAD_CHAN_0, &vpadStatus, 1, &vpadError);

			if (vpadError == VPAD_READ_NO_SAMPLES) {
				padData.hw_connected[GUI_HW_DRC] = true;
			}
			else if (vpadError == VPAD_READ_SUCCESS) {
				padData.hw_connected[GUI_HW_DRC] = true;

				padData.hw_buttons_d[GUI_HW_DRC] = MapVPADToGeneric(vpadStatus.trigger);
				padData.hw_buttons_h[GUI_HW_DRC] = MapVPADToGeneric(vpadStatus.hold);
				padData.hw_buttons_r[GUI_HW_DRC] = MapVPADToGeneric(vpadStatus.release);

				padData.hw_stickX[GUI_HW_DRC] = clampf(vpadStatus.leftStick.x, -1.0f, 1.0f);
				padData.hw_stickY[GUI_HW_DRC] = clampf(vpadStatus.leftStick.y, -1.0f, 1.0f);
				padData.hw_substickX[GUI_HW_DRC] = clampf(vpadStatus.rightStick.x, -1.0f, 1.0f);
				padData.hw_substickY[GUI_HW_DRC] = clampf(vpadStatus.rightStick.y, -1.0f, 1.0f);

				// Map Touch Screen to pointer coordinates
				if (vpadStatus.tpNormal.touched) {
					VPADTouchData calib;
					VPADGetTPCalibratedPoint(VPAD_CHAN_0, &calib, &vpadStatus.tpNormal);
					padData.isTouch = true;
					padData.validPointer = true;
					padData.cursor_x = calib.x;
					padData.cursor_y = calib.y;
				}
			}
		}

		// KPAD Processing (Wiimotes & Pro Controllers)
		KPADStatus kpadStatus;
		int kpadRead = KPADRead((KPADChan)i, &kpadStatus, 1);

		if (kpadRead > 0) {
			padData.hw_connected[GUI_HW_WIIMOTE] = true;

			if (kpadStatus.extensionType == WPAD_EXT_PRO_CONTROLLER) {
				padData.hw_connected[GUI_HW_WUPC] = true;
				padData.hw_buttons_d[GUI_HW_WUPC] = MapKPADProToGeneric(kpadStatus.pro.trigger);
				padData.hw_buttons_h[GUI_HW_WUPC] = MapKPADProToGeneric(kpadStatus.pro.hold);
				padData.hw_buttons_r[GUI_HW_WUPC] = MapKPADProToGeneric(kpadStatus.pro.release);

				padData.hw_stickX[GUI_HW_WUPC] = clampf(kpadStatus.pro.leftStick.x, -1.0f, 1.0f);
				padData.hw_stickY[GUI_HW_WUPC] = clampf(kpadStatus.pro.leftStick.y, -1.0f, 1.0f);
				padData.hw_substickX[GUI_HW_WUPC] = clampf(kpadStatus.pro.rightStick.x, -1.0f, 1.0f);
				padData.hw_substickY[GUI_HW_WUPC] = clampf(kpadStatus.pro.rightStick.y, -1.0f, 1.0f);
			}
			else if (kpadStatus.extensionType == WPAD_EXT_CLASSIC || kpadStatus.extensionType == WPAD_EXT_MPLUS_CLASSIC) {
				padData.hw_connected[GUI_HW_CLASSIC] = true;
				padData.hw_buttons_d[GUI_HW_CLASSIC] = MapKPADClassicToGeneric(kpadStatus.classic.trigger);
				padData.hw_buttons_h[GUI_HW_CLASSIC] = MapKPADClassicToGeneric(kpadStatus.classic.hold);
				padData.hw_buttons_r[GUI_HW_CLASSIC] = MapKPADClassicToGeneric(kpadStatus.classic.release);

				padData.hw_stickX[GUI_HW_CLASSIC] = clampf(kpadStatus.classic.leftStick.x, -1.0f, 1.0f);
				padData.hw_stickY[GUI_HW_CLASSIC] = clampf(kpadStatus.classic.leftStick.y, -1.0f, 1.0f);
				padData.hw_substickX[GUI_HW_CLASSIC] = clampf(kpadStatus.classic.rightStick.x, -1.0f, 1.0f);
				padData.hw_substickY[GUI_HW_CLASSIC] = clampf(kpadStatus.classic.rightStick.y, -1.0f, 1.0f);
			}
			else {
				// Core Wiimote or Nunchuk
				padData.hw_buttons_d[GUI_HW_WIIMOTE] = MapKPADCoreToGeneric(kpadStatus.trigger);
				padData.hw_buttons_h[GUI_HW_WIIMOTE] = MapKPADCoreToGeneric(kpadStatus.hold);
				padData.hw_buttons_r[GUI_HW_WIIMOTE] = MapKPADCoreToGeneric(kpadStatus.release);

				if (kpadStatus.posValid) {
					padData.validPointer = true;
					padData.isTouch = false;
					padData.cursor_x = kpadStatus.pos.x;
					padData.cursor_y = kpadStatus.pos.y;
				}

				if (kpadStatus.extensionType == WPAD_EXT_NUNCHUK || kpadStatus.extensionType == WPAD_EXT_MPLUS_NUNCHUK) {
					padData.hw_connected[GUI_HW_NUNCHUK] = true;
					if(kpadStatus.nunchuk.trigger & WPAD_NUNCHUK_BUTTON_Z) padData.hw_buttons_d[GUI_HW_NUNCHUK] |= GUI_TRIGGER_ZL;
					if(kpadStatus.nunchuk.trigger & WPAD_NUNCHUK_BUTTON_C) padData.hw_buttons_d[GUI_HW_NUNCHUK] |= GUI_TRIGGER_L;
					if(kpadStatus.nunchuk.hold & WPAD_NUNCHUK_BUTTON_Z) padData.hw_buttons_h[GUI_HW_NUNCHUK] |= GUI_TRIGGER_ZL;
					if(kpadStatus.nunchuk.hold & WPAD_NUNCHUK_BUTTON_C) padData.hw_buttons_h[GUI_HW_NUNCHUK] |= GUI_TRIGGER_L;

					padData.hw_stickX[GUI_HW_NUNCHUK] = clampf(kpadStatus.nunchuk.stick.x, -1.0f, 1.0f);
					padData.hw_stickY[GUI_HW_NUNCHUK] = clampf(kpadStatus.nunchuk.stick.y, -1.0f, 1.0f);
				}
			}
		}

		// Merge Aggregate State
		for (uint32_t hw = 0; hw < GUI_HW_MAX; hw++) {
			if (!padData.hw_connected[hw]) continue;

			padData.buttons_d |= padData.hw_buttons_d[hw];
			padData.buttons_h |= padData.hw_buttons_h[hw];
			padData.buttons_r |= padData.hw_buttons_r[hw];

			if (std::abs(padData.hw_stickX[hw]) > std::abs(padData.stickX)) padData.stickX = padData.hw_stickX[hw];
			if (std::abs(padData.hw_stickY[hw]) > std::abs(padData.stickY)) padData.stickY = padData.hw_stickY[hw];
			if (std::abs(padData.hw_substickX[hw]) > std::abs(padData.substickX)) padData.substickX = padData.hw_substickX[hw];
			if (std::abs(padData.hw_substickY[hw]) > std::abs(padData.substickY)) padData.substickY = padData.hw_substickY[hw];
		}

		userInput[i]->update(padData, deltaTime);

		// Rumble Lifecycle
		if (rumbleRequest[i] && rumbleCount[i] < 3) {
			if(padData.hw_connected[GUI_HW_WIIMOTE] || padData.hw_connected[GUI_HW_WUPC]) WPADControlMotor((WPADChan)i, TRUE);
			if(i == 0 && padData.hw_connected[GUI_HW_DRC]) VPADControlMotor(VPAD_CHAN_0, vpadRumblePattern, 120);
			rumbleCount[i]++;
		} else if (rumbleRequest[i]) {
			rumbleCount[i] = 12;
			rumbleRequest[i] = false;
		} else {
			if (rumbleCount[i]) rumbleCount[i]--;
			WPADControlMotor((WPADChan)i, FALSE);
			if (i == 0) VPADStopMotor(VPAD_CHAN_0);
		}
	}
}
