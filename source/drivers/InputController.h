/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * InputController.h
 *
 * Represents a single connected logical controller.
 * Handles device-specific translation (like sideways Wiimote mapping)
 * and repeat-delay logic for UI navigation.
 ***************************************************************************/
#pragma once

#include "InputData.h"

//!Logical controller for one input channel. Translates an InputPadData snapshot (sideways Wiimote mapping, deadzone) and drives repeat-delay timing for UI navigation.
//!\ingroup grp_input
class InputController {
public:
	//!\param channel Controller channel (0-3) this controller reports for
	InputController(int channel);
	~InputController() = default;

	/**
	 * Updates the controller state. Called once per frame by the driver.
	 * @param data The raw, mapped inputs from the hardware.
	 * @param deltaTime Elapsed time since last frame in seconds.
	 */
	void update(const InputPadData& data, float deltaTime);

	//!Sets whether the Wiimote is held sideways; if so the d-pad and primary/secondary buttons are remapped (A/B become 2/1).
	void setSideways(bool s) { sideways = s; }
	//!\return true if the Wiimote is held sideways
	bool isSideways() const { return sideways; }
	//!\return the channel this controller reports for (see setChannel())
	int getChannel() const { return channel; }

	//! Temporarily overrides the channel this controller reports via getChannel().
	//! Used by list-based elements (e.g. GuiFileBrowser) to present a "no channel"
	//! (-1) identity to items the cursor isn't currently over, so a stale
	//! stateChan left on a reused slot can't block clicks from the real channel.
	//! Callers MUST restore the original value (see getChannel()) after the
	//! element update() call this wraps.
	void setChannel(int c) { channel = c; }

	//!\return the latest input snapshot passed to update()
	const InputPadData& getPadData() const { return currentData; }

	//!\return true if any button in the mask was pressed this frame (INPUT_BTN_* / INPUT_TRIGGER_*)
	bool isPressed(uint32_t logicalButtonMask) const;
	//!\return true if any button in the mask is held this frame
	bool isHeld(uint32_t logicalButtonMask) const;
	//!\return true if the primary ("accept") button was pressed this frame: A, or 2 when sideways
	bool isPrimaryPressed() const;
	//!\return true if the secondary ("cancel") button was pressed this frame: B, or 1 when sideways
	bool isSecondaryPressed() const;

	// Navigation helpers: each returns true on the initial press and then
	// repeats while the d-pad or stick is held, using the scroll delays below.
	// They account for sideways orientation.

	//!\return true when a step up is due (initial press, then auto-repeat while held)
	bool up() const;
	//!\return true when a step down is due (initial press, then auto-repeat while held)
	bool down() const;
	//!\return true when a step left is due (initial press, then auto-repeat while held)
	bool left() const;
	//!\return true when a step right is due (initial press, then auto-repeat while held)
	bool right() const;

private:
	int channel;
	bool sideways;
	InputPadData currentData;

	// Analog stick deadzone
	const float STICK_DEADZONE = 0.2f;

	// Scrolling delay timers (in seconds)
	const float SCROLL_DELAY_INITIAL = 0.3f;
	const float SCROLL_DELAY_LOOP = 0.05f;

	float scrollTimer;

	// Internal helper to process directional holds and repeats
	bool processDirection(uint32_t logicalButtonMask, float stickAxis, bool isNegativeAxis) const;

	// Mutable state to allow the const navigation functions to reset the timer
	// when a valid scroll triggers. (A common pattern to keep accessors clean).
	mutable float internalScrollTimer;
};

extern InputController* controller[4];

void InitUserInputControllers();
