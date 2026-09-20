## libgui
https://github.com/dborth/libgui (Under GPL License)

libgui is a GUI library for the **GameCube, Wii, and Wii U** created to
help structure the design of a complicated GUI interface, and to enable
an author to create a sophisticated, feature-rich GUI. It was originally
conceived and written after I started to design a GUI for Snes9x GX, and
found libwiisprite and GRRLIB inadequate for the purpose.

libgui powers the GUI of several Nintendo homebrew emulators, including
Snes9x GX, FCE Ultra GX, and Visual Boy Advance GX. What started as a
Wii-only library has grown into a single, shared codebase that targets
three different consoles from one source tree - GameCube and Wii share
one driver set outright, and Wii U runs on modern GX2/GPU hardware
through an entirely separate one, but the `GuiElement`/`GuiWindow`/
`GuiButton`/etc. class hierarchy your app is built from is identical on
all three. It was designed to be flexible and is easy to modify - don't
be afraid to change the way it works or expand it to suit your GUI's
purposes! If you do, and you think your changes might benefit others,
please share them so they might be added to the project!

### Features

* **Three platforms, one codebase.** GameCube, Wii, and Wii U builds all
  compile from the same `source/libgui` and application code - no
  `#ifdef`s in your GUI logic, no per-platform art or asset variants.
* **A real hardware abstraction layer.** Every platform touchpoint
  (video, audio, input, storage, threading) sits behind a small set of
  abstract driver interfaces (see [Architecture](#architecture) below),
  so the core library never includes a platform header.
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
  platform.
* **Storage device enumeration** - SD, USB, and (on GameCube/Wii) DVD,
  with hot-plug polling designed to run on its own thread.
* **Cooperative threading primitives** (`Thread`/`Mutex`/`Cond`) so app
  and library code can spin up background work (eg. the storage
  device-checking thread) without touching a platform's raw threading
  API directly.


### Supported Platforms

| Platform | Toolchain | Video | Audio | Input |
|---|---|---|---|---|
| GameCube | devkitPPC + libogc2 | GX | AESND | PAD |
| Wii | devkitPPC + libogc2 | GX | AESND | WPAD (Wiimote/Nunchuk/Classic/Pro), plus Wii U GamePad support |
| Wii U | devkitPPC + wut + libwhb | GX2 | AX (sndcore2) | VPAD (GamePad), KPAD/WPAD (Wiimote/Nunchuk/Classic/Pro) |


### Architecture

libgui's core (`source/libgui/`) never includes `<gccore.h>`, `<gx2/*.h>`,
`<wpad/wpad.h>`, or any other platform header. Everything platform-specific
sits behind a small set of abstract driver interfaces in `source/drivers/`,
composed by a single `Platform` object:

```cpp
class Platform {
public:
    virtual void init(int width, int height) = 0;
    virtual void shutdown() = 0;
    virtual AudioDriver* getAudio() = 0;
    virtual VideoDriver* getVideo() = 0;
    virtual InputDriver* getInput() = 0;
    virtual FileSystemDriver* getFileSystem() = 0;
    virtual ThreadDriver* getThread() = 0;
    virtual SystemEvent getSystemEvent() = 0;
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
on - talks only to the five interfaces below.

#### The five driver interfaces

| Interface | Responsibility | GameCube/Wii (`drivers/ogc`) | Wii U (`drivers/wut`) |
|---|---|---|---|
| `VideoDriver` | Frame lifecycle, screen size/refresh rate/delta time; hands out an `ImageRenderer` (textured quads) and `GlyphRenderer` (glyph quads + solid rectangles) | `OgcVideoDriver` - raw GX, double-buffered XFB | `WutVideoDriver` - GX2 + libwhb's `WHBGfx*` helpers, submitting the same UI to both the TV and GamePad every frame, backed by two small custom GX2 shaders (`Texture2DShader`, `ColorShader`) |
| `AudioDriver` | Fixed one-shot PCM voices plus one background OGG stream | `OgcAudioDriver` - AESND | `WutAudioDriver` - AX (sndcore2), 16 voice slots plus a ring-buffered stereo stream path |
| `InputDriver` | Polls hardware and produces a per-channel `InputPadData` snapshot each frame, consumed by a persistent `InputController` per channel | `OgcInputDriver` - PAD (GameCube) / WPAD (Wiimote, Nunchuk, Classic, Wii U Pro Controller), plus Wii U GamePad via the vendored `libwiidrc` | `WutInputDriver` - VPAD (GamePad stick/buttons/touch) and KPAD/WPAD for up to 4 Wiimotes/Nunchuks/Classic/Pro Controllers, with GamePad touch mapped onto the same unified cursor/button fields as an IR pointer |
| `FileSystemDriver` | Storage device enumeration, mount/poll, and hot-plug detection (`StorageDevice`, `MountResult`) | `GameCubeFileSystemDriver` (memory cards, GC Loader, DVD) / `WiiFileSystemDriver` (hot-pluggable SD, USB, DVD) | `WutFileSystemDriver` - SD via a runtime-assigned FSA path, USB by polling a fixed set of candidate devoptab prefixes each cycle |
| `ThreadDriver` | Raw thread/mutex/condition-variable primitives | `OgcThreadDriver` - libogc's LWP | `WutThreadDriver` - coreinit's `OSThread`/`OSMutex`/`OSCondition` |

Application and library code never implements against `ThreadDriver`
directly - it uses the thin `Thread`/`Mutex`/`Cond` RAII wrapper classes
in `source/drivers/`, which forward to `platform->getThread()`. A
platform-agnostic `SystemTime`/`Ticks` helper (`source/drivers/Time.h`)
rounds out the layer for anything that needs monotonic timing without
touching `gettime()` or `OSGetSystemTime()` directly.

#### Repository layout

```text
libgui/
├── Makefile[.wii|.gc|.wiiu]   # per-platform build, dispatched by the top-level Makefile
├── data/                      # images/fonts/sounds/lang - one shared asset set, bin2o'd for all 3 platforms
├── meta/                      # Wii U .wuhb icon/splash assets
└── source/
    ├── demo.cpp / demo.h      # entry point / app template
    ├── menu.cpp / menu.h      # template menu screens (not part of the library itself)
    ├── filebrowser.cpp/.h     # SD/USB file browser built on GuiFileBrowser
    │
    ├── drivers/               # ---- Platform Abstraction Layer ----
    │   ├── Platform.h, VideoDriver.h, AudioDriver.h, InputDriver.h,
    │   │   FileSystemDriver.h, ThreadDriver.h   # the five abstract interfaces
    │   ├── Thread.h/.cpp, Mutex.h/.cpp, Cond.h/.cpp   # RAII wrappers app/core code uses directly
    │   ├── InputController.h/.cpp   # per-channel logical controller (repeat/orientation/scroll logic)
    │   ├── InputData.h, Time.h      # platform-agnostic input snapshot & monotonic timing
    │   │
    │   ├── ogc/                # GameCube + Wii implementation
    │   └── wut/                # Wii U implementation (+ wut/shaders: the GX2 shader plumbing)
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
screens (settings, SD/USB file browser) on top of the core library and
are explicitly *not* part of the library itself - they're a recommended
pattern to build from, not a hard dependency.


### Building

You'll need [devkitPro](https://devkitpro.org/) with `devkitPPC`
installed, plus the platform-specific pieces below (all available via
`dkp-pacman`/the devkitPro pacman repos):

* **GameCube / Wii**: `libogc2`, plus the `ppc` portlibs for `freetype`,
  `libpng`, `zlib`, and `libvorbisidec`/`libogg` (Tremor).
* **Wii U**: `wut`, `libwhb`, plus the same `ppc` portlibs above.

Then, from the repository root:

```sh
make -f Makefile.wii    # Wii  -> .dol
make -f Makefile.gc     # GameCube -> .dol
make -f Makefile.wiiu   # Wii U -> .rpx / .wuhb
make                    # builds all three
```

Each Makefile builds the same `source`, `source/drivers`, and
`source/libgui` trees, adding only the matching `drivers/ogc` or
`drivers/wut` (`+ drivers/wut/shaders`) directory for that platform.


### Quickstart

Start from the supplied template example (`source/demo.cpp`). For more
advanced uses, see the source code for Snes9x GX, FCE Ultra GX, and
Visual Boy Advance GX - all of which build their menu system on top of
libgui.


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
KPAD input, FSA-based storage) and a GameCube build, both made possible
by refactoring the library behind a proper hardware abstraction layer -
see [Architecture](#architecture) above and the changelog for the
complete list of changes.
