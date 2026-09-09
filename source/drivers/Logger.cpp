/****************************************************************************
 * Platform Abstraction Layer
 * Daryl Borth 2026
 * Logger.cpp
 *
 * Generic - every platform-specific byte in or out lives behind an
 * LoggingDriver implementation in drivers/ogc or drivers/wut. This file
 * never includes a platform SDK header.
 ***************************************************************************/
#include "Logger.h"
#include "Platform.h"

#include <cstdio>
#include <cstring>

Logger::Logger()
{
}

Logger::~Logger()
{
	shutdown();

	for (int i = 0; i < slotCount; i++)
		delete slots[i].backend;
}

bool Logger::registerBackend(LogBackendId id, LoggingDriver * backend)
{
	// Must be exactly one bit - a caller passing an OR of flags here
	// almost certainly meant multiBackendMask, not this.
	if (backend == nullptr || id == LOGGER_NONE || (id & (id - 1)) != 0)
	{
		delete backend;
		return false;
	}

	for (int i = 0; i < slotCount; i++)
	{
		if (slots[i].id == id)
		{
			delete backend;
			return false;
		}
	}

	if (slotCount >= MAX_BACKENDS)
	{
		delete backend;
		return false;
	}

	slots[slotCount].id = id;
	slots[slotCount].backend = backend;
	slots[slotCount].active = false;
	slotCount++;
	return true;
}

uint32_t Logger::resolveActiveMask() const
{
	uint32_t mask;

	switch (config.mode)
	{
		case LogMode::OSReport:    mask = LOGGER_OSREPORT; break;
		case LogMode::UDP:         mask = LOGGER_UDP;      break;
		case LogMode::SerialGecko: mask = LOGGER_SERIAL;    break;
		case LogMode::SDFile:      mask = LOGGER_SD;   break;
		case LogMode::Multi:       mask = config.multiBackendMask; break;
		default:                   mask = LOGGER_NONE;    break;
	}

	if (config.mirrorToOSReport)
		mask |= LOGGER_OSREPORT;

	return mask;
}

void Logger::init(const LogConfig & newConfig)
{
	MutexLock guard(lock);

	config = newConfig;
	uint32_t activeMask = resolveActiveMask();

	for (int i = 0; i < slotCount; i++)
	{
		bool wantActive = (slots[i].id & activeMask) != 0;

		if (wantActive && !slots[i].active)
		{
			slots[i].active = slots[i].backend->init(config);
		}
		else if (!wantActive && slots[i].active)
		{
			slots[i].backend->shutdown();
			slots[i].active = false;
		}
		else if (wantActive && slots[i].active)
		{
			// Already active from a previous init() - reopen against
			// the new config (target IP/path/etc may have changed).
			slots[i].backend->shutdown();
			slots[i].active = slots[i].backend->init(config);
		}
	}

	initialized = true;
}

void Logger::shutdown()
{
	MutexLock guard(lock);

	for (int i = 0; i < slotCount; i++)
	{
		if (slots[i].active)
		{
			slots[i].backend->shutdown();
			slots[i].active = false;
		}
	}

	initialized = false;
}

void Logger::setLevel(LogLevel level)
{
	MutexLock guard(lock);
	config.level = level;
}

static const char * LevelTag(LogLevel level)
{
	switch (level)
	{
		case LogLevel::Debug:   return "[DEBUG] ";
		case LogLevel::Info:    return "[INFO]  ";
		case LogLevel::Warning: return "[WARN]  ";
		case LogLevel::Error:   return "[ERROR] ";
		default:                return "";
	}
}

void Logger::log(LogLevel level, const char * fmt, va_list args)
{
	if (!initialized || level == LogLevel::None)
		return;

	// Cheap early-out before taking the lock or touching the stack
	// buffer at all - the overwhelmingly common case once a build has
	// settled on a level (eg. Info in release, Debug only when actively
	// chasing a bug).
	if (level < config.level)
		return;

	MutexLock guard(lock);

	// Re-check under the lock: another thread may have raised the level
	// (or shut Logger down) between the check above and taking it.
	if (!initialized || level < config.level)
		return;

	// Fixed stack buffer only - no malloc/new anywhere in this path, so
	// logging from a time-sensitive loop (audio callback, emulator core)
	// has bounded, predictable cost and can't fail from heap exhaustion.
	char line[512];
	size_t offset = 0;

	if (config.includeSequenceNumber)
	{
		int n = snprintf(line + offset, sizeof(line) - offset, "%06u ", (unsigned)(++sequence));
		if (n > 0)
			offset += (size_t)n < (sizeof(line) - offset) ? (size_t)n : (sizeof(line) - offset - 1);
	}

	if (config.includeLevelTag)
	{
		size_t remaining = sizeof(line) - offset;
		size_t tagLen = strlen(LevelTag(level));
		if (tagLen < remaining)
		{
			memcpy(line + offset, LevelTag(level), tagLen);
			offset += tagLen;
		}
	}

	if (offset < sizeof(line) - 1)
	{
		int n = vsnprintf(line + offset, sizeof(line) - offset, fmt, args);
		if (n > 0)
			offset += (size_t)n < (sizeof(line) - offset) ? (size_t)n : (sizeof(line) - offset - 1);
	}

	// Ensure a trailing newline so line-oriented backends (UDP/file/
	// serial) don't run consecutive log lines together - but don't
	// overflow the buffer doing it.
	if (offset == 0 || line[offset - 1] != '\n')
	{
		if (offset < sizeof(line) - 1)
			line[offset++] = '\n';
		else
			line[offset - 1] = '\n';
	}
	line[offset] = '\0';

	uint32_t activeMask = resolveActiveMask();

	for (int i = 0; i < slotCount; i++)
	{
		if (slots[i].active && (slots[i].id & activeMask) != 0)
			slots[i].backend->write(level, line, offset);
	}
}

void LogPrintf(LogLevel level, const char * fmt, ...)
{
	if (!platform || !platform->getLogger())
		return;

	va_list args;
	va_start(args, fmt);
	platform->getLogger()->log(level, fmt, args);
	va_end(args);
}
