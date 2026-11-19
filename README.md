# Perfect Engine Launcher

A work-in-progress native Perfect Dark port for Nintendo 3DS, based on the
[Perfect Dark decompilation](https://github.com/n64decomp/perfect_dark) and
[PC port](https://github.com/perfect-dark-pc-port/perfect_dark).
It uses a Citro3D renderer and shares 3DS platform work with
[Eyes of Gold](https://github.com/Raihwy3/eyes-of-gold).

Requires a **New 3DS or New 2DS**. The first mission has been completed on hardware;
the full campaign has not been tested. Expect rendering bugs, freezes and
slowdowns. Old 3DS models are not supported by this build.

## Features

- Dual-screen menus, with touchscreen controls and mission objectives on the bottom screen.
- A separate bottom-screen inventory preview.
- Circle Pad movement and C-stick aiming, touchscreen look and optional gyro controls.
- Stereoscopic depth for the game world, first-person weapons and supported UI.
- Native 400×240 and centered 320×240 display modes, with 30 or 60 FPS limits.
- Smooth custom crosshairs reworked from [Conker3DSRecomp](https://github.com/Raihwy3/Conker3DSRecomp), with adjustable colour and opacity.
- Graphics, audio, controls and crosshair settings saved between launches.
- Optional startup update checks, with confirmation before installation.

## Installation

1. Extract the install ZIP onto your SD card, keeping `3ds/perfect-engine-launcher/`.
2. Put your legally backed-up **US Rev 1 Perfect Dark ROM** in that folder, named **`pd.z64`**.
3. Make sure your console has DSP firmware at `/3ds/dspfirm.cdc`.
4. Launch **Perfect Engine Launcher** through Homebrew Launcher.

Keep the `.smdh` icon next to the `.3dsx` executable. Settings are stored in
`pd.ini`. No ROM or extracted game files are included in the install ZIP.

## Controls

Use **3DS Dual Stick** in the game's control settings. Circle Pad moves,
C-stick looks, L aims and R fires. A uses or accepts, B changes weapons or
goes back, X reloads and Y crouches. START pauses. D-pad Up changes weapon
function. Touchscreen look and gyro sensitivity are adjustable in 3DS Options.

## Building

You need Git, Python 3 and devkitPro's 3DS development tools: devkitARM,
libctru, Citro3D, picasso, 3dsxtool and smdhtool. The updater also needs the devkitPro
`3ds-curl`, `3ds-mbedtls` and `3ds-zlib` packages (including minizip). The scripts currently expect
devkitPro at `/opt/devkitpro`. `make` builds the launcher on Linux.

```sh
make
```

The first build downloads a pinned PC-port checkout and applies
`patches/perfect-dark-3ds.patch`. This keeps the engine changes reviewable
without copying the working checkout, extracted files or its Git history
into this repository. You do not need a ROM to build the launcher.

The install ZIP is written to
`local/releases/perfect-engine-launcher.zip`.

`make checks` runs the host audio-queue checks. `make updater-check` tests update
parsing, downloads and recovery; it also needs host curl, minizip/zlib, OpenSSL
headers and the `openssl` command. `make rom-checks` also tests
the decoder and ROM verification; those tests need your own ROM at
`local/roms/pd.ntsc-final.z64`. The optional desktop probes need a separate
CMake build and are not part of the normal launcher build.

## Updates
Updates replace only `perfect-engine-launcher.3dsx` and its `.smdh` icon.
The ROM, saves and settings stay in place. A new executable takes effect after
relaunching. Create `no-update-check.flag` in the game folder to disable checks.


## Credits and licenses

The launcher additions are MIT licensed. The upstream engine, Fast3D renderer,
Eyes of Gold code, GamepadMotionHelpers and adapted stereo/flick-stick code
retain their own copyright notices and licenses. See [CREDITS.txt](CREDITS.txt),
[LICENSE](LICENSE), `licenses/` and the notices in `vendor/` and `distribution/`.
The launcher license does not cover the original game's assets.

Disclosure: AI tools were used during the development and organization of this project.
