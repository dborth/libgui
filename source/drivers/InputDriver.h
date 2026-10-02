/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * InputDriver.h
 *
 * Platform input backend GuiElements delegate to.
 * Exactly one driver implements this and assigns the single global instance.
 ***************************************************************************/
#pragma once

enum {
	WIIMOTE_ORIENTATION_VERTICAL = 0,
	WIIMOTE_ORIENTATION_HORIZONTAL,
	WIIMOTE_ORIENTATION_LENGTH
};

//!Platform input backend GuiElements delegate to, via the per-channel
//!InputController objects update() feeds. Exactly one driver implements
//!this and assigns the single global Platform instance.
//!\ingroup grp_input
class InputDriver
{
	public:
		virtual ~InputDriver() = default;
		
		//!Initializes the controller subsystem.
		virtual void init() = 0;
		//!Shuts the controller subsystem down.
		virtual void shutdown() = 0;
		
		//! Polls the hardware and dispatches InputPadData payloads to the UI
		virtual void update() = 0;
		
		//! Requests a rumble event on the specified controller channel
		virtual void setRumble(int channel, bool rumble) = 0;
		
		//!Globally enables or disables rumble requested through setRumble().
		void setRumbleEnabled(bool enabled) { rumbleEnabled = enabled; }
		//!\return true if rumble is enabled
		bool isRumbleEnabled() const { return rumbleEnabled; }
		//!\param orientation WIIMOTE_ORIENTATION_VERTICAL or WIIMOTE_ORIENTATION_HORIZONTAL
		void setWiimoteOrientation(int orientation) { wiimoteOrientation = orientation; }
		//!\return the current Wiimote orientation (a WIIMOTE_ORIENTATION_* value)
		int getWiimoteOrientation() const { return wiimoteOrientation; }
	
	protected:
		bool rumbleEnabled = true; //!< see setRumbleEnabled()
		int wiimoteOrientation = WIIMOTE_ORIENTATION_VERTICAL; //!< see setWiimoteOrientation()
};
