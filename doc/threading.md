# Threading

Application and library code never calls a platform's threading API; it uses
`Thread`, `Mutex`, and `Cond` from `source/drivers/`, which forward to the
platform's `ThreadDriver` (`LWP` on GameCube/Wii, coreinit on Wii U).

**`Thread`** owns at most one backend thread. `start(entry, arg, stackSize,
priority, wake)` runs `entry(arg)` on a new thread and returns `false` if a
thread is already running or the backend couldn't create one; the destructor
joins a thread that's still running. Priorities are portable
(`ThreadPriority::Idle`, `Low`, `Normal`, `High`, `TimeCritical`) and are
mapped onto each platform's own priority range. `join()`, `suspend()`,
`resume()`, and `isSuspended()` do what they say. `cancel()` is a best-effort
last resort: not every platform can terminate a running thread, so prefer
letting the thread exit on its own.

**Cooperative stop.** A thread's entry function polls `stopRequested()` in its
loop condition. `requestStop()` sets that flag (safe to call from any thread)
and invokes the optional `wake` callback given to `start()`, which is how a
thread parked on a condition variable or a long wait gets knocked loose.

```cpp
static Thread worker;
static ThreadSync sync;                       // a mutex plus two condition variables

static void wakeWorker() { MutexLock g(sync.mutex); sync.workCond.signal(); }

static void * workerMain(void *)
{
    while(!worker.stopRequested())
    {
        MutexLock g(sync.mutex);
        while(!haveWork && !worker.stopRequested())
            sync.workCond.wait(sync.mutex);   // unlocks while waiting, relocks on return
        // ... do the work ...
    }
    return nullptr;
}

worker.start(workerMain, nullptr, 16384, ThreadPriority::Low, wakeWorker);
```

**`Thread::JoinAll()`** is the app-exit safety net. Every thread that starts
successfully is registered in a process-wide list; `JoinAll()` requests every
outstanding thread to stop (using each thread's own wake callback) and joins
them all, so once it returns nothing can still be touching driver state.
Call it once, late in shutdown, right before `platform->requestExit()`.

**`Mutex` / `MutexLock`.** `Mutex` is a plain mutual-exclusion lock - treat it
as non-recursive. `MutexLock` is the RAII guard that locks on construction and
unlocks on destruction, so an early return can't leave a mutex held.

**`Cond`.** `wait(mutex)` atomically unlocks the mutex and blocks until
signalled, then relocks it before returning. `signal()` wakes *every* waiter
on every backend (there is no single-waiter wake), so always re-check your
condition in a loop.

**`ThreadSync`** bundles a `Mutex` with two `Cond`s (`workCond`, `idleCond`)
for the common handshake between a background worker and its caller: one side
sets a flag under the mutex and signals `workCond` to wake the other; the
other clears the flag and signals `idleCond` when idle. The demo's storage
device-checking thread uses it: the thread runs while the menus are up, and
`HaltDeviceCheckingThread()` parks it (blocking until it confirms it is idle)
when the menus are left, before it is resumed or joined.

**`ThreadId::current()`** returns a comparable identifier for the calling
thread - including the app's original main thread, which was never started
through `Thread` - for code that only needs to ask "am I on the GUI thread?".

**`SystemTime`** (`Time.h`) is a monotonic clock that hides `gettime()` and
`OSGetSystemTime()`: `SystemTime::now()` returns an opaque `Ticks` value, and
`diffSecs()`, `diffMillisecs()`, and `diffMicrosecs()` convert the interval
between two samples.


[Back to the README](../README.md)
