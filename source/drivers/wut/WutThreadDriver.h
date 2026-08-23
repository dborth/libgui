/****************************************************************************
 * libgui - drivers/wut
 * Daryl Borth 2026
 * WutThreadDriver.h
 ***************************************************************************/
#pragma once
#include "../ThreadDriver.h"

class WutThreadDriver : public ThreadDriver
{
	public:
		void init() override;
		void shutdown() override;

		bool createThread(ThreadEntry entry, void * arg, uint32_t stackSize, int priority, void ** outHandle) override;
		void joinThread(void * thread) override;
		void cancelThread(void * thread) override;
		void suspendThread(void * thread) override;
		void resumeThread(void * thread) override;
		bool isThreadSuspended(void * thread) override;

		void * createMutex() override;
		void destroyMutex(void * mutex) override;
		void lockMutex(void * mutex) override;
		void unlockMutex(void * mutex) override;

		void sleepMilliseconds(uint32_t ms) override;
};
