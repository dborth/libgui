/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * LoggerFile.h
 *
 * Generic - works unmodified on GC, Wii, and Wii U. All three platforms
 * expose their mounted storage through a standard devoptab path (eg.
 * "sd:/debug.log" on GC/Wii, "/vol/external01/debug.log" on Wii U, or
 * whatever a caller passes in LogConfig::filePath), so a single stdio-
 * based implementation covers every platform. There is nothing 
 * platform-specific left to put behind an interface here.
 *
 * Register this only after FileSystemDriver::init() (and, if the target
 * device is removable, a successful mountStorageDevice() call) has run -
 * Logger::init()/reconfigure() can be called again later once storage
 * becomes available if it wasn't at platform startup.
 ***************************************************************************/
#pragma once

#include <cstdio>
#include "Logger.h"

class LoggerFile : public LoggingDriver
{
	public:
		LoggerFile() = default;
		~LoggerFile() override { shutdown(); }

		bool init(const LogConfig & config) override;
		void shutdown() override;
		void write(LogLevel level, const char * line, size_t len) override;
		const char * name() const override { return "File"; }

	private:
		FILE *         file = nullptr;
		LogFlushPolicy flushPolicy = LogFlushPolicy::Immediate;
		uint32_t       flushEveryNWrites = 16;
		uint32_t       writesSinceFlush = 0;
};
