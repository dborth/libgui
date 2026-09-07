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

### Added

**Wii U support**
* New `WutPlatform`, and a full `drivers/wut/` driver set: `WutVideoDriver`
  (GX2 + libwhb's `WHBGfx*` helpers, submitting the same UI to the TV and
  GamePad every frame), `WutAudioDriver` (AX/sndcore2, 16 fixed voice
  slots plus a ring-buffered stereo stream path), `WutInputDriver` (VPAD
  for the GamePad's stick/buttons/touch, KPAD/WPAD for up to 4
  Wiimotes/Nunchuks/Classic/Pro Controllers), `WutFileSystemDriver` (SD
  via a runtime-assigned FSA path, USB via devoptab prefix polling), and
  `WutThreadDriver` (coreinit `OSThread`/`OSMutex`/`OSCondition`).
* Two small custom GX2 shaders (`Texture2DShader`, `ColorShader`) plus
  the supporting shader plumbing (`Shader`, `VertexShader`, `PixelShader`,
  `FetchShader`) in `drivers/wut/shaders/`, replacing GX's fixed-function
  pipeline for the Wii U build.
* GamePad touch input mapped onto the same unified cursor/button fields
  used by the Wiimote's IR pointer, so touch-driven UI works without a
  separate gesture API; IR pointer position is smoothed to counter 
  `KPADReadEx` sampling faster/noisier than the UI update rate.
* `Makefile.wiiu`, packaging the `.rpx` into a `.wuhb` via `wuhbtool`
  with icon/TV-splash/GamePad-splash assets under `meta/`.

**GameCube support**
* New `GameCubePlatform` and `GameCubeFileSystemDriver` (memory card
  slots, GC Loader, DVD), plus `Makefile.gc`. Video/audio/input code is
  shared with the Wii build and selects GameCube behavior at compile
  time via devkitPPC's `HW_DOL` macro.

**Hardware abstraction layer**
* Introduced the `Platform` composition root and the five abstract
  driver interfaces (`VideoDriver`, `AudioDriver`, `InputDriver`,
  `FileSystemDriver`, `ThreadDriver`) that `source/libgui/` now talks to
  exclusively - no platform header is included outside `source/drivers/`.
* New `Thread`/`Mutex`/`Cond` RAII wrapper classes, backed by
  `ThreadDriver`, plus a `ThreadPriority` enum and platform-agnostic
  `SystemTime`/`Ticks` monotonic timing helper (`Time.h`) so app and
  library code no longer calls `gettime()`/`LWP_*` or their Wii U
  equivalents directly.
* `InputPadData`/`InputController`: a platform-agnostic per-channel
  input snapshot and a persistent per-channel controller object that
  owns orientation, deadzone, and directional-repeat/scroll-delay logic,
  replacing driver-specific pad handling in app code.
* Storage device enumeration/mount/poll unified behind
  `FileSystemDriver` (`StorageDevice`, `MountResult`), with a device-
  checking thread and hot-plug polling designed to run independently of
  the main loop.

**Rendering & assets**
* Rewritten, platform-agnostic FreeType2-based text renderer
  (`GuiTextRenderer`) with per-pixel-size glyph shaping/caching,
  delegating only the final rasterized-quad draw to a `GlyphRenderer`.
* New hashed msgid → UTF-8 `GuiTextTranslator`, loading a single binary
  `.lang` blob instead of per-string lookups.
* New built-in Tremor (integer OGG) player (`GuiSoundOggPlayer`),
  shared by every platform's `AudioDriver` for the background stream.
* `VideoDriver::getRefreshRate()`/`getDeltaTime()`, so frame-timing code
  no longer hardcodes NTSC/PAL assumptions.

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
