/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutAudioDriver.cpp
 ***************************************************************************/
#include <coreinit/cache.h>
#include <string.h>
#include <unistd.h>

#include "WutAudioDriver.h"

static WutAudioDriver *instance = nullptr;

static void wut_frame_callback() {
	if (instance)
		instance->handleStreamCallback();
}

void WutAudioDriver::init() {
	instance = this;
	AXInit();
	AXRegisterFrameCallback(wut_frame_callback);

	nextVoiceSlot = 0;

	// Pre-allocate the 16 hardware SFX voices
	for (int i = 0; i < 16; i++) {
		voices[i].voice = AXAcquireVoice(31, 0, 0);
		voices[i].active = false;
		if (voices[i].voice) {
			AXVoiceBegin(voices[i].voice);
			AXSetVoiceType(voices[i].voice, 0);

			AXVoiceDeviceMixData mix;
			memset(&mix, 0, sizeof(mix));
			mix.bus[0].volume = 0x8000;
			mix.bus[0].delta = 0;

			AXSetVoiceDeviceMix(voices[i].voice, (AXDeviceType) 0, 0, &mix);
			AXSetVoiceDeviceMix(voices[i].voice, (AXDeviceType) 1, 0, &mix);
			AXVoiceEnd(voices[i].voice);
		}
	}

	// Allocate the dedicated background stream voice
	streamVoice = AXAcquireVoice(31, 0, 0);
	if (streamVoice) {
		AXVoiceBegin(streamVoice);
		AXSetVoiceType(streamVoice, 0);

		AXVoiceDeviceMixData mix;
		memset(&mix, 0, sizeof(mix));
		mix.bus[0].volume = 0x8000;
		mix.bus[0].delta = 0;

		AXSetVoiceDeviceMix(streamVoice, (AXDeviceType) 0, 0, &mix);
		AXSetVoiceDeviceMix(streamVoice, (AXDeviceType) 1, 0, &mix);
		AXVoiceEnd(streamVoice);
	}

	streamVolume = 255;
	writeOffset = 0;
	eofSilenceWritten = false;
}

void WutAudioDriver::shutdown() {
	stopStream();
	AXRegisterFrameCallback(nullptr);

	for (int i = 0; i < 16; i++) {
		if (voices[i].voice) {
			AXFreeVoice(voices[i].voice);
			voices[i].voice = nullptr;
		}
	}

	if (streamVoice) {
		AXFreeVoice(streamVoice);
		streamVoice = nullptr;
	}

	AXQuit();
	instance = nullptr;
}

int32_t WutAudioDriver::playVoice(const uint8_t *data, int32_t length, int volume) {
	int voiceIdx = nextVoiceSlot;
	nextVoiceSlot = (nextVoiceSlot + 1) % 16;

	WutVoiceSlot &slot = voices[voiceIdx];
	if (!slot.voice)
		return -1;

	AXSetVoiceState(slot.voice, 0); // Stop current if active
	slot.active = true;

	AXVoiceOffsets offsets;
	memset(&offsets, 0, sizeof(offsets));
	offsets.data = data;
	offsets.dataType = AX_VOICE_FORMAT_LPCM16;
	offsets.loopingEnabled = AX_VOICE_LOOP_DISABLED;
	offsets.endOffset = length / 2; // Conversion: bytes to 16-bit samples
	AXSetVoiceOffsets(slot.voice, &offsets);

	AXVoiceSrc src;
	memset(&src, 0, sizeof(src));
	uint32_t samplesPerSec = AXGetInputSamplesPerSec();
	src.ratio = (uint32_t)(0x00010000 * ((float) 48000 / (float) samplesPerSec));
	AXSetVoiceSrc(slot.voice, &src);
	AXSetVoiceSrcType(slot.voice, 1);

	AXVoiceVeData veData;
	veData.volume = (volume << 7); // Remap 0-255 to ~0-0x8000
	veData.delta = 0;
	AXSetVoiceVe(slot.voice, &veData);

	AXSetVoiceState(slot.voice, 1);
	return voiceIdx;
}

void WutAudioDriver::stopVoice(int32_t voice) {
	if (voice >= 0 && voice < 16 && voices[voice].voice) {
		AXSetVoiceState(voices[voice].voice, 0);
		voices[voice].active = false;
	}
}

void WutAudioDriver::pauseVoice(int32_t voice) {
	if (voice >= 0 && voice < 16 && voices[voice].voice)
		AXSetVoiceState(voices[voice].voice, 0);
}

void WutAudioDriver::resumeVoice(int32_t voice) {
	if (voice >= 0 && voice < 16 && voices[voice].voice)
		AXSetVoiceState(voices[voice].voice, 1);
}

bool WutAudioDriver::isVoicePlaying(int32_t voice) {
	if (voice >= 0 && voice < 16 && voices[voice].voice) {
		return voices[voice].active && (voices[voice].voice->state == AX_VOICE_STATE_PLAYING);
	}
	return false;
}

void WutAudioDriver::setVoiceVolume(int32_t voice, int volume) {
	if (voice >= 0 && voice < 16 && voices[voice].voice) {
		AXVoiceVeData veData;
		veData.volume = (volume << 7);
		veData.delta = 0;
		AXSetVoiceVe(voices[voice].voice, &veData);
	}
}

void WutAudioDriver::playStream(const uint8_t *data, int32_t length, bool loop, int volume) {
	stopStream();
	streamVolume = volume;

	// Scrub the entire buffer with silence to guarantee a clean start
	memset(streamBuf, 0, sizeof(streamBuf));
	DCFlushRange(streamBuf, sizeof(streamBuf));

	writeOffset = 0;
	eofSilenceWritten = false;

	if (oggPlayer.play(data, length, 0, loop)) {
		int32_t readySize = 0;
		const uint8_t *pcm = nullptr;

		while (oggPlayer.isPlaying() && !(pcm = oggPlayer.getReadyBuffer(&readySize))) {
			usleep(1000);
		}

		if (pcm && readySize > 0) {
			int samples = readySize / (oggPlayer.getChannels() == 2 ? 4 : 2);

			if (oggPlayer.getChannels() == 2) {
				int16_t *pcm16 = (int16_t*) pcm;
				for (int i = 0; i < samples; i++) {
					streamBuf[i] = (int16_t)(((int32_t)pcm16[i * 2] + (int32_t)pcm16[i * 2 + 1]) / 2);
				}
			} else {
				memcpy(streamBuf, pcm, readySize);
			}

			DCFlushRange(streamBuf, samples * 2);
			writeOffset = samples;
			oggPlayer.consumeBuffer();

			AXSetVoiceState(streamVoice, 0);

			// Lock the hardware to a fixed, infinite loop covering the entire physical buffer
			AXVoiceOffsets offsets;
			memset(&offsets, 0, sizeof(offsets));
			offsets.data = streamBuf;
			offsets.dataType = AX_VOICE_FORMAT_LPCM16;
			offsets.loopingEnabled = AX_VOICE_LOOP_ENABLED;
			offsets.loopOffset = 0;
			offsets.endOffset = STREAM_BUFFER_SAMPLES;
			AXSetVoiceOffsets(streamVoice, &offsets);

			AXVoiceSrc src;
			memset(&src, 0, sizeof(src));
			uint32_t samplesPerSec = AXGetInputSamplesPerSec();
			src.ratio = (uint32_t)(0x00010000 * ((float) oggPlayer.getSampleRate() / (float) samplesPerSec));
			AXSetVoiceSrc(streamVoice, &src);
			AXSetVoiceSrcType(streamVoice, 1);

			AXVoiceVeData veData;
			veData.volume = (streamVolume << 7);
			veData.delta = 0;
			AXSetVoiceVe(streamVoice, &veData);

			AXSetVoiceState(streamVoice, 1);
		}
	}
}

void WutAudioDriver::handleStreamCallback() {
	if (!streamVoice || oggPlayer.isPaused() || streamVoice->state != AX_VOICE_STATE_PLAYING)
		return;

	// Query exactly where the hardware is right now. streamBuf is the required base pointer.
	uint32_t currentOffset = AXGetVoiceCurrentOffsetEx(streamVoice, streamBuf);

	// Calculate how many valid, unplayed samples are queued ahead of the hardware playhead
	uint32_t queuedSamples = (writeOffset - currentOffset + STREAM_BUFFER_SAMPLES) % STREAM_BUFFER_SAMPLES;

	// If we have less than half a buffer (16KB / 8192 samples) queued, ask the decoder for more
	if (queuedSamples < (STREAM_BUFFER_SAMPLES / 2)) {
		int32_t readySize = 0;
		const uint8_t *pcm = oggPlayer.getReadyBuffer(&readySize);

		if (pcm && readySize > 0) {
			int samples = readySize / (oggPlayer.getChannels() == 2 ? 4 : 2);

			// Handle physical wrap-around if the new chunk crosses the end of streamBuf
			int samplesToEnd = STREAM_BUFFER_SAMPLES - writeOffset;
			int write1 = (samples < samplesToEnd) ? samples : samplesToEnd;
			int write2 = samples - write1;

			if (oggPlayer.getChannels() == 2) {
				int16_t *pcm16 = (int16_t*) pcm;
				for (int i = 0; i < write1; i++) {
					streamBuf[writeOffset + i] = (int16_t)(((int32_t)pcm16[i * 2] + (int32_t)pcm16[i * 2 + 1]) / 2);
				}
				if (write2 > 0) {
					for (int i = 0; i < write2; i++) {
						streamBuf[i] = (int16_t)(((int32_t)pcm16[(write1 + i) * 2] + (int32_t)pcm16[(write1 + i) * 2 + 1]) / 2);
					}
				}
			} else {
				memcpy(&streamBuf[writeOffset], pcm, write1 * 2);
				if (write2 > 0) {
					memcpy(&streamBuf[0], pcm + (write1 * 2), write2 * 2);
				}
			}

			if (write1 > 0) DCFlushRange(&streamBuf[writeOffset], write1 * 2);
			if (write2 > 0) DCFlushRange(&streamBuf[0], write2 * 2);

			writeOffset = (writeOffset + samples) % STREAM_BUFFER_SAMPLES;
			oggPlayer.consumeBuffer();

		} else if (!oggPlayer.isPlaying()) {

			// Natural End-Of-File reached. Scrub the unwritten remainder of the buffer with silence
			// so the DSP doesn't play old loop garbage before halting.
			if (!eofSilenceWritten) {
				int clearCount = STREAM_BUFFER_SAMPLES - queuedSamples;
				if (clearCount > 0) {
					int clear1 = (clearCount < (STREAM_BUFFER_SAMPLES - writeOffset)) ? clearCount : (STREAM_BUFFER_SAMPLES - writeOffset);
					int clear2 = clearCount - clear1;

					memset(&streamBuf[writeOffset], 0, clear1 * 2);
					DCFlushRange(&streamBuf[writeOffset], clear1 * 2);

					if (clear2 > 0) {
						memset(&streamBuf[0], 0, clear2 * 2);
						DCFlushRange(&streamBuf[0], clear2 * 2);
					}
				}
				eofSilenceWritten = true;
			}

			// Once the hardware finishes playing the very last valid samples, halt it.
			if (queuedSamples < 128) {
				AXSetVoiceState(streamVoice, 0);
			}
		}
	}
}

void WutAudioDriver::stopStream() {
	if (streamVoice)
		AXSetVoiceState(streamVoice, 0);
	oggPlayer.stop();
}

void WutAudioDriver::pauseStream() {
	if (streamVoice)
		AXSetVoiceState(streamVoice, 0);
	oggPlayer.pause(true);
}

void WutAudioDriver::resumeStream() {
	if (streamVoice)
		AXSetVoiceState(streamVoice, 1);
	oggPlayer.pause(false);
}

bool WutAudioDriver::isStreamPlaying() {
	return oggPlayer.isPlaying();
}

void WutAudioDriver::setStreamVolume(int volume) {
	streamVolume = volume;
	if (streamVoice) {
		AXVoiceVeData veData;
		veData.volume = (volume << 7);
		veData.delta = 0;
		AXSetVoiceVe(streamVoice, &veData);
	}
}
