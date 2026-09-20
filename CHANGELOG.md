# Changelog

All notable changes to libgui are documented here. Versions follow
`MAJOR.MINOR`; dates are approximate, taken from the commit history.


## [2.0] - Unreleased

The headline of this release is **Wii U support**, added alongside a
**GameCube build** - both made possible by refactoring the library
behind a real hardware abstraction layer. libgui went from "a Wii
library" to "a library that happens to run on three consoles" without
changing the public shape of `GuiElement`/`GuiWindow`/`GuiButton`/etc.
Existing Wii apps built against 1.07 will need to update against the new
driver layer (see below) but the core widget API is unchanged.

This release also adds full storage hot-plug with multiple simultaneous
USB drives, network shares (SMB) on every console, and a multi-backend
logging framework.

### Added

**Wii U support**
* New `WutPlatform`, and a full `drivers/wut/` driver set: `WutVideoDriver`
  (GX2 + libwhb's `WHBGfx*` helpers, submitting the same UI to the TV and
  GamePad every frame), `WutAudioDriver` (AX/sndcore2, 16 fixed voice
  slots plus a ring-buffered stereo stream path, mixed to both the TV and
  the GamePad), `WutInputDriver` (VPAD for the GamePad's stick/buttons/
  touch, KPAD/WPAD for up to 4 Wiimotes/Nunchuks/Classic/Pro
  Controllers), `WutFileSystemDriver` (see **Storage** below), and
  `WutThreadDriver` (coreinit `OSThread`/`OSMutex`/`OSCondition`).
* Two small custom GX2 shaders (`Texture2DShader`, `ColorShader`) plus
  the supporting shader plumbing (`Shader`, `VertexShader`, `PixelShader`,
  `FetchShader`) in `drivers/wut/shaders/`, replacing GX's fixed-function
  pipeline for the Wii U build.
* GamePad touch input mapped onto the same unified cursor/button fields
  used by the Wiimote's IR pointer, so touch-driven UI works without a
  separate gesture API.
* HOME button handling: HOME is delivered to the app as an ordinary
  `INPUT_BTN_HOME` press rather than opening the system overlay, and
  `WutInputDriver::openHomeButtonOverlay()` opens the overlay on demand.
* `Makefile.wiiu`, packaging the `.rpx` into a `.wuhb` via `wuhbtool`
  with icon/TV-splash/GamePad-splash assets under `meta/`.

**GameCube support**
* New `GameCubePlatform` and `GameCubeFileSystemDriver` (SD Gecko in
  memory card slot A or B, SD2SP2 in serial port 2, GC Loader, and DVD),
  plus `Makefile.gc`. Video/audio/input code is shared with the Wii build
  and selects GameCube behavior at compile time via devkitPPC's `HW_DOL`
  macro.

**Hardware abstraction layer**
* Introduced the `Platform` composition root and the abstract driver
  interfaces (`VideoDriver`, `AudioDriver`, `InputDriver`,
  `FileSystemDriver`, `ThreadDriver`, and the `Logger`'s `LoggingDriver`
  backends) that `source/libgui/` now talks to exclusively - no platform
  header is included outside `source/drivers/`.
* `InputPadData`/`InputController`: a platform-agnostic per-channel
  input snapshot and a persistent per-channel controller object that
  owns orientation, deadzone, and directional-repeat/scroll-delay logic,
  replacing driver-specific pad handling in app code.
* Application lifecycle on `Platform`: `Status` (`Running`, `Paused`,
  `Exiting`), `SystemEvent` (`ShutdownRequested`, `ResetRequested`),
  `triggerExit()`, `shouldExit()`, and `requestExit()`, which shuts the
  drivers down and ends the app the way each console expects (return to
  the Wii U system menu, IOS power-off on Wii when the power button
  started the exit, plain exit otherwise). On Wii U the status is
  `Paused` while the app is in the background, and the video and audio
  drivers skip their work.
* `VideoDriver::getRefreshRate()`/`getDeltaTime()`

**Storage**
* Storage device enumeration/mount/poll unified behind `FileSystemDriver`
  (`StorageDevice`, `MountResult`), with a device-checking thread and
  hot-plug polling designed to run independently of the main loop.
* Full hot-plug on all three consoles. Wii: SD, USB, and DVD presence is
  polled. GameCube: SD Gecko slots A/B and SD2SP2 are polled with an EXI
  presence probe. Wii U: SD is static, USB attach/detach is detected by
  scanning the USB stack for storage interfaces (`WutUsbProbe`), and a
  mounted USB volume is verified with a real uncached sector read so a
  pulled drive is noticed even when the filesystem cache could still
  answer.
* Multiple simultaneous USB drives: up to three on Wii (`WiiUsbMulti`
  opens each IOS mass-storage device independently and exposes each as its
  own disc interface) and up to three on Wii U, as `DEVICE_USB`,
  `DEVICE_USB2`, and `DEVICE_USB3`. Drives take the first free slot in
  attach order.
* Wii U USB storage through libmocha's raw disc interface and libdvm
  (`dvm_wut`), which mounts FAT, exFAT, and NTFS volumes. The SD card is
  mounted natively by Cafe OS.
* `StorageDevice` carries a volume label, an `alwaysListed` flag, and
  optional capacity/free-space/block-size/read-only telemetry
  (`metricsValid`; filled in by the Wii U driver). `isDevicePresent()` is
  a cheap cached presence check that never mounts anything, so a device
  list can show only devices that are actually there.
* `getValidLoadDevices()`/`getValidSaveDevices()`, `getPath()` helpers, and
  `FindFirstMountedPath()`.

**Network shares (SMB)**
* New `SmbDriver` interface, owned by `FileSystemDriver` and reached via
  `getSmb()`, with `OgcSmbDriver` (GameCube via the Broadband Adapter, and
  Wii) and `WutSmbDriver` (Wii U), all built on libsmb2. A connected share
  is mounted as the `DEVICE_SMB` device at `smb:/` through a newlib
  devoptab, so the app uses ordinary POSIX file calls. `connect()` brings
  the network up as needed and returns `Success`, `InvalidSettings`,
  `NetworkUnavailable`, or `ConnectFailed`, with libsmb2's own error text
  available from `getLastError()`.

**Logging framework**
* New multi-backend `Logger` (`Logger.h`), reached through the
  `LOG_DEBUG`/`LOG_INFO`/`LOG_WARN`/`LOG_ERROR` macros (also usable from C
  code), with four severity levels plus `None`.
* Backends behind a `LoggingDriver` interface: console debug output
  (`SYS_Report`/`OSReport`), non-blocking UDP (Wii and Wii U), USB Gecko
  over EXI (GameCube and Wii), USB serial (Wii U, when the optional
  `usbserial` module header is available at build time), and a generic
  stdio-based file backend (`LoggerFile`) that works on every platform.
* `LogConfig` selects a single backend or, with `LogMode::Multi`, any
  combination via a `LOGGER_*` bitmask; configures the UDP target, Gecko
  channel, serial baud rate, log file path and flush policy
  (`Immediate`, `EveryNWrites`, `Never`); and toggles level tags and
  sequence numbers. The console debug output can be mirrored
  automatically, so logging works in emulators with no configuration.
* Reconfigurable at runtime: `Logger::init()` can be called again and
  reconciles the active backends, and `setLevel()` changes the minimum
  severity. A backend that fails to start (no SD card, bad address, no
  network) is skipped and reported instead of being fatal.
* Allocation-free (fixed 512-byte stack line buffer), thread-safe (one
  mutex serializes dispatch), and harmless to call before the logger
  exists or after it shuts down. Custom backends can be added with
  `registerBackend()`.
* Compiled out entirely by default: with `LOGGING_ENABLED` at 0 every
  `LOG_*()` call expands to nothing, so no format strings or argument
  evaluation are left in the binary and the logger is never created. Build
  with `-DLOGGING_ENABLED=1` to enable it.

**Threading and timing**
* `Thread`/`Mutex`/`Cond` RAII wrapper classes, backed by `ThreadDriver`,
  plus a portable `ThreadPriority` enum, `MutexLock` scope guard,
  `ThreadSync` (a mutex with a work and an idle condition variable), and
  `ThreadId` for comparing threads, including the main thread.
* Cooperative thread stopping: `Thread::requestStop()`/`stopRequested()`
  and an optional wake callback passed to `start()` to break a thread out of
  a wait.
* `Thread::JoinAll()`, an app-exit safety net that stops and joins every
  thread still running, so nothing touches driver state once shutdown
  begins.
* Platform-agnostic `SystemTime`/`Ticks` monotonic timing helper
  (`Time.h`) so app and library code no longer calls `gettime()`/`LWP_*` or
  their Wii U equivalents directly.

**Input**
* Adaptive IR pointer smoothing on Wii U using a One Euro Filter
  (`OneEuroFilter.h`): heavy smoothing at rest, light smoothing during fast
  motion.
* Menu hover rumble is a short tick with an enforced quiet gap, so moving
  across buttons doesn't buzz continuously. The Wii U GamePad uses a
  reduced-amplitude rumble pattern. `InputDriver::setRumbleEnabled()` toggles
  rumble globally.
* `InputDriver::setWiimoteOrientation()` selects vertical or horizontal
  Wiimote orientation for the Accept/Cancel triggers and directional input.

**Audio**
* `GuiSound` separates volume into music (any looping sound) and sound
  effects (any non-looping sound), each with its own global default volume.

**Rendering & assets**
* Rewritten, platform-agnostic FreeType2-based text renderer
  (`GuiTextRenderer`) with per-pixel-size glyph shaping/caching,
  delegating only the final rasterized-quad draw to a `GlyphRenderer`.
* New hashed msgid → UTF-8 `GuiTextTranslator`, loading a single binary
  `.lang` blob instead of per-string lookups.
* New built-in Tremor (integer OGG) player (`GuiSoundOggPlayer`),
  shared by every platform's `AudioDriver` for the background stream.

**Template app**
* Network Share settings screen (connect/disconnect and status), device
  list with volume labels that shows only devices that are present, error
  prompts when a device or share can't be opened, a "Wii U Overlay" button
  on Wii U, and orderly shutdown of the device-checking thread.

### Changed
* `GuiImage`/`GuiImageData` refactored to use significantly less memory:
  PNG decoding moved out of the video driver and into `GuiImageData`
  itself, using a single caller-supplied decode scratch buffer
  (`GuiImageData::setDecodeScratch()`) shared across every decode instead
  of a fresh allocation per image; decoding now fails fast if no scratch
  buffer has been set.
* Types standardized on `<cstdint>` fixed-width types throughout the
  public API.
* Binary/media assets moved out of `source/` and into `data/`.
* Numerous internal renames for consistency now that the same classes
  serve three platforms instead of one.

### Fixed
* GUI selection/focus edge cases, and proper cleanup when elements are
  removed from the main window mid-frame.
* Quirks in `GuiFileBrowser`/`GuiOptionBrowser` paging.

### Documentation
* Doxygen coverage extended to the entire `source/drivers/` hierarchy
  (previously only `source/libgui/` was documented), covering the
  abstract interfaces and both the `ogc` and `wut` concrete driver sets.
* Filled in missing/incorrect class and method documentation across
  `source/libgui/` (`GuiSoundOggPlayer`, `GuiTextRenderer`,
  `GuiTextTranslator`, `GuiTrigger`, and others).


## [1.07] - June 28, 2026
* Bugfix for virtual keyboard space key (missing null termination) (retro100)
* Updated build config for newer bin2o/devkitPPC header generation (Spotlight)
* Added WiiU Controller (libwiidrc) input support, split headers and began C++11 conversion, fixed optionbrowser/demo issues and truncation/unused-parameter warnings (Carsten Teibes)
* Added README.md (bladeoner)
* Fixed FreeType/portlibs linking order (meta)
* Fixed on-screen keyboard issues, bumped GUI thread memory, removed custom FreeType dependency, compressed images and fixed sRGB profile, updated makefile for current devkitPPC, silenced a compiler warning (dborth)

## [1.06] - July 22, 2011
* Compatibility with devkitPPC r24 and libogc 1.8.7
* Minor bug fixes and optimizations

## [1.05] - October 16, 2009
* Text alignment corrections
* Compatibility with devkitPPC r18 and libogc 1.8.0
* Removed outside dependencies - uses only devkitpro portlibs now
* Added grayscale method to image class (thanks dimok!)
* Other minor optimizations

## [1.04] - August 4, 2009
* Rewritten ogg player - fixed a crashing bug
* Improved text rendering performance
* Improved logic for option browser and file browser classes
* Onscreen keyboard class improvements
* GuiText: Added SetScroll and SetWrap and changed behavior of SetMaxWidth
* Other minor GUI logic corrections and code cleanup

## [1.03] - May 22, 2009
* Add file browser class to template - browses your SD card
* New images for the template (thanks mvit!)
* Add a function to get the parent element

## [1.02] - April 13, 2009
* Fixed letterboxing on PAL
* Add STATE_HELD for held button actions (eg: draggable elements)
* Now tracks state changes per-remote
* Default constructor for GuiImage
* Keyboard corrections, added more keyboard keys
* Better handling of multiple wiimote cursors on-screen
* Added functions for the ability to alter button behavior for all states
* Documented GuiTrigger class
* Refactor - moved trigger class definition to gui.h

## [1.01] - April 5, 2009
* Changed default sound format to 16bit PCM 48000
* Added loop option for OGG sound playback

## [1.00] - April 4, 2009
* Initial release
