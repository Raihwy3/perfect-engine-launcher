# New 3DS transfer pass

The development folder is now `perfect-engine-launcher`. SD files remain in
`/3ds/perfect-engine-launcher/`: use `pd.z64` and keep `pd.ini` and saves.

The complete original engine now builds at `-O2`. Set `PD_ENGINE_OPT=-Og` to
produce a diagnostic comparison. Neither flag establishes a console FPS gain.
The renderer adapts Eyes of Gold revision 41485d9371397b0faf155b09ea4015fef8f2a58a:
a larger memory-aware GPU command buffer and capacity guards, gyro gravity/filter
updates, flick-stick refinements, adjustable convergence and HUD/reticle depth,
and optional ghost reduction. The Perfect Dark renderer and menu paths are kept.

3DS Options groups Controls, Crosshair, 3D Settings, Performance, Screen Effects,
Audio, Accessibility and Experimental. Existing config keys keep their meaning;
new keys are appended. Vertical look inherits the existing horizontal setting
until explicitly changed. Crosshair Original retains PD's sights, colour and
alpha. Custom silhouettes replace the standard/classic aimer, retaining weapon
special sights and target tracking. Colour and opacity can be selected separately.
Aiming still uses PD's native free-reticle path. Flick stick no longer consumes
the C-stick while aiming. All new visual controls default to the old appearance.

The bounded texture cache can retain up to 4 MiB, subject to a 3 MiB free-linear
reserve. Guard/script optimisations retain their existing eligibility rules;
this pass does not freeze mission-critical actors or delete rooms behind fog.
Bottom-screen objectives retain the 10 Hz content cadence and full-rate input.

Audio worker waits now report a fatal timeout after 2.5 seconds rather than wait
forever. An unresponsive worker's buffers are retained until process exit;
shutdown does not free memory it could still access. DSP voice slots are reusable across changing/stage-local voice identities rather
than permanently falling back after the first 23 identities. Simultaneous voice
overflow still uses the original CPU mixer. Audio queue diagnostics
report queued frames, underruns and dropped blocks. The original CPU/NDSP mixer
selection is retained; no GoldenEye scheduler acknowledgement is copied into PD,
whose audio manager uses a different frame pipeline. OpenDSP is not integrated.

This is a private New 3DS/2DS test build. Engine compile, sanitizer checks and
mock-GPU mission probes cannot establish hardware stability, visible pixel
correctness, DSP playback quality, or sustained frame rate. Old 3DS support and
a public distribution-content audit remain separate work. No ROM is packaged.
