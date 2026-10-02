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
//!\ingroup grp_pal
class AudioDriver
{
	public:
		virtual ~AudioDriver() = default;

		//!Allocates the backend's audio resources. Does not start playback; see start().
		virtual void init() = 0;
		//!Releases everything init() allocated. Call stop() first.
		virtual void shutdown() = 0;
		//!Starts the audio backend running (eg. registers the DSP/AX callback).
		virtual void start() = 0;
		//!Stops the audio backend.
		virtual void stop() = 0;

		//!Start a one-shot/short PCM voice. Returns a backend-defined
		//!voice handle (>=0) on success, or a negative value if no voice
		//!was available.
		virtual int32_t playVoice(const uint8_t * data, int32_t length, int volume) = 0;
		//!Stops a voice returned by playVoice() and frees its slot.
		virtual void stopVoice(int32_t voice) = 0;
		//!Pauses a playing voice, keeping its position and slot.
		virtual void pauseVoice(int32_t voice) = 0;
		//!Resumes a voice paused with pauseVoice().
		virtual void resumeVoice(int32_t voice) = 0;
		//!\return true while the voice is still playing (false once it has finished or been stopped).
		virtual bool isVoicePlaying(int32_t voice) = 0;
		//!Changes the volume of a playing voice.
		//!\param voice Voice handle returned by playVoice()
		//!\param volume Volume, 0-100
		virtual void setVoiceVolume(int32_t voice, int volume) = 0;

		//!Streamed (OGG) playback. There is no per-call stream handle,
		//!only one can play at a time.
		virtual void playStream(const uint8_t * data, int32_t length, bool loop, int volume) = 0;
		//!Stops the stream started by playStream().
		virtual void stopStream() = 0;
		//!Pauses the stream, keeping its position.
		virtual void pauseStream() = 0;
		//!Resumes a stream paused with pauseStream().
		virtual void resumeStream() = 0;
		//!\return true while the stream is playing.
		virtual bool isStreamPlaying() = 0;
		//!Changes the stream volume.
		//!\param volume Volume, 0-100
		virtual void setStreamVolume(int volume) = 0;
};
