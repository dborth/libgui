# Logging

`LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, and `LOG_ERROR` (and `LOG`, an alias for
`LOG_INFO`) are printf-style macros usable from anywhere in the app, C++ or
C, with no platform-specific code:

```cpp
LOG_INFO("mounted %d devices", count);
LOG_ERROR("smb connect failed: %s", smb->getLastError());
```

**Zero cost when disabled.** Logging is off unless you build with
`-DLOGGING_ENABLED=1` (for example `CFLAGS += -DLOGGING_ENABLED=1`). In a
default build every `LOG_*()` call expands to nothing: no format string is
placed in the binary, no arguments are evaluated (so don't rely on side
effects inside a log call), and the `Logger` isn't created or initialized at
all - `platform->getLogger()` returns `nullptr`.

**Backends.** A `Logger` fans each formatted line out to registered
`LoggingDriver` backends. Each platform registers the ones it supports:

| Backend | `LogMode` / mask bit | GameCube | Wii | Wii U |
|---|---|---|---|---|
| Console debug output | `OSReport` / `LOGGER_OSREPORT` | `SYS_Report` | `SYS_Report` | `OSReport` |
| UDP | `UDP` / `LOGGER_UDP` | - | Yes | Yes |
| Serial | `SerialGecko` / `LOGGER_SERIAL` | USB Gecko (EXI) | USB Gecko (EXI) | USB serial adapter, if the optional `usbserial` module's header is available at build time |
| File | `File` / `LOGGER_FILE` | Yes | Yes | Yes |

* **Console debug output** needs no hardware, and is what Dolphin and Cemu
  capture, so logging works in an emulator with no configuration.
  `mirrorToOSReport` (on by default) copies every line here in addition to the
  selected backend(s).
* **UDP** sends each line as a non-blocking datagram to a configurable
  address and port. It is best-effort by design - a slow or offline listener
  never stalls the app - and `includeSequenceNumber` makes dropped packets easy
  to spot.
* **USB Gecko** detects the adapter on the configured EXI channel at startup.
  If none is attached, or it is unplugged mid-run, the backend goes quiet
  instead of failing or stalling logging.
* **File** appends to a log file through ordinary stdio (never truncating an
  earlier run's log), and works the same on every platform because it goes
  through the mounted storage. By default the platform points it at
  `debug.log` at the root of the first mounted storage device (SD, then USB on
  Wii; SD on Wii U; the SD2SP2 card on GameCube). `LogFlushPolicy` picks the
  trade-off: `Immediate` flushes every line (safest against a crash or power
  loss), `EveryNWrites` flushes every N lines, and `Never` leaves it to the C
  library and shutdown (fastest).

**Configuration.** Everything is set through a `LogConfig`; nothing is
hardcoded in a backend.

| Field | Default | Meaning |
|---|---|---|
| `mode` | `File` | `OSReport`, `UDP`, `SerialGecko`, `File`, or `Multi` |
| `multiBackendMask` | `LOGGER_OSREPORT` | With `Multi`, the OR of `LOGGER_*` backends to use, e.g. `LOGGER_UDP \| LOGGER_FILE` |
| `level` | `Info` | Minimum severity: `Debug`, `Info`, `Warning`, `Error`, or `None` (log nothing) |
| `mirrorToOSReport` | `true` | Always also write to the console debug output |
| `targetIp` / `targetPort` | `192.168.1.100` / `4405` | UDP destination |
| `geckoChannel` | `1` | EXI channel for a USB Gecko (0 = memory card slot A, 1 = slot B) |
| `serialBaudRate` | `115200` | Baud rate requested from a Wii U USB serial adapter |
| `filePath` | `sd:/debug.log` | Log file path |
| `flushPolicy` / `flushEveryNWrites` | `Immediate` / `16` | File flushing behavior |
| `includeLevelTag` | `true` | Prefix lines with `[DEBUG]`, `[INFO]`, `[WARN]`, `[ERROR]` |
| `includeSequenceNumber` | `false` | Prefix lines with a running call counter |

```cpp
#if LOGGING_ENABLED
LogConfig config;
config.mode = LogMode::Multi;
config.multiBackendMask = LOGGER_UDP | LOGGER_FILE;
snprintf(config.filePath, sizeof(config.filePath), "sd:/myapp.log");
config.targetIp = "192.168.1.50";
config.level = LogLevel::Debug;
platform->getLogger()->init(config);
#endif
```

**Runtime behavior.**

* `Logger::init()` can be called again at any time - for example from a
  settings menu - and reconciles against what's already running: backends no
  longer selected are shut down, newly selected ones are started, and ones
  that stay selected are reopened against the new configuration.
  `setLevel()` changes just the minimum severity without touching any backend.
* A backend that can't start (no SD card mounted for the file log, a bad UDP
  address, no network) is skipped rather than treated as fatal, and the
  failure is reported through the console debug output.
* A line is formatted into a fixed 512-byte stack buffer - logging never
  allocates - and long lines are truncated. The severity check happens before
  any locking, so filtered-out calls are nearly free.
* Logging is thread-safe: all dispatch is serialized under one mutex, so any
  thread may log, and calls made before the logger exists or after it shuts
  down are harmless no-ops.
* At shutdown the logger is closed before the other drivers, with the file
  backend closed first so its buffered output is written out.
* `Logger::registerBackend()` accepts your own `LoggingDriver` under a unique
  single-bit `LogBackendId`; a `Logger` holds up to 8 backends.


## Stall tracing

`drivers/Trace.h` is a small diagnostic layer on top of the logger for finding
where a thread gets stuck without disturbing timing. Each thread records the
name of the phase it is in - in plain RAM, with no I/O and no locks - plus a
short ring of the most recent phase changes across all threads. Nothing is
written while things are healthy. A watchdog thread reports any thread that
has sat in one phase too long, together with the recent history and every
other thread's phase, and a phase that finished but was slow is reported when
it ends.

Like `LOG_*()`, it compiles to nothing unless `LOGGING_ENABLED` is set, so it
costs release builds no thread and no memory.

* `TRACE_THREAD(name)` - names the calling thread; call it first thing in the
  thread.
* `TRACE_AT(where)` - the calling thread is now in phase `where`; use
  `"idle"` while waiting for work so the thread is never reported as stalled.
  Pass string literals only: the pointer itself is what gets stored.
* `TRACE_LOG(...)` - logs one milestone line tagged with the thread's name.
* `TRACE_STATE_FN(fn)` - registers a function that describes application state
  (queues, flags) for stall reports. It runs on the watchdog thread with no
  locks held, so it may only read plain variables.
* `TRACE_WATCHDOG()` - starts the stall detector thread.

Reports go through `LOG_*()`, which serializes on the logger's mutex, and the
file backend holds that mutex while it writes to the SD card. If the stall you
are chasing is the SD card itself, a report may not get out - use the UDP
backend for those runs.


[Back to the README](../README.md)
