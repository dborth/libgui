#pragma once

#include "AudioDriver.h"
#include "VideoDriver.h"
#include "InputDriver.h"
#include "FileSystemDriver.h"
#include "ThreadDriver.h"

class AudioDriver;
class VideoDriver;
class InputDriver;
class FileSystemDriver;
class ThreadDriver;

//! Platform execution state.
enum class Status
{
	Running,
	Paused,
	Exiting
};

//!A hardware/OS-level system event a Platform can report. These are
//!mutually exclusive by construction.
enum class SystemEvent
{
	None,
	//!Power button pressed (console or, on Wii, a Wiimote) - or, on Wii U,
	//!the OS asking the app to exit. Stop running as soon as practical.
	ShutdownRequested,
	//!Reset button pressed (Wii only).
	//!Soft-reset the currently running game and keep going.
	ResetRequested,
};

//!Composition root for a platform. Owns the five concrete drivers below
//!and is the only place app code needs an `#ifdef` to pick a platform -
//!everything else goes through the abstract driver interfaces.
class Platform
{
	public:
		virtual ~Platform() = default;

		//!Constructs and initializes all five drivers for this platform.
		//!\param width Design canvas width in pixels
		//!\param height Design canvas height in pixels
		virtual void init(int width, int height) = 0;
		//!Shuts down and releases all five drivers.
		virtual void shutdown() = 0;

		virtual AudioDriver* getAudio() = 0;
		virtual VideoDriver* getVideo() = 0;
		virtual InputDriver* getInput() = 0;
		virtual FileSystemDriver* getFileSystem() = 0;
		virtual ThreadDriver* getThread() = 0;

		//!Current hardware/OS-level system event, if any. A single query
		//!rather than independent shutdown/reset flags.
		virtual SystemEvent getSystemEvent() = 0;

		//! Current platform lifecycle state (Running, Paused, Exiting).
		virtual Status getStatus() const = 0;
		//! Transitions platform state to move to Exiting.
		virtual void triggerExit() = 0;
};

//! The globally accessible platform instance
extern Platform* platform;
