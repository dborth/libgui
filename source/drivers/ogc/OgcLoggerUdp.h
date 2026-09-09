/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * OgcLoggerUdp.h
 *
 * Non-blocking UDP log streaming for Wii (HW_RVL). GameCube BBA is not
 * supported yet, so on a GameCube build init() always reports failure and
 * write() is a no-op - safe to register unconditionally from shared
 * driver-wiring code without an #ifdef at the call site.
 ***************************************************************************/
#pragma once

#include "../Logger.h"

#ifdef HW_RVL
#include <network.h>
#endif

class OgcLoggerUdp : public LoggingDriver
{
	public:
		bool init(const LogConfig & config) override;
		void shutdown() override;
		void write(LogLevel level, const char * line, size_t len) override;
		const char * name() const override { return "UDP"; }

	private:
#ifdef HW_RVL
		s32 sock = -1;
		struct sockaddr_in serverAddr {};
#endif
};
