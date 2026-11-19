# Title/front-end bring-up status

The project is isolated from both GoldenEye projects. Upstream commit:
`a4b58e6dd14d5842d588e9841efa8bceb8b86fc3` (port branch).

Completed:

- Compiled all 360 original engine/interpreter units for ARM and linked a 3DSX.
- Implemented libctru lifecycle, files, clock, input and diagnostic logging.
- Connected the original Fast3D interpreter to the reused PICA renderer.
- Verified the full supplied US Rev 1 ROM SHA-1; tested missing, truncated and
  modified images against the actual verifier.
- Compared the target inflater with zlib on the data segment and 1,403 compressed
  ROM files, including output canaries.
- Ran the unmodified desktop engine through 120 Defection frames using software
  OpenGL, with audio disabled. This validates the ROM and original mission path,
  not ARM execution or the 3DS renderer's output.

- Submitted 120 Defection frames through the actual PICA material/draw code in a
  host probe with GPU calls mocked; no unsupported material or ASAN memory error
  was reported. This does not validate the rendered image.

- Ran the 3DS boot target on the desktop engine: the title sequence advanced
  through the legal screen, Rare logo, Nintendo logo, Rare presents and PD logo,
  then changed to the front-end menu stage (`0x26`) at about 1,367 frames. The
  menu stage then ran 240 more frames with its geometry present and no crash.

The initial target is New 3DS/2DS, with a 32 MiB resident ROM and a 16 MiB engine
heap. The launcher boots the title sequence (`--boot-stage 0x5a`) and continues
to the front-end menu instead of the attract-mode Defection demo. Rendering is
capped at 30 fps; the original simulation clock is retained. Framebuffer effects
are disabled. Engine stereo PCM16 audio now feeds a bounded DSP queue at
22020 Hz. Defection was completed on hardware by the user with the previous
silent diagnostic. The audio/title/menu build still needs hardware testing. Azahar
could not start inside this sandbox because its startup socket is blocked.

Reports: `build/engine-compile.json`, `build/inflate/validation.json`,
`build/rom-verification.json`, `build/package-validation.json`.
The optional desktop/PICA host probes require an existing CMake desktop build
in `build/desktop-reference`, SDL2/OpenGL development libraries and software EGL.
The PICA host probe uses mock GPU calls: it checks command submission and material
handling, not rendered pixels, GPU timing or hardware safety.

Next: run the diagnostic build on hardware and inspect `pd-3ds.log`. This remains
a private build pending compatibility work and a distribution content audit.

Audio queue checks exercise stereo frame counts, copied-buffer ownership, split
submissions, safe slot reuse and shutdown under ASAN/UBSAN. No test tones or
prerecorded game sounds are used. The existing SD directory is retained so ROM
and save paths remain compatible.

The new audio-enabled host probe submitted the full title sequence and 120
front-end frames through the PICA command/material code with GPU calls mocked.
It passed without an unsupported material or ASAN failure; DSP playback and
actual 3DS menu output remain hardware checks.

## 3DS controls and options

The default HID bridge now follows Eyes of Gold's Vesper mapping, including
dedicated reload/crouch and separate menu accept/cancel bits. Stick sensitivity
and deadzones now affect sampled axes instead of being no-op settings.

The 3DS-only options layout removes fullscreen/window settings, mouse/device/key
binding menus, unsupported AA and anisotropy, detail textures, framebuffer
effects and other no-op controls. Supported gameplay/HUD/audio settings remain.
A 30/60 dropdown, nearest/bilinear filtering and centered 320×240 / full 400×240
rendering modes are functional and persist through pd.ini. This does not add an
800-wide output mode.

`tools/check_input_mapping.py` exercises the actual input bridge with ASAN/UBSAN.
`tools/check_video_settings.py` exercises the actual video adapter's dimensions,
viewport/scissor offsets, physical framebuffer guard, caps and save calls. Both
optional host checks use the configured desktop-audio-menu include paths.
The changed controls/options and 4:3 presentation need a console test.

Stick-order correction: the PC engine defaults swapSticks=1 because its first
analog pair drives look and its second drives movement. The 3DS bridge now
feeds C-stick into the first pair and Circle Pad into the second. Menu navigation
reads both pairs; sensitivity/deadzone settings retain physical-stick ordering.
The extended scheme is labelled 3DS Dual Stick in both control selectors.
The input regression check verifies these engine roles explicitly.
