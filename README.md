## libgui
https://github.com/dborth/libgui (Under GPL License)

[API documentation](https://dborth.github.io/libgui/) ·
[Changelog](CHANGELOG.md) ·
[Latest build](https://github.com/dborth/libgui/releases/tag/Pre-release)

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
  [Storage](doc/storage.md).
* **Network shares (SMB)** through libsmb2 on all three consoles,
  mounted as an ordinary `smb:/` device so the rest of the app reads and
  writes it with plain POSIX calls. See [Network shares](doc/network-shares.md).
* **Threading primitives** (`Thread`, `Mutex`, `Cond`) with a
  cooperative stop protocol and an app-exit safety net, so app and
  library code can run background work without touching a platform's raw
  threading API. See [Threading](doc/threading.md).
* **A multi-backend logging framework** with severity levels, runtime
  reconfiguration, and output to the console debug channel, UDP, a USB
  Gecko / USB serial adapter, or a log file - compiled out entirely when
  disabled. See [Logging](doc/logging.md).
* **Consistent input across every controller** - GameCube pad, Wiimote
  (IR pointer, Nunchuk, Classic Controller), Wii U Pro Controller, and
  the Wii U GamePad (sticks, buttons, and touch) all arrive as the same
  per-channel input snapshot, with adaptive IR pointer smoothing and
  menu rumble feedback. See [Input](doc/input.md).
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

The `Logger` (see [Logging](doc/logging.md)) is owned by `Platform` alongside
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
`Thread::JoinAll()` under [Threading](doc/threading.md). The demo's `main()`
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
├── doc/                       # topic guides (also included in the doxygen docs)
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


### Guides

Topic guides, each readable here on GitHub and included in the
[API documentation](https://dborth.github.io/libgui/):

* [Storage](doc/storage.md) - SD, USB, DVD and network devices, hot-plug and mounting
* [Network shares (SMB)](doc/network-shares.md) - Connecting to an SMB share as an ordinary `smb:/` device
* [Input](doc/input.md) - Controllers, the input snapshot, rumble and pointer smoothing
* [Audio](doc/audio.md) - One-shot voices, the OGG stream and volume categories
* [Threading](doc/threading.md) - Thread, Mutex, Cond and the cooperative stop protocol
* [Logging](doc/logging.md) - The multi-backend logger and its configuration


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
(see [Logging](doc/logging.md)).

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

The API reference is generated with doxygen from the source and this README,
and published at https://dborth.github.io/libgui/

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

**[2.00 - September 23, 2026]** adds full Wii U support (GX2 video, AX audio, VPAD/
KPAD input, FSA-based SD storage, USB storage with libdvm), a GameCube build,
network shares, multi-drive USB with full hot-plug, and a logging framework -
all made possible by refactoring the library behind a proper hardware
abstraction layer. See [Architecture](#architecture) above and the changelog
for the complete list of changes.
