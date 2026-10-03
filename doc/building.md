# Building

This is the one build guide for **libgui**, **Snes9x GX**, **FCE Ultra GX**
and **Visual Boy Advance GX**. All four build for GameCube, Wii and Wii U from
a single source tree, with the same toolchain and the same dependencies. The
project READMEs only add what is specific to that project (extra libraries,
output file names) and link here.

The GitHub Actions workflow in each repository (`.github/workflows/build.yml`)
is a complete, working reference for every step below. If this page and the
workflow ever disagree, the workflow is right. Please open an issue or a pull
request so the page can be corrected.

You do not need to be a developer to follow this guide, but you should be
comfortable with a command line.

## 1. Install the toolchain

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started). On Windows
use the installer and the MSYS2 shell it provides (the package manager is
`pacman`). On Linux and macOS install `dkp-pacman` (the package manager is
`dkp-pacman`). The commands below are written with `dkp-pacman`; use `pacman`
if you are on Windows. Prefix them with `sudo` where your setup needs it.

Update the package database first:

```sh
dkp-pacman -Syu
```

Make sure the devkitPro environment variables are set (the installer does this
for you; on Linux/macOS add them to your shell profile):

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITPPC=$DEVKITPRO/devkitPPC
```

### All platforms

Install `devkitPPC` and the PowerPC portlibs:

```sh
dkp-pacman -S devkitPPC ppc-pkg-config ppc-freetype ppc-libpng ppc-zlib \
              ppc-libvorbisidec ppc-libogg
```

The three emulators also need `ppc-mxml` (libgui itself does not):

```sh
dkp-pacman -S ppc-mxml
```

You also need **CMake** and **Ninja**, which are used to build `libsmb2`
(section 2). If `ninja` is not on your `PATH`, install it with your system
package manager (for example `apt install ninja-build`).

### GameCube and Wii

Install `libogc2`, which provides the FAT, ISO9660, DVD, network and
controller libraries the Makefiles link against. libgui builds against
`libogc2`, not the original `libogc`. First add the libogc2 package repository
by following the instructions at
<https://github.com/extremscorner/pacman-packages#readme>, then:

```sh
dkp-pacman -S libogc2
```

When asked for a `libogc2-libfat` provider, `libogc2-libdvm` is generally
preferred (see the [libogc2 README](https://github.com/extremscorner/libogc2#installing)).

### Wii U

Install the Wii U toolchain, which provides `wut` (and with it `libwhb`):

```sh
dkp-pacman -S wiiu-dev
```

## 2. Build the libraries that come from source

`libsmb2`, `libmocha` and `libdvm` are built from the `dborth` forks and
installed into your devkitPro tree. They are not available as packages. The
workflows always use the latest commit of each fork, and so should you: if a
build starts failing after you pull the project, re-run this section against
the latest commits first.

If `$DEVKITPRO` is not writable by your user, run the `make install` steps
with `sudo -E` so your environment variables are preserved.

### libmocha (Wii U only)

Gives the Wii U build access to USB storage through the Mocha CFW component.

```sh
git clone --depth 1 https://github.com/dborth/libmocha.git
cd libmocha && make install && cd ..
```

### libdvm (Wii U only)

Mounts FAT, exFAT and NTFS volumes on USB storage on Wii U. It has
submodules, so the clone must include them:

```sh
git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/dborth/libdvm.git
cd libdvm && make install && cd ..
```

### libsmb2 (all platforms, once per platform)

Provides SMB network shares on GameCube, Wii and Wii U. Build and install it
once for each platform you intend to build for:

```sh
git clone --depth 1 https://github.com/dborth/libsmb2.git
cd libsmb2
make -f Makefile.platform gc_install      # GameCube
make -f Makefile.platform wii_install     # Wii
make -f Makefile.platform wiiu_install    # Wii U
```

Every platform builds into the same `build/` directory, so remove it between
platforms (`rm -rf build`) or you will be configuring one platform on top of
another.

## 3. Build the project

Clone the project you want and build it from its root directory:

```sh
git clone --depth 1 https://github.com/dborth/<project>.git
cd <project>
make -f Makefile.wii -j3      # Wii
make -f Makefile.gc -j3       # GameCube
make -f Makefile.wiiu -j3     # Wii U
```

Running plain `make` builds all three platforms, so it needs every dependency
above to be installed. Each platform also has a matching clean target
(`make wii-clean`, `gc-clean`, `wiiu-clean`, or `clean` for all).

| Project | Wii | GameCube | Wii U |
|---|---|---|---|
| libgui (demo app) | `libgui-wii-demo.dol` | `libgui-gc-demo.dol` | `libgui-wiiu-demo.wuhb` |
| Snes9x GX | `executables/snes9xgx-wii.dol` | `executables/snes9xgx-gc.dol` | `executables/snes9xgx-wiiu.wuhb` |
| FCE Ultra GX | `executables/fceugx-wii.dol` | `executables/fceugx-gc.dol` | `executables/fceugx-wiiu.wuhb` |
| Visual Boy Advance GX | `executables/vbagx-wii.dol` | `executables/vbagx-gc.dol` | `executables/vbagx-wiiu.wuhb` |

The libgui demo is written to the repository root; the emulators write to
`executables/`. The Wii U build produces an `.rpx` and packages it into a `.wuhb` for Aroma.
To install a build, copy it to your SD card as described in the project's
README (the Wii `.dol` is renamed to `boot.dol` inside `apps/<project>/`).

## Optional: debug logging

Logging is compiled out by default. To enable it, add `-DLOGGING_ENABLED=1`
to the compiler flags. See [Logging](logging.md).

## Troubleshooting

* **`DEVKITPRO` not set**: the Makefiles stop immediately with that message.
  Set the environment variables from section 1.
* **`cannot find -lsmb2`, `-lmocha` or `-ldvm`**: the library for that
  platform has not been installed yet (section 2). `libsmb2` is installed
  separately for each platform.
* **`cannot find -lmxml`**: install `ppc-mxml` (the emulators only).
* **Wii U libraries missing on a Wii or GameCube build**: they are not needed.
  Check you ran the platform-specific Makefile (`make -f Makefile.wii`) rather
  than plain `make`.
