/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2009-2026
 * WutAudioDriver.h
 ***************************************************************************/
#pragma once

#include <stdint.h>
#include <sndcore2/core.h>
#include <sndcore2/voice.h>
#include "../AudioDriver.h"
#include "../../libgui/GuiSoundOggPlayer.h"

class WutAudioDriver : public AudioDriver
{
	public:
		void init() override;
		void shutdown() override;

		int32_t playVoice(const uint8_t* data, int32_t length, int volume) override;
		void stopVoice(int32_t voice) override;
		void pauseVoice(int32_t voice) override;
		void resumeVoice(int32_t voice) override;
		bool isVoicePlaying(int32_t voice) override;
		void setVoiceVolume(int32_t voice, int volume) override;

		void playStream(const uint8_t* data, int32_t length, bool loop, int volume) override;
		void stopStream() override;
		void pauseStream() override;
		void resumeStream() override;
		bool isStreamPlaying() override;
		void setStreamVolume(int volume) override;

		void handleStreamCallback(); // Hardware frame callback hook

	private:
		struct WutVoiceSlot {
			AXVoice* voice;
			bool active;
		};

		GuiSoundOggPlayer oggPlayer;
		WutVoiceSlot voices[16];
		int nextVoiceSlot;

		AXVoice* streamVoice;

		// Circular Ring Buffer (16,384 samples = 32KB)
		static const uint32_t STREAM_BUFFER_SAMPLES = 16384;
		alignas(32) int16_t streamBuf[STREAM_BUFFER_SAMPLES];

		uint32_t writeOffset;
		bool eofSilenceWritten;
		int streamVolume;
};
