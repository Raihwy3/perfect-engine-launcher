# Performance and usability pass

The previous diagnostic completed Defection on a New 3DS. This build still needs
hardware validation; no FPS improvement is claimed from host timings.

Implemented:

- Restored the engine's authored model distance nodes, disabled by the PC port.
  Original, Balanced, Strong and Dynamic strengths adjust distance scale; Dynamic
  targets the selected frame cap. This does not generate new low-poly level meshes.
- Distant texture mip levels, bounded mip promotion, and cosmetic particle/shard
  thinning beyond distance thresholds, with culling behind the engine fog limit.
  Effect simulation and damage are not reduced.
- View reduction scales the engine far plane and recomputes its fog range. Original
  preserves the supplied range. No hard room-distance deletion was introduced.
- Material input/fog hints reduce redundant planning. RDP constant changes form
  batch barriers, so the hints cannot treat differing colours as a single constant.
- Experimental model-space GPU submissions with original CPU visibility/face
  decisions retained. Shared vertex IDs enable indexed conversion reuse; hashed
  complete packed attributes enable retained buffers without stale lighting/UV/fog.
  CPU transforms used for the reference decisions are still performed: this is
  not a complete GPU-only transform pipeline. Hashing/cache overhead can outweigh
  savings in some scenes; the GPU and retained-cache options can be tested separately.
- Optional cached guard sight queries at Original/30/15/5 Hz; movement, stage changes,
  cutscenes and alertness invalidate or tighten the cache. Optional hidden-guard
  caching extends failed sight results, not successful detections.
- Optional distant enemy AI script cadence, restricted to unseen, unalerted human
  enemy guards standing/patrolling beyond 2000 units. Combat, allies, special actors,
  cutscenes and multiplayer bypass it. It does not freeze all distant scripts,
  movement or physics, and can delay eligible guard decisions; Original is default.
- Simple dark translucent glass removes reflection texture/UV work for glass objects.
  Original remains selectable; collision, breaking and hit processing are untouched.
- Optional audio synthesis worker on New-model core 2 with synchronous fallback.
  The main thread joins at the end of the frame before game state changes or stage
  memory is freed. This preserves single ownership of the original audio state;
  it does not make gameplay simulation run concurrently on all cores. Worker faults
  are handed back to the main thread for orderly error reporting.
- Optional DSP dry-voice summation on channels 1–23, with the original CPU wet/reverb
  and overflow on channel 0. Pending blocks are submitted together in the DSP callback.
  Sources are copied into owned linear buffers, busy slots are not overwritten,
  and allocation failure falls back to the CPU path. The choice takes effect on restart.
- Persistent image smoothing/sharpening, vignette strength/activation, bloom, grain
  and vertical 2x supersampling using the existing PICA effects. Smoothing is not FXAA
  or CAS. Effects default off and can increase GPU cost. No 800-wide mode was added.
- Scoped stereo depth for the top-screen menus and HUD, with batch barriers between
  scene/UI scopes. This does not implement full stereoscopic world rendering.
- Bottom-screen text menu with touch focus/selection, unlocked mission rows/paging,
  option navigation, objective status and drag aiming. Content is read from the ROM
  through the original engine. It is a new flat UI, not a 1:1 rendered PDA mirror.
- Five-second profiling windows and backend cache counters in pd-3ds.log.

Validation:

- All original engine units cross-compile and the 3DSX relocation/package checks run.
- Input and video adapter checks cover default control roles, display bounds,
  centered 4:3 viewport/scissor, physical framebuffer size, frame caps and save calls.
- Runtime policy tests cover sight-cache age, movement, alertness, cutscenes, stage
  reset, eligible AI cadence and exclusions, particle/fog policy and touch isolation.
- PCM queue tests cover ownership, busy slots, pending DSP channel submission and
  sidecar release. A mixer test compares dry samples/stereo order against the CPU
  envelope mixer while verifying identical wet samples and saved envelope state.
- The performance host probe runs modified engine/interpreter units through actual
  PICA command/material code with mocked GPU calls. It checks transformed indexed
  vertices against CPU clip coordinates and asserts claimed uniform input colours
  really are uniform. It does not rasterize pixels, emulate PICA float24 precision,
  exercise actual multicore scheduling, or establish speed on a console.

The DSP sidecar helper was reused from Eyes of Gold's native_dsp_mix.c/.h under
its native MIT license; the existing package includes that license. No audio,
texture, model or other game asset was added to the package.

## Viewmodel and objective display follow-up

The 3DS interpreter now uses the Eyes of Gold homogeneous triangle-facing helper
with adaptive double precision for uncertain signs. Triangles crossing the eye
plane bypass the invalid trivial clip-bit rejection. First-person weapon draws
are explicitly scoped and keep the full-detail CPU attribute path even when
experimental GPU transforms or texture LOD are enabled. This targets missing
weapon facets; the visual result still needs a console check.

The bottom LCD renders a separate original-engine display list at 320×240. It
uses `menuitemObjectivesRenderOne`, the ROM-loaded Handel Gothic fonts and the
original objective lines/status colours. Status comes from the engine's cached
results so the extra display does not run objective checks again. It is shown
during gameplay and on objective pause pages; other menus retain the touch list.
The GPU and console-text paths explicitly hand ownership of the bottom LCD back
and forth, and the top viewport/aspect are restored after the bottom draw.

The geometry sign test covers 100,000 zero/mixed-W cases against a long-double
reference. Policy checks cover mission/cutscene/pause/CI/title display selection,
and the video test checks bottom output without the top screen's 4:3 inset. The
ASAN engine submission test reached 600 Defection frames and exercised 148 bottom
draws (1,056 vertices), with target/viewport/clip checks and no memory error. These
are host command checks, not PICA rasterization or hardware screenshots.

A further 600-frame gameplay probe uses the original mission-script intro skip,
then injects alternating look, short movement, aim, firing and reload inputs.
It exercised 292,007 scoped first-person triangles and 35,668 bottom submissions
(254,496 vertices). Target, native viewport and clipping assertions passed, as
did the uniform-material and world-transform reference checks under ASAN. The
900-frame unmodified intro path and 1,487-frame title/front-end path also passed.

## Startup bounds and stall recovery correction

The first objectives build incorrectly used `MAX_OBJECTIVES` (10) to walk
`g_Briefing.objectivenames` and `objectivedifficulties`, which each have six
slots. This was an out-of-bounds read in the new adapter, even though the initial
ASAN probe did not detect it. The adapter now collects indices using the actual
briefing array length; the same bounded list drives counting and drawing. Tests
cover all six slots, difficulty filtering and output canaries. The host engine
probe additionally enables the bounds sanitizer for this adapter.

The 3DS timing path caps a single elapsed interval to six 60 Hz ticks (100 ms),
drops the excess and rebases to the actual counter value. Both residual counters
are reset after a long stall, and stage loading resets the timing anchor before
the first game tick. Normal 30/60 FPS updates keep their original timing. This
also bounds the scheduler's audio synthesis loop, which uses the same tick count.
Controlled-clock tests cover 1-45 second stalls, immediate next-frame recovery,
32-bit counter wrap and stage resets.

Routine SD logging is buffered and flushed once per second at frame boundaries;
fatal errors and normal shutdown still flush. A console crash may lose the last
second of buffered diagnostics. No current console log was available to identify
the original hardware stall, so the startup crash and stall trigger remain
unconfirmed on hardware.

## Software bottom-screen isolation build

Hardware still crashed after the bounds fix; there was no fresh hardware log or
dump to establish the fault location. The next build removes the bottom-screen
GPU path from objective rendering, and restores world culling to the prior
reference behaviour. The stable facing helper is limited to first-person models.

Original objective drawing still supplies text positions, fonts, lines and cached
status colours. Dedicated hooks rasterize its CI4 font data and line gradients to
a CPU RGB565 canvas. The original display-list commands are written only to a
private scratch list and are never submitted. No bottom viewport, matrix, render
target or GSP output change is made by this path. The LCD copy uses the existing
software framebuffer, and is skipped while the displayed content is unchanged.

The full title/menu-to-Defection test passed with audio and 600 scripted gameplay
frames. It exercised 291,955 first-person triangles, zero bottom GPU submissions
and two software refreshes. The RGB565 output was exported and visually checked.
This is an isolation build, not confirmation that the hardware crash is fixed.

## Bottom-screen dialog presentation

The bottom screen now uses the ROM's Handel Gothic fonts and the engine's dialog
palette throughout menus, instead of the generic console menu. Its title comes
from the current dialog; the objective display uses the mission's Status title.
The title strip uses the original left/middle/right gradient, with the original
body fill and three border colours. Sibling dialog titles appear vertically at
the sides, with touch targets for switching tabs. Menu values sit to the right of
labels, and lists, labels and briefing paragraphs use the same typography.

The objective page retains the original numbered objective drawing and status
connectors, adds its Abort entry when paused, and keeps the full window while
playing. Abort, side tabs and footer targets are mapped to the corresponding
original menu actions. Touch-and-drag aiming outside menus is unchanged. The
initial boot/error console remains available before the ROM fonts are loaded.

Rendering still uses the software RGB565 framebuffer; the bottom GPU target is
not used. Host validation passed title/menu to Defection with audio, 600 frames,
and an actual pause-dialog opening. The exported paused objective framebuffer
was inspected. Touch policy tests cover rows, footer, side tabs and Abort. This
presentation change still needs testing on the console.

## Original menu layout mirroring

The previous fullscreen reconstruction left Inventory as a simple list and made
short dialogs much taller than their originals. Bottom-screen menus now capture
text and line primitives during the existing original dialog render. This keeps
the original columns, row positions, alignment, scrolling, live labels, selected
colours and descriptions, without calling menu handlers or rendering the menu a
second time. Dialogs retain their original proportions and are centred on the
bottom LCD, with downscaling only when needed to fit. The title and border use
the existing original palette. Text is clipped to the dialog body.

The added Back/Previous/Next/Select strip has been removed. Touch hits come from
the original selectable rows and actual visible list-option positions. Side
labels switch tabs; touching the title goes back. Sliders accept touches on the
left/right halves of their row. Physical controls continue to operate the
original menu. No invisible footer action zones are used while mirroring.

The mirror is refreshed every six renders. It remains a software RGB565 path:
3D inventory previews and the animated background are not mirrored yet. The
original top screen continues rendering those normally. No bottom GPU target or
additional original-engine rendering pass is introduced.

## Bottom UI cadence and foreground near plane

Removed the six-render capture throttle and six-render presentation cadence for
the native bottom menu. They made the bottom menu update at only 5 Hz with a
30 FPS game, or 10 Hz at 60 FPS. Mirrored menus now capture and present every
rendered frame. Runtime skips the entire fallback menu builder when the native
mirror is available, avoiding duplicate list/label/value handlers. Native-size
pixel placement uses integer offsets; solid-colour primitives skip gradient
calculations. The idle bottom panel is retained instead of redrawn repeatedly.
The framebuffer transpose uses 8x8 tiles; an ASAN/UBSAN check verifies every LCD
pixel and buffer canaries against the original coordinate mapping.

Foreground triangles no longer use the trivial all-outside rejection for mixed
positive/negative W vertices. First-person vertices with positive W that cross
the near plane are clamped just inside it before PICA depth conversion. This
matches the intent of the N64 foreground path, whose CPU near-plane rejection
is disabled, while leaving world geometry and screen-space X/Y unchanged. The
host run exercised both near-crossing and behind-camera weapon vertices. The
visual result still requires hardware verification; this is not confirmation
that all missing-facet defects are resolved.

New `bottom-copy`, `bottom-draw` profile entries and viewmodel clipping counts
are written to the game log. The available copied console log is still from
build eced2d5dfed9, so it cannot establish the current gameplay bottleneck. The
host intro/menu/Defection/Status/Inventory run completed 700 frames with audio,
293,876 foreground triangles and zero bottom GPU draws. Host timing is not a
3DS performance measurement.

## Completed menu-frame copy and ordered foreground depth clears

The hardware log from build a1372e29c1b2 confirms that software menu drawing cost
about 7.3 ms per captured menu frame. The animated element behind the original
menus is the engine's HUD-piece model, not a standalone background bitmap.

On hardware, bottom menus now use the completed left top-LCD image. The backend
records the destination framebuffer before FrameEnd's LCD transfer, then reads
that same buffer only after the next blocking FrameBegin has completed the
previous GPU queue. It invalidates the CPU cache before reading. This introduces
one frame of latency, with no extra GPU render, render target, framebuffer
allocation, output reconfiguration or additional GPU wait. The centre 320 pixels
are copied at 1:1 resolution, with a menu/side-tab mask so the top-screen health
bar is not copied. BGR8 is converted to the existing RGB565 bottom framebuffer.
Animated HUD-piece geometry, weapon previews, fonts and transitions are thus
copied from the original rendering. Touch coordinates use the original centered
aspect correction. Unsupported output formats or a failed cache invalidation
fall back to the software mirror. `bottom-frame` reports hardware copy time.
Host tests validate conversion, crop, masks, orientation, canaries and touch
projection. They do not emulate GPU framebuffer transfers; console testing is
required for this new copy path.

Found a separate weapon-depth ordering defect: the original bgunRender clears
depth via viPrepareZbuf before rendering the foreground weapon. The PICA adapter
used C3D_FrameBufClear, which queues GX_MemoryFill immediately, while preceding
3D commands remained deferred until FrameEnd. This let world geometry repopulate
depth after the intended weapon clear. The adapter now flushes pending draws,
flushes vertex/index/cache data, and calls C3D_FrameSplit before an in-frame
clear following geometry. The host checks require this split before depth clears
with prior vertices. The 700-frame gameplay/pause/inventory probe passed with
490 ordered splits, 293,912 foreground triangles and zero bottom GPU draws.
The earlier foreground vertex-depth clamp was removed: it did not resolve the
reported problem and could distort depth. The hardware visual result is still
unverified.

## Fullscreen menu-window copy

Replaced the fixed centre-strip copy with a source rectangle around the active
menu window and its side tabs, expressed in physical top-LCD coordinates. The
whole visible source rectangle is nearest-sampled into the 320x240 bottom LCD,
including its original animated background. Short windows therefore expand
vertically as well as horizontally; this is a scaled window image, rather than
an independent reflow of the original menu. Touch input uses the inverse of the
same clamped source rectangle. Borders and side-tab targets stay in the copied
image; there is no separate control strip.

Window dimensions are captured when the original dialog renders, rather than
read later from an animating dialog. The transfer source rectangle is recorded
alongside the completed framebuffer pointer. Empty and offscreen rectangles are
clipped safely before sampling. ASAN/UBSAN checks cover full, partial, clipped,
empty and one-pixel windows, all output pixels, colour channels, orientation,
buffer canaries and the resized touch mapping.

The new hardware log is from b5a324d8df21 and ends without a fatal message or
fault address. No newer crash dump is present. It does not identify the reported
mid-level crash, and this fullscreen-layout change does not claim to fix it.

The longer host probe completed 3,000 Defection gameplay frames, exercising
1,879,626 first-person triangles and 2,880 ordered depth splits, without a host
fault. It did not reproduce or diagnose the reported hardware crash.

## Native bottom window with a separate background

The fullscreen bitmap zoom was not the requested layout. Bottom menus now draw
a native 320x240 window, including the original palette/title strip/borders,
side titles, native-resolution ROM font glyphs, reflowed row/column containers
and touch targets. Font pixels and row-local line spacing are not scaled. The
original text callbacks supply live values, status colours and strings.

A display-list marker immediately after the original HUD-piece/background pass
requests a background-only transfer, before the health bar and dialog text are
drawn. The backend submits prior geometry, queues a display transfer to a
288,000-byte linear BGR buffer, and resumes top-screen rendering. After the next
blocking FrameBegin, the completed background is read and composed with the
native RGB565 UI and its alpha coverage. The existing GPU scene is not rendered
again, and no second bottom render target is introduced. Inventory's model area
is copied separately from the completed top image and fitted with its aspect
ratio preserved; text, borders and controls are never part of that image copy.

This changes the bottom presentation only. The previously reported hardware
crash is still undiagnosed; a host pass cannot confirm GPU transfer behaviour
or console stability. Bottom background/preview compositing needs hardware
validation.

## Perfect Engine Launcher / backdrop fixes

Renamed the launcher-facing boot/error text, log header, window title, SMDH
metadata, package README and main source documentation to Perfect Engine
Launcher. The build now writes `perfect-engine-launcher-private.zip`. Existing
SD installation/config/ROM paths remain compatible.

Two concrete problems were found in the background path. The capture marker
preceded the projected menu-backdrop display list, so those planes were absent
from the snapshot. It now follows that display list and precedes the dialog UI.
Also, filtered rendering uses padded 256x512 surfaces (512x512 when supersampling)
rather than an unpadded 240x400 renderbuffer. The transfer now uses matching
source/output dimensions, accounts for supersampling, and publishes its actual
BGR row stride. CPU sampling uses that stride and the original window interior
as a background-only source region. Native bottom text/controls are unchanged.
Transfer failures are checked and logged instead of publishing invalid content.

Host checks exercise actual transfer-setup code with normal, padded/filter and
supersampled dimensions and verify that a failed transfer is not published.
ASAN/UBSAN checks verify visible-pixel decoding and buffer canaries for normal
and padded surfaces. These checks cannot confirm the appearance of a real GPU
snapshot; the background fix needs another console test. The game log now
reports its surface dimensions and any transfer failure.

## Menu-open freeze recovery

The background-transfer build froze on menu opening on hardware. No fresh log
for that build is available locally, so the exact GPU fault is not established.
The recovery build removes the background snapshot marker from the engine's
display list, makes the backend snapshot callback issue no commands, and returns
false from the platform frame-copy capability query without allocating a buffer.
Thus menu opening cannot enter the new split/transfer/composite path. The native
software bottom UI and Perfect Engine Launcher branding remain. The animated
background and captured Inventory model preview are temporarily disabled.
Pre-existing foreground depth clears remain separate from this change.

The backend regression check explicitly verifies that a background snapshot
issues no transfer or split for normal, padded and supersampled targets, even
when mock platform callbacks advertise an available buffer. A host stress mode
repeatedly opens the pause menu, switches tabs and closes it while gameplay
continues. Hardware recovery still requires confirmation on the console.

## Release optimization and retained bottom panels

The original engine archive and game adapters now compile with `-O2` instead
of the bring-up `-Og` setting. This includes the Fast3D interpreter, original
audio synthesizer and scene-building code. `-fno-strict-aliasing`, `-fwrapv` and
normal floating-point semantics remain; no fast-math or simulation-rate change
was added. The compiler mode is recorded in `build/engine-compile.json`.

Closed objective/launcher panels now reuse their completed GPU target for three
out of four rendered frames. Stage, language, difficulty, objective text IDs or
status changes trigger an immediate redraw. Opening an interactive menu disables
the cache. Retained frames still transfer the target to the current bottom LCD
buffer, keeping double-buffered output consistent. No extra game framebuffers
are allocated. The host probe verified 1350 retained frames over 1800 frames.

`Profile audio-join` measures main-thread time waiting for the audio worker,
separately from its full synthesis duration. The available copied hardware log
was from an older build, so its timings are not a before/after benchmark for this
change. New console measurements are required before claiming an FPS improvement.

### Hardware regression: release optimization rolled back

The whole-engine `-O2`/retained-panel build crashed immediately on the user's
console despite passing host checks. The complete performance pass was reverted
to the previous `-Og` compiler mode and original panel submission path. The
pre-optimization ZIP was restored byte-for-byte. The copied hardware log predates
this crash, so no specific faulting instruction or optimization is established.
Future optimization work must isolate changes and retain a working baseline.


## Gameplay bottom-panel cadence

The closed mission-objectives panel now rebuilds its display list at up to
10 Hz using elapsed time, rather than every rendered frame. Cached objective
status, text ID/difficulty or stage changes force a redraw immediately. Menus,
cutscenes, the title and CI windows remain full-rate. Closing a menu or resetting
the stage forces a fresh first image. Touch aiming and normal engine simulation
keep their original cadence.

Skipped updates submit a small retention marker. The backend keeps the completed
bottom target attached and marks it for LCD output without clearing it, replaying
an old display list or allocating another framebuffer. This keeps both LCD
buffers consistent. The LCD still scans normally; only content generation and
rendering are reduced.

The native GPU menu pass already rebuilds its own touch rectangles. Top-screen
software capture was therefore clearing and rasterising a canvas that the bottom
pass immediately discarded. Top capture now returns early on 3DS; bottom hit
capture remains. Runtime fallback text construction is also skipped when the
native objective panel already handles the display.

This pass retains the previous -Og engine compiler mode. The controlled-clock
check measured 100 redraws and 500 retained frames over a 600-frame/60 Hz sequence,
with immediate invalidation on changed keys, menu entry/exit, clock resets and
stalls. The actual objective adapter was checked for status, difficulty and text
changes. ASAN-backed engine/PICA probes passed 1,800 gameplay frames with nine
pause openings, eight tab switches and eight closes, retaining 883 frames. A
separate title/audio-to-Defection run passed 800 mission frames and 270 rotating
Inventory preview fits. No hardware FPS improvement is claimed from these host
runs. `Bottom cadence` lines in pd-3ds.log report actual redraw/retention counts
for the next console test.


## Audio queue and renderer state corrections

Combat Simulator no longer qualifies for the single-player mission panel, so
its bottom window cannot use the stale Defection mission title. D-pad Up now
selects secondary weapon function during gameplay, while retaining Up navigation
in menus; Select is left available for the existing Select+Start exit chord.

Queue accounting subtracts the DSP's current sample position from the playing
buffer. Hardware synthesis aims for roughly 100 ms queued rather than 50 ms,
providing more margin for slow frames. Silent DSP channels now reference a shared
immutable zero buffer, avoiding a separate clear and cache flush per silent
channel per submission. Channel lengths and synchronization remain unchanged.
This does not make synthesis independent of frame scheduling, so long stalls
can still interrupt dialogue.

Decoded texture cache keys now distinguish palette contents and image/tile
strides. Reflection lighting coefficients are refreshed after matrix pops, and
the look-at enable check includes the Z direction. Zero-length direction vectors
remain zero instead of producing NaNs. These correct renderer state errors;
the reported Institute computer text and reflective surfaces still need a
console visual check. The engine remains compiled with -Og.


## Touchscreen naming keyboard

Naming dialogs use a native bottom-screen keyboard with 28x30-pixel letter
keys, wider action keys and the engine's menu palette. Every key registers its
own touch rectangle. Touch sets the existing keyboard row/column and sends the
same selection action as A, preserving character limits, Caps, Delete, Cancel
and OK behavior for all dialogs using MENUITEMTYPE_KEYBOARD. The bottom pass
reserves 194 pixels for the widget. Controller navigation and the top layout
remain available; the desktop-only external-keyboard row is omitted on 3DS.

The PICA host probe's PD_PROBE_KEYBOARD scenario opens a naming dialog and
checks all 50 cell centres, including the shared action-key spans, then uses
the original engine tick function to type, delete, change Caps and commit.


## Crisp top menu text

Menu font setup selects point sampling on 3DS instead of bilinear/average
filtering. Original glyphs, window layout and world texture filtering are
retained. When the render-state snapshot indicates a menu, presentation uses
the copy pass without image smoothing/sharpening taps. Supersampling and other
configured effects retain their existing behavior. This reduces filtering blur
at the existing native LCD resolution; it does not replace the ROM bitmap fonts
with higher-detail glyphs. The keyboard integration probe checks the actual
menu font display-list setup selects point filtering.

The top-font point-filter override was reverted after console feedback found
it visibly pixelated. Top text again follows the engine's 2D filtering policy;
the bottom's native-size font pass retains point sampling. Menu presentation
still avoids the optional full-frame image filter. This is a filtering correction,
not a higher-resolution font implementation.


## Native top-screen menu layout

Top dialogs now have their own LCD-coordinate pass: 400x240 in widescreen or
320x240 in the centered 4:3 viewport. Window positions, extents and column widths
are converted from logical menu coordinates. Font pixels, row heights and scroll
offsets are left at native size, so they are no longer enlarged by the old
320x220 mapping. The pass restores dialog/column geometry and VI/projection state
after drawing; controller input and bottom layout still use the original state.
The Fast3D scope saves and restores RSP/RDP state and forces viewport/scissor and
texture rebinding on exit. It does not change world rendering resolution.

Menu models still load and tick once. Their temporary 3D view is restored to the
native 2D view before text is drawn, and the desktop aspect correction is skipped
when the model scissor is already expressed in native pixels. Bottom inventory
previews continue to use the single captured geometry stream.

ASAN/PICA checks passed title/audio startup through 800 mission frames with
373 native dialog passes, 43,408 one-to-one pixel rectangle checks and 270
rotating inventory fits. A separate 4:3 keyboard run passed 800 frames, 279
dialog passes and 23,902 pixel rectangle checks. An 1,800-frame stress run passed
eight opens, eight swipes, eight closes, 1,221 dialog passes and 122,700 pixel
rectangle checks. The layout probe checks restored VI, dialog dimensions, columns
and unchanged row data. UI ownership/input tests also pass. These verify native
geometry and stability on the host; final appearance still needs console testing.


## Random-freeze memory fixes

Two memory-lifetime failures were reproduced without needing a console log:

- If the linear DSP allocation or the normal-heap dry-mixer allocation failed,
  CPU fallback allocated only the PCM ring, then cleared an 8 KiB silence buffer
  just past its end. The fallback now reserves the silence buffer as well.
  Audio-queue ASAN tests inject each allocation failure separately and verify
  initialization, CPU submission and shutdown. The fallback path failed before the correction
  with a heap-buffer-overflow; both injected failures now pass, along with normal CPU/DSP tests.
- Texture-cache eviction could erase a node still referenced by a texture unit
  and recycle its GPU texture ID. A cache-pressure probe reproduced a
  heap-use-after-free while a bound entry was retained across more than 1,024
  imports on the other unit. Eviction now skips currently bound entries. The
  map also reserves its maximum capacity before insertion because LRU entries
  hold map iterators, which rehashing would invalidate.

These are concrete causes of memory corruption, not confirmation that every
reported hardware freeze shares one cause. No audio worker timeout, forced
thread cancellation or GPU wait workaround has been added.


## Deferred backgrounds, scratch bounds and install paths

The native top dialog now creates and finishes its own deferred projection-plane
list inside the native coordinate scope. Previously it appended into the outer
list, then restored that list's write cursor and discarded the appended planes.
Each deferred list can now branch to another frame-owned block before its
commands reach the end; two final slots remain reserved for the closing tag/end.

The existing stage vertex/scratch allocator did not enforce its buffer bounds.
The deliberate 1,000-plane stress case corrupted adjacent game state with the
old allocator. Native allocations now check available space and use aligned
heap spill chunks when necessary. Spill ownership follows the two graphics
buffers: the other frame remains alive until its buffer is reused. Stage reset
also releases spills. Unit ASAN/UBSAN checks cover alignment, adjacent-buffer
canaries, lifetime across a swap and reclamation. This is a real corruption fix,
but no fresh console crash report was available to identify the user's crash.

Boot and runtime share pd_paths.h: the console data root is now
sdmc:/3ds/perfect-engine-launcher and the ROM is pd.z64. Runtime explicitly passes
--rom-file pd.z64, preserving the upstream PC default for desktop tooling.
The package contains perfect-engine-launcher.3dsx and its SMDH under the new
folder. Existing pd.ini and saves can be preserved by renaming the old data
folder before extracting the updated package; remove the old launcher pair.

The title/inventory probe passed 800 mission frames and recorded 19,278 native
top-background vertices plus 270 rotating inventory fits. An 800-frame 4:3
keyboard test loaded pd.z64 from a folder named perfect-engine-launcher and
recorded 19,638 background vertices. An 1,800-frame repeated-transition run
loaded the same filename, passed eight opens/swipes/closes and recorded 64,998
background vertices. The 1,000-plane growth/canary test and ROM bootstrap test
also pass. Hardware appearance and crash resolution still need console testing.


## Confirmed texture-allocation failure

The copied console log for build d7feb45e21a3 ends at 110,966,080 us with
`FATAL: Native PICA: texture allocation failed`. It had DSP dry mixing and
supersampling enabled. This report identifies texture allocation failure rather
than an unknown hang or a graphics scratch failure.

The texture cache previously bounded entry count but not decoded GPU bytes,
and cache clears/removals kept allocations alive until their IDs were reused.
Removed entries now release their GPU storage. Release remains deferred even
between frames: FrameEnd submits work but does not prove the GPU has finished.
Only the next successful FrameBegin/completion point drains retired storage and
permits immediate reclamation while trimming. Texture quality is unchanged.

At that safe point the cache trims least-recently-used entries to a 2 MiB
resident target, reducing the target further when needed to preserve 2 MiB of
free linear memory. Any interpreter bindings that refer to removed entries are
invalidated and reimport on use. Mip levels count toward resident size. The
allocation error now includes requested dimensions and free linear space.

The host mock now enforces a hard 4 MiB allocation limit. A 320-upload streaming
probe without trimming reproduces texture allocation failure. With trimming,
the same probe peaks at 2,621,440 bytes (2 MiB retained plus a 512 KiB new-frame
burst), then verifies that cache clearing defers release until completion and
returns the old allocation space. An ASAN-backed 800-frame keyboard/menu run
and an 1,800-frame gameplay/menu stress run pass; the latter also tests bound
cache entries, eight opens/swipes/closes and the projected background. Console
confirmation is still required for the reported failure.

## Inventory preview and numeric HUD — 2026-11-05

Build `012c1aabafcb`. Bottom inventory preview now measures the complete model display list without GPU submissions, then renders it with a single common fit applied before each submitted batch. This replaces modifying vertices after their draw commands were submitted. The measurement pass adds interpreter work while the inventory is open; gameplay is unaffected. Preview bounds leave space below the manufacturer and weapon name. Bottom inventory label rows are 18 pixels and the description marquee has 20 pixels, matching the larger bottom font rather than clipping it to 12 pixels. Numeric ammo HUD strings use 1.2x glyph geometry with their existing alignment anchors and shadow; ROM font source detail is unchanged.

Validation: ARM engine/renderer compilation, link and 3DSX relocation validation passed. The 800-frame actual-engine PICA host probe passed under AddressSanitizer, including 270 centered rotating previews and 256137 bottom preview vertices. See `build/inventory-hud-pica.log`. This checks geometry submission and memory safety, not hardware raster output; console visual testing remains necessary. Install package: `local/releases/perfect-engine-launcher-private.zip`.

### Console follow-up: missing ammo and preview hitch

The console test reported missing ammo digits and uneven preview rotation in build 012c1aabafcb. Build `92f9057ebfe2` restores the original textRender numeric glyph/palette/shadow path. The alternative triangle-text path was inappropriate for this HUD setup. A scoped rectangle transform now enlarges the original ammo HUD, including gauges, by 25%, anchored to the left/right screen edge and bottom; original font texture detail is unchanged. Both normal and no-ammo exit paths end the scope before other UI draws.

Inventory fitting is now measured once per selected weapon and cached, with extra rotation margin. Subsequent frames render one geometry pass rather than measuring and refitting every frame. The fit diagnostic runs after preview drawing, when the backend viewport has been applied. Build and package relocation validation passed; console validation is still required.

### HUD sampling follow-up

Build `687a8f450317` removes the 25% HUD enlargement at the user's request for sharper output rather than larger glyphs. The HUD scope now forces nearest texture sampling through the existing sampler-state cache. The original glyph path, palette, shadows, gauge sizes and positions remain intact. This removes linear texture interpolation; it does not add detail to ROM font bitmaps or bypass whole-scene postprocessing. Build/link/relocation validation passed.

### Larger HUD with new outline-rasterized fonts

Build `cbbb7aff3cf5` replaces the numeric HUD glyphs with JetBrains Mono rasterized from its bundled outline font at 20px, with a 14px companion for weapon/function labels. The new CI4 glyphs use the existing projected rectangle text path, palette alpha coverage, and point sampling; they are not enlarged ROM font bitmaps. Ammo counts retain their alignment anchors and get a separate projected shadow. Loaded/reserve text is spaced farther apart; ammo gauges widen from 9 to 12 logical pixels. These changes affect the weapon/ammo HUD, not subtitles, crosshair or other menus. Whole-scene image filters remain active if selected. The new lettering differs from the original font.

Font source and SIL OFL licence are in `assets/fonts`; `tools/generate_hud_font.py` regenerates the checked-in glyph header. Normal builds do not require Pillow. The install ZIP includes the font licence. ARM build and 3DSX relocation checks passed. Console visual validation remains required. The host probe now checks nonzero alpha in large HUD glyph textures at GPU submissions, rather than only checking frame completion.

### HUD proportions corrected after console feedback

Build `f840895d5c5f`: reduce numeric outline rasterization from 20px to 14px, matching the weapon-label family/size (12px advance becomes 9px). Retain fresh outline rasterization and coverage alpha. Enlarge the actual player health/shield vertex geometry by 20% around its own local centre, retaining offy and appearance animation; NPC/boss bars are unchanged. Ammo clip/reserve gauges increase from 57/36 to 68/43 logical pixels, with the existing 12px width. Function square is 13px. Loaded count offset reduces to 14px, keeping counts separate at the smaller font size.

ARM/link/package relocation checks passed. The 800-frame gameplay/menu/inventory host check passed under AddressSanitizer, including 48018 larger-font textured vertices with nonzero alpha and 270 rotating inventory previews. Report: `build/balanced-hud-pica.log`. Console appearance remains unverified.

### Compact proportional HUD revision

Build `1ecdb0da3e9c` replaces the fixed-width HUD replacement with Noto Sans Medium rasterized at 12px, with proportional label advances and tabular digits. Both numeric and label paths share the glyph atlas. Glyph line height reduces to 14 pixels, loaded-count offset to 12, and shadow opacity to 0x70. Ammo gauges are 10 pixels wide with clip/reserve heights 63/40, and the function indicator is 12 pixels. Health/shield geometry scales 1.1 around its local centre. This keeps the geometry around 10% above original sizing while removing the previous oversized spacing. Whole-scene postprocessing remains unchanged.

New font source and OFL licence are in assets/fonts; the install ZIP includes LICENSE-NotoSans.txt. The prior font sources are retained for rollback but are unused by the build and not bundled in the install ZIP. Checked actual generated glyph preview in build/compact-hud-font-preview.png: proportional advances, equal digit advances, expected CI4 glyph allocation sizes. ARM build/link/package validation and 800-frame ASAN gameplay/menu/inventory check passed (build/compact-hud-pica.log). These checks do not replace console appearance testing.

### HUD experiments reverted at user request

Build `a4fc3d66eab6` restores bondgun.c byte-for-byte from the backup taken before the HUD changes. Original numeric and label fonts, text renderer, shadows, positions, ammo gauge dimensions and function indicator are restored. Removed health/shield vertex resizing and renderer HUD filtering scope. Removed font-generation inputs and code from active project paths (retained under local/backups/reverted-hud-20261105-025708), removed replacement-font package licences and test instrumentation. The install ZIP returns to six files. Inventory preview/text fixes are separate and retained. ARM build/link and 3DSX relocation validation passed.

### Cutscene framing and selective interpreter optimisation

Build `c0c2c5c59f2f` adds Screen Effects > Cutscene Bars: Original (default), Larger, Off. Saved as 3DS.CutsceneBars. Original takes the existing viewport code unchanged, including its subtitle handling. Larger uses the game's cinema-height framing centred in the full viewport, interpolating to full height with the original cutscene-to-gameplay tween. Off uses full height throughout. Overrides are limited to active single/shared-viewport cutscenes, excluding end screens and the existing special-camera exclusion. Gameplay viewport/HUD remains unchanged.

Performance change: compile only port/fast3d/gfx_pc.cpp at -O2 instead of -Og, keeping the graphics backend's existing -O2 and gameplay/audio engine units at -Og. The old Nov 4 copied console log reports roughly 19.8ms interpreter work and 7–12ms GPU work, but it predates recent fixes and is not a benchmark of this build. No console FPS gain is claimed. This avoids repeating the previous whole-engine -O2 experiment. A fresh console log is needed to measure this targeted change.

Build reliability: rebuild the static archive from all existing objects in a fresh temporary archive. game/player.c and modeldata/player.c share a basename; incremental ar replacement can overwrite the wrong member and lose g_PlayerModeldef. tools/build_engine.py now uses fresh archive construction for full builds.

Validation: ARM build/link and 3DSX relocation validation passed. Runtime policy tests cover persisted option bounds, original/off/larger framing, centring, viewport containment and tween endpoints. The actual-engine -O2 host probe passed 800 frames under ASAN, including CUTSCENE_BARS_PASS through real viewport getters, native top/bottom menus and 270 inventory previews. See build/cutscene-o2-pica.log and build/desktop-audio-menu/run-pica/reference.log. These checks do not establish hardware FPS or rule out hardware-specific regressions.

### Revert graphics -O2 after console rendering regressions

Build `914965b9714f` restores the Fast3D interpreter to -Og. The user reported glitchy native top-screen menu transitions and a solid rectangle replacing the bottom inventory weapon after the selective -O2 build. The compiler-level change is the leading suspect, not a console-confirmed root cause. Keep the backend/platform optimisation levels unchanged. Cutscene Bars and the archive-build fix are retained; HUD changes stay reverted. Future optimisation work should not re-enable interpreter -O2 without investigating the hardware rendering difference.

Host instrumentation now verifies each bottom weapon preview submits more than 12 vertices with non-planar depth, in addition to its viewport and fit bounds. The restored build passed the 1800-frame ASAN transition stress run: eight open/swipe/close cycles, 350 rotating 3D previews, native-menu background/coordinate checks, and the actual cutscene-bar viewport checks. See build/menu-rollback-stress-pica.log. ARM build/link and relocation validation passed. The host renderer does not emulate actual GPU raster output, so console confirmation remains required.
