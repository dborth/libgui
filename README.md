## libgui
https://github.com/dborth/libgui (Under GPL License)

libgui is a GUI library for the **GameCube, Wii, and Wii U** created to
help structure the design of a complicated GUI interface, and to enable
an author to create a sophisticated, feature-rich GUI. It was originally
conceived and written after I started to design a GUI for Snes9x GX, and
found libwiisprite and GRRLIB inadequate for the purpose.

libgui powers the GUI of several Nintendo homebrew emulators, including
Snes9x GX, FCE Ultra GX, and Visual Boy Advance GX. It is a single,
shared codebase that targets three different consoles from one source
tree - GameCube and Wii share one driver set, and Wii U runs on modern
GX2/GPU hardware through an entirely separate one, but the `GuiElement`/
`GuiWindow`/`GuiButton`/etc. class hierarchy your app is built from is
identical on all three. It was designed to be flexible and is easy to
modify - don't be afraid to change the way it works or expand it to suit
your GUI's purposes! If you do, and you think your changes might benefit
others, please share them so they might be added to the project!

### Features

* **Three platforms, one codebase.** GameCube, Wii, and Wii U builds all
  compile from the same `source/libgui` and application code - no
  `#ifdef`s in your GUI logic, no per-platform art or asset variants.
* **A real hardware abstraction layer.** Every platform touchpoint
  (video, audio, input, storage, threading, logging) sits behind a small
  set of abstract driver interfaces (see [Architecture](#architecture)
  below), so the core library never includes a platform header.
* **A full widget set.** Buttons, images, text, an on-screen keyboard,
  paged option and file browsers, windows/containers with focus and
  selection navigation, and a trigger-based input-condition system that
  auto-resolves "Accept"/"Cancel" semantics across button layouts.
* **FreeType2-based text rendering** with glyph shaping/caching per
  pixel size, UTF-8 string tables loaded from a compact binary `.lang`
  blob, and bitmask text styling (justification, alignment, underline,
  strike).
* **PNG image decoding** straight to a platform-native texture, with a
  caller-supplied scratch buffer so decode memory is sized once by the
  app rather than allocated per image.
* **Audio** via fixed one-shot PCM voices plus a single background OGG
  stream, decoded through an integer (Tremor) decoder shared by every
  platform, with separate global music and sound-effect volumes.
* **Storage** - SD, USB, DVD, and network shares behind one
  `FileSystemDriver`, with full hot-plug detection: up to three
  simultaneous USB drives on Wii and Wii U, SD Gecko cards on GameCube,
  and FAT, exFAT, and NTFS volumes on Wii U USB. See
  [Storage](#storage).
* **Network shares (SMB)** through libsmb2 on all three consoles,
  mounted as an ordinary `smb:/` device so the rest of the app reads and
  writes it with plain POSIX calls. See [Network shares](#network-shares-smb).
* **Threading primitives** (`Thread`, `Mutex`, `Cond`) with a
  cooperative stop protocol and an app-exit safety net, so app and
  library code can run background work without touching a platform's raw
  threading API. See [Threading](#threading).
* **A multi-backend logging framework** with severity levels, runtime
  reconfiguration, and output to the console debug channel, UDP, a USB
  Gecko / USB serial adapter, or a log file - compiled out entirely when
  disabled. See [Logging](#logging).
* **Consistent input across every controller** - GameCube pad, Wiimote
  (IR pointer, Nunchuk, Classic Controller), Wii U Pro Controller, and
  the Wii U GamePad (sticks, buttons, and touch) all arrive as the same
  per-channel input snapshot, with adaptive IR pointer smoothing and
  menu rumble feedback. See [Input](#input).
* **A proper application lifecycle** - a single `Platform` object
  reports running/paused/exiting state and hardware events (power and
  reset buttons, the Wii U HOME menu and close requests) the same way on
  every console. See [Application lifecycle](#application-lifecycle).


### Supported Platforms

| Platform | Toolchain | Video | Audio | Input | Storage |
|---|---|---|---|---|---|
| GameCube | devkitPPC + libogc2 | GX | AESND | PAD | SD Gecko (slot A/B), SD2SP2, GC Loader, DVD, SMB (Broadband Adapter) |
| Wii | devkitPPC + libogc2 | GX | AESND | WPAD (Wiimote/Nunchuk/Classic/Pro), plus Wii U GamePad support | SD, up to 3 USB, DVD, SMB |
| Wii U | devkitPPC + wut + libwhb | GX2 | AX (sndcore2) | VPAD (GamePad), KPAD/WPAD (Wiimote/Nunchuk/Classic/Pro) | SD, up to 3 USB (FAT/exFAT/NTFS), SMB |


### Architecture

libgui's core (`source/libgui/`) never includes `<gccore.h>`, `<gx2/*.h>`,
`<wpad/wpad.h>`, or any other platform header. Everything platform-specific
sits behind a small set of abstract driver interfaces in `source/drivers/`,
composed by a single `Platform` object:

```cpp
class Platform {
public:
    virtual void init(int width, int height) = 0;   // constructs + initializes every driver
    virtual void requestExit() = 0;                 // shut down and leave the app; does not return

    virtual AudioDriver* getAudio() = 0;
    virtual VideoDriver* getVideo() = 0;
    virtual InputDriver* getInput() = 0;
    virtual FileSystemDriver* getFileSystem() = 0;
    virtual ThreadDriver* getThread() = 0;
    virtual Logger* getLogger() = 0;                // nullptr when logging is compiled out

    virtual SystemEvent getSystemEvent() = 0;       // None / ShutdownRequested / ResetRequested
    virtual Status getStatus() const = 0;           // Running / Paused / Exiting
    virtual void triggerExit() = 0;                 // move to Status::Exiting
    bool shouldExit();                              // Exiting, or a shutdown event was reported
};
extern Platform* platform; // single global instance, assigned by the app
```

There are three concrete `Platform` implementations - `GameCubePlatform`
and `WiiPlatform` (sharing GameCube/Wii's driver set), and `WutPlatform`
(Wii U) - selected once at startup based on the build:

```cpp
#ifdef __WIIU__
static WutPlatform platformInstance;
#elif defined(HW_DOL)
static GameCubePlatform platformInstance;
#else
static WiiPlatform platformInstance;
#endif
Platform* platform = &platformInstance;
```

That's the only platform branch an app needs to write. Everything else -
`GuiElement::update()`, `GuiImage`, `GuiText`, the file browser, and so
on - talks only to the interfaces below.

#### The driver interfaces

| Interface | Responsibility | GameCube/Wii (`drivers/ogc`) | Wii U (`drivers/wut`) |
|---|---|---|---|
| `VideoDriver` | Frame lifecycle, screen size/refresh rate/delta time; hands out an `ImageRenderer` (textured quads) and `GlyphRenderer` (glyph quads + solid rectangles) | `OgcVideoDriver` - raw GX, double-buffered XFB | `WutVideoDriver` - GX2 + libwhb's `WHBGfx*` helpers, submitting the same UI to both the TV and GamePad every frame, backed by two small custom GX2 shaders (`Texture2DShader`, `ColorShader`) |
| `AudioDriver` | Fixed one-shot PCM voices plus one background OGG stream | `OgcAudioDriver` - AESND | `WutAudioDriver` - AX (sndcore2), 16 voice slots plus a ring-buffered stereo stream path, mixed to both the TV and the GamePad |
| `InputDriver` | Polls hardware and produces a per-channel `InputPadData` snapshot each frame, consumed by a persistent `InputController` per channel; controls rumble and Wiimote orientation | `OgcInputDriver` - PAD (GameCube) / WPAD (Wiimote, Nunchuk, Classic, Wii U Pro Controller), plus Wii U GamePad via the vendored `libwiidrc` | `WutInputDriver` - VPAD (GamePad stick/buttons/touch) and KPAD/WPAD for up to 4 Wiimotes/Nunchuks/Classic/Pro Controllers |
| `FileSystemDriver` | Storage device enumeration, mount/poll, hot-plug detection, and the network-share driver (`StorageDevice`, `MountResult`, `SmbDriver`) | `WiiFileSystemDriver` / `GameCubeFileSystemDriver`, with `OgcSmbDriver` | `WutFileSystemDriver`, with `WutSmbDriver` |
| `ThreadDriver` | Raw thread/mutex/condition-variable primitives | `OgcThreadDriver` - libogc's LWP | `WutThreadDriver` - coreinit's `OSThread`/`OSMutex`/`OSCondition` |

The `Logger` (see [Logging](#logging)) is owned by `Platform` alongside
these drivers and fans each log line out to a set of `LoggingDriver`
backends.

The UI is laid out on a fixed design canvas (640x480 in the demo) that the
video driver maps onto the output, so widget positions and sizes are
expressed in the same units on every platform.

#### Application lifecycle

`Platform` gives an app one consistent way to find out that it should stop,
pause, or reset, regardless of console:

* `getStatus()` returns `Status::Running`, `Status::Paused`, or
  `Status::Exiting`. On Wii U the status is `Paused` whenever the app is
  not in the foreground (for example while the HOME menu overlay is up);
  the video and audio drivers skip their work while paused, and the app
  keeps running. Once the OS asks the app to close, or the app calls
  `triggerExit()`, the status becomes `Exiting` and stays there.
* `getSystemEvent()` reports hardware/OS events: `ShutdownRequested`
  (the power button on the console or a Wiimote on Wii; a close/standby
  request from Cafe OS on Wii U) and `ResetRequested` (the reset button,
  Wii only - soft-reset and keep going). GameCube has no hardware event
  source and always reports `None`.
* `shouldExit()` is true once either `Status::Exiting` is set or a
  shutdown event has been reported, so a main loop only needs to check
  one thing.
* `requestExit()` shuts every driver down and then ends the app the way
  the platform expects: Wii U asks Cafe OS to return to the system menu
  (or loader) and waits for the app to leave the foreground before
  exiting; Wii performs an IOS power-off when a power button started the
  exit, and otherwise exits normally; GameCube exits. It does not return.

Background threads must be stopped before `requestExit()` - see
`Thread::JoinAll()` under [Threading](#threading). The demo's `main()`
shows the intended order:

```cpp
platform->init(640, 480);
// ... set up fonts, audio, the device-checking thread, run the menus ...
Thread::JoinAll();
platform->requestExit();
```

#### Repository layout

```text
libgui/
├── Makefile[.wii|.gc|.wiiu]   # per-platform build, dispatched by the top-level Makefile
├── data/                      # images/fonts/sounds/lang - one shared asset set, bin2o'd for all 3 platforms
├── meta/                      # Wii U .wuhb icon/splash assets
└── source/
    ├── demo.cpp / demo.h      # entry point / app template
    ├── menu.cpp / menu.h      # template menu screens (not part of the library itself)
    ├── filebrowser.cpp/.h     # storage file browser built on GuiFileBrowser
    │
    ├── drivers/               # ---- Platform Abstraction Layer ----
    │   ├── Platform.h, VideoDriver.h, AudioDriver.h, InputDriver.h,
    │   │   FileSystemDriver.h, ThreadDriver.h   # the abstract interfaces
    │   ├── SmbDriver.h                          # network-share interface (owned by FileSystemDriver)
    │   ├── Logger.h/.cpp, LoggerFile.h/.cpp     # logging framework + the generic file backend
    │   ├── Thread.h/.cpp, Mutex.h/.cpp, Cond.h/.cpp   # RAII wrappers app/core code uses directly
    │   ├── InputController.h/.cpp   # per-channel logical controller (repeat/orientation/scroll logic)
    │   ├── InputData.h, Time.h, OneEuroFilter.h   # input snapshot, monotonic timing, pointer smoothing filter
    │   │
    │   ├── ogc/                # GameCube + Wii implementation (video, audio, input, threads,
    │   │   │                   #   SMB, and the SysReport/UDP/USB Gecko logging backends)
    │   │   ├── wii/            # WiiPlatform, WiiFileSystemDriver, WiiUsbMulti, libwiidrc
    │   │   └── gamecube/       # GameCubePlatform, GameCubeFileSystemDriver
    │   └── wut/                # Wii U implementation (video, audio, input, threads, storage, SMB,
    │       │                   #   USB hot-plug probe, libdvm glue, and the OSReport/UDP/USB serial
    │       │                   #   logging backends)
    │       └── shaders/        # the GX2 shader plumbing
    │
    └── libgui/                 # ---- Platform-agnostic core UI library ----
        ├── Gui.h                       # umbrella header; alignment/state/scroll enums
        ├── GuiElement, GuiWindow, GuiButton, GuiImage, GuiImageData,
        │   GuiText, GuiTextRenderer, GuiTextTranslator, GuiTrigger
        ├── GuiSound, GuiSoundOggPlayer
        ├── GuiFileBrowser, GuiOptionBrowser, GuiKeyboard
        └── ...
```

`menu.cpp`/`menu.h` and `filebrowser.cpp`/`.h` build the actual template
screens (settings, storage file browser, network share) on top of the core
library and are explicitly *not* part of the library itself - they're a
recommended pattern to build from, not a hard dependency.


### Storage

Every storage device is described by a `StorageDevice` (id, display name,
mount prefix such as `sd:/`, volume label, whether it is removable, whether
it is mounted automatically at startup, whether it is always shown in a
device list, and optional capacity/free-space/block-size/read-only
telemetry). The app enumerates devices, mounts them, and then builds paths
with `getMountPath()` / `getPath()`; after that, all file access is ordinary
POSIX I/O. `getValidLoadDevices()` and `getValidSaveDevices()` list which
devices make sense as a load or save location on the current platform, and
`FindFirstMountedPath()` picks the first mounted device out of a priority
list.

Each device type has exactly one mount. A mount attempt returns
`MountResult::Success`, `DeviceNotFound`, or `MountFailed` (something is
present but can't be mounted, for example an unsupported format), and
`mountResultMessage()` supplies a short string suitable for showing to the
user.

| Device | GameCube | Wii | Wii U |
|---|---|---|---|
| SD card | SD Gecko in slot A or B, SD2SP2 in serial port 2 (FAT) | SD slot (FAT) | SD slot, mounted natively by Cafe OS |
| USB storage | - | Up to 3 simultaneous drives (FAT), via IOS | Up to 3 simultaneous drives (FAT, exFAT, NTFS), via libmocha + libdvm |
| GC Loader | Mounted as a FAT device | - | - |
| DVD | Data DVD (ISO9660) | Data DVD (ISO9660) | - |
| Network share | SMB | SMB | SMB |

**Multiple USB drives.** Wii and Wii U each expose three USB storage slots
(`DEVICE_USB`, `DEVICE_USB2`, `DEVICE_USB3`). A newly attached drive takes
the first free slot in attach order; slots are not tied to a physical port.
On Wii this is handled by `WiiUsbMulti`, which opens each attached mass
storage device through IOS independently and presents each one as its own
disc interface. On Wii U, each slot is opened through libmocha's raw disc
interface and mounted through libdvm, which recognizes FAT, exFAT, and NTFS.
USB on Wii U requires the Mocha CFW component (available under Aroma);
without it the USB slots are simply absent while the SD card and network
share continue to work.

**Hot-plug.** Devices can be inserted and removed while the app runs.
`pollStorageDevices()` is meant to be called about once a second from a
background thread (the demo's device-checking thread does exactly this at
low priority), and reports which mounted devices disappeared and whether the
device list changed shape so the UI can refresh.

* *Wii* - SD, all three USB slots, and the DVD drive are checked each cycle.
* *GameCube* - SD Gecko slots A and B and SD2SP2 are checked each cycle with
  an EXI presence probe. The GC Loader and the DVD drive are only touched
  when the user mounts them.
* *Wii U* - USB attach/detach is detected by scanning the USB stack; 
  a mounted USB volume is verified each cycle with a real, uncached sector
  read, so a drive that was pulled is noticed even if the filesystem cache
  could still answer from memory. A slot holding a device that won't mount
  gets a few quick retries and is then probed on a slow backoff schedule,
  which resets immediately when the USB stack reports a hardware change.

`isDevicePresent()` is a cheap, cached check that never mounts anything, so
a UI can list only devices that are actually there (devices flagged
`alwaysListed`, such as the network share, are shown regardless). On a mount
failure or a read/write error, `invalidateStorageDevice()` marks a device to
be re-mounted fresh on its next use.


### Network shares (SMB)

`FileSystemDriver::getSmb()` returns the platform's `SmbDriver`, which
mounts a single SMB share as the `DEVICE_SMB` device at `smb:/`. GameCube and
Wii use `OgcSmbDriver`; Wii U uses `WutSmbDriver`. Both are built on libsmb2
and register an `smb:/` devoptab, so once connected the share behaves like
any other mounted device.

```cpp
SmbShareInfo share = {};                 // host, share, user, password (empty user = guest)
snprintf(share.host, sizeof(share.host), "192.168.0.100");
snprintf(share.share, sizeof(share.share), "Files");

SmbDriver * smb = platform->getFileSystem()->getSmb();
SmbConnectResult result = smb->connect(share);   // brings the network up first if needed

if(result != SmbConnectResult::Success)
    showError(smb->connectResultMessage(result), smb->getLastError());
else
    FILE * f = fopen("smb:/roms/game.sfc", "rb");
```

`connect()` brings the console's network connection up if it isn't already
(GameCube/Wii via libogc's network stack; Wii U via the system's network
account service), and is a no-op if already connected to the same share.
Results are `Success`, `InvalidSettings` (no host or share), `NetworkUnavailable`,
or `ConnectFailed` (the network is up but the server, share, or credentials
were rejected); `getLastError()` adds libsmb2's own detail, such as an
access-denied or host-resolution message. `disconnect()` unmounts the share.
The demo's Network Share screen shows the whole flow.


### Input

Every driver's `update()` builds one platform-independent `InputPadData` per
channel per frame: a unified button bitmask (pressed / held / released) that
is a superset of every supported controller, plus per-hardware-profile copies
(GameCube pad, Wiimote, Nunchuk, Classic Controller, Wii U Pro Controller,
Wii U GamePad) so a controller made of several parts - for example a Wiimote
with a Nunchuk - reports each part separately before they are merged. It also
carries the pointer position and angle, whether the pointer is valid, and
whether it came from touch. A persistent `InputController` per channel turns
that into what widgets consume: analog deadzone, directional repeat and scroll
delay timing, and Wiimote orientation, so none of that logic is duplicated per
platform.

* **Pointer smoothing.** The Wii U driver filters the Wiimote IR pointer with
  a One Euro Filter (`OneEuroFilter.h`), which smooths heavily while the
  pointer is nearly still and backs off as it moves faster - a steady cursor
  at rest without lag on fast sweeps.
* **GamePad touch.** Touch positions are scaled onto the design canvas and
  delivered as pointer input; touch-down, hold, and release act as A-button
  press, hold, and release, so touch-driven widgets behave like a pointer
  click.
* **HOME button.** On Wii U the HOME button is delivered to the app as an
  ordinary `INPUT_BTN_HOME` press, and the system HOME menu overlay doesn't
  open on its own, so the app decides what HOME does.
  `WutInputDriver::openHomeButtonOverlay()` opens the overlay on demand (the
  demo's "Wii U Overlay" button).
* **Wiimote orientation.** `InputDriver::setWiimoteOrientation()` selects
  `WIIMOTE_ORIENTATION_VERTICAL` or `WIIMOTE_ORIENTATION_HORIZONTAL`; the
  semantic Accept/Cancel triggers resolve to A/B (vertical) or 2/1
  (horizontal) accordingly.
* **Rumble.** `setRumbleEnabled()` turns rumble on or off globally. Menu
  hover feedback is a short (about 33 ms) tick followed by an enforced quiet
  gap (about 100 ms), so quickly moving across buttons doesn't produce a
  continuous buzz. On the Wii U GamePad the tick uses a reduced-amplitude
  pattern.
* **Wii U GamePad on Wii.** The vendored `libwiidrc` lets a Wii app read a Wii
  U GamePad as an additional controller.


### Audio

`AudioDriver` provides fixed one-shot PCM voices and exactly one streamed OGG
track at a time. `GuiSound` wraps both: a looping sound is treated as music
and takes the single stream, and a non-looping sound is a sound effect on a
voice. `GuiSound::setDefaultVolume(VOLUME_TYPE::MUSIC, ...)` and
`VOLUME_TYPE::SFX` set independent global volumes (0-100) for the two
categories; changing the music volume takes effect immediately on the track
that is playing. OGG decoding is done by the platform-independent
`GuiSoundOggPlayer` (Tremor) on a background thread, on every platform. On
Wii U, sound is mixed to both the TV and the GamePad.


### Threading

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


### Logging

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


### Building

You'll need [devkitPro](https://devkitpro.org/) with `devkitPPC`
installed, plus the platform-specific pieces below (available via
`dkp-pacman`/the devkitPro pacman repos unless noted):

* **All platforms**: the `ppc` portlibs for `freetype`, `libpng`, `zlib`, and
  `libvorbisidec`/`libogg` (Tremor), and `libsmb2` (see below).
* **GameCube / Wii**: `libogc2`, which provides the FAT, ISO9660, DVD, network,
  and controller libraries the Makefiles link against.
* **Wii U**: `wut` and `libwhb`, plus `libmocha` and `libdvm`.
* **Libraries built from source**: `libsmb2`, `libmocha`, and `libdvm` come from
  the forks at `github.com/dborth/`. `libsmb2` is built once per platform
  (`make -f Makefile.platform wii_install`, `gc_install`, or `wiiu_install`)
  and needs the Ninja build tool; `libdvm` is cloned with its submodules.
  The workflow in `.github/workflows/build.yml` is a working reference.

Then, from the repository root:

```sh
make -f Makefile.wii    # Wii  -> .dol
make -f Makefile.gc     # GameCube -> .dol
make -f Makefile.wiiu   # Wii U -> .rpx / .wuhb
make                    # builds all three
```

Each Makefile builds the same `source`, `source/drivers`, and
`source/libgui` trees, adding only the directories for its platform:
`drivers/ogc` plus `drivers/ogc/wii` or `drivers/ogc/gamecube` for Wii and
GameCube, and `drivers/wut` plus `drivers/wut/shaders` for Wii U.

To enable logging, add `-DLOGGING_ENABLED=1` to the build's compiler flags
(see [Logging](#logging)).

The GitHub Actions workflow builds all three platforms on every push, deploys
the doxygen documentation to GitHub Pages, and keeps a rolling pre-release
with the latest `.dol`, `.rpx`, and `.wuhb` builds.


### Quickstart

Start from the supplied template example (`source/demo.cpp`). It shows the
full startup and shutdown sequence: create the platform, size the image
decode scratch buffer, create the font renderer and string table, start audio
and the storage device-checking thread, run the menus, then join background
threads and exit. For more advanced uses, see the source code for Snes9x GX,
FCE Ultra GX, and Visual Boy Advance GX - all of which build their menu system
on top of libgui.


### Contact

If you have any suggestions for the library or documentation, or want to
contribute, please visit the libgui website:
https://github.com/dborth/libgui


### Documentation

See the included doxygen documentation - http://dborth.github.io/libgui/

This covers both the core GUI classes (`source/libgui`) and the platform
driver layer (`source/drivers`), including the GameCube/Wii (`ogc`) and
Wii U (`wut`) driver implementations.


### Credits

This library was wholly designed and written by Tantric. Thanks
also to the authors of GRRLIB and libwiisprite for laying the foundations.
Thanks to mvit for the artwork and Peter de Man for the music used in the
template. Thanks to FIX94 for `libwiidrc`, letting a Wii app use a Wii U
GamePad.


### Update History

See [CHANGELOG.md](CHANGELOG.md) for the full, version-by-version history.

**[2.0 - Unreleased]** adds full Wii U support (GX2 video, AX audio, VPAD/
KPAD input, FSA-based SD storage, USB storage with libdvm), a GameCube build,
network shares, multi-drive USB with full hot-plug, and a logging framework -
all made possible by refactoring the library behind a proper hardware
abstraction layer. See [Architecture](#architecture) above and the changelog
for the complete list of changes.
