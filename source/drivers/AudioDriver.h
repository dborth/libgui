/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * AudioDriver.h
 *
 * Platform audio backend GuiSound delegates to. Exactly one driver
 * implements this and assigns the single global instance below
 ***************************************************************************/
#pragma once

#include <stdint.h>

//!Platform audio backend GuiSound delegates to: fixed one-shot PCM voices
//!plus one background OGG stream. Exactly one driver implements this and
//!assigns the single global Platform instance.
class AudioDriver
{
	public:
		virtual ~AudioDriver() = default;

		virtual void init() = 0;
		virtual void shutdown() = 0;
		//!Starts the audio backend running (eg. registers the DSP/AX callback).
		virtual void start() = 0;
		//!Stops the audio backend.
		virtual void stop() = 0;

		//!Start a one-shot/short PCM voice. Returns a backend-defined
		//!voice handle (>=0) on success, or a negative value if no voice
		//!was available.
		virtual int32_t playVoice(const uint8_t * data, int32_t length, int volume) = 0;
		virtual void stopVoice(int32_t voice) = 0;
		virtual void pauseVoice(int32_t voice) = 0;
		virtual void resumeVoice(int32_t voice) = 0;
		virtual bool isVoicePlaying(int32_t voice) = 0;
		virtual void setVoiceVolume(int32_t voice, int volume) = 0;

		//!Streamed (OGG) playback. There is no per-call stream handle,
		//!only one can play at a time.
		virtual void playStream(const uint8_t * data, int32_t length, bool loop, int volume) = 0;
		virtual void stopStream() = 0;
		virtual void pauseStream() = 0;
		virtual void resumeStream() = 0;
		virtual bool isStreamPlaying() = 0;
		virtual void setStreamVolume(int volume) = 0;
};
