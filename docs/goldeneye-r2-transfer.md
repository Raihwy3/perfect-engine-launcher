# Shared motion and stereo transfer

Adapted from Eyes of Gold commit c588df4950b4373a064642caae9a5a36e5ad45cc.
Perfect Dark retains its own branding, ROM paths, configuration, menus and renderer integration.

3DS Options → Controls:
- Look, gyro and touch sensitivity, with a 400% maximum and existing saved gains preserved.
- Gyro Off/Aim Only/Look + Aim; Local/World/Player spaces; exclusive local yaw or roll.
- Stillness calibration, using GamepadMotionHelpers, with progress and a bounded timeout.
- Experimental C-stick flick stick, adapting JoyShockMapper's 100ms turn and rotation smoothing.
- Free/Classic aim movement: Classic replaces movement while aiming with lean and crouch, or manual zoom where the weapon supports it.

3DS Options → Accessibility:
- Aim lock: optional fixed reticle and camera aiming.
- Stamina sway: original behavior by default; optional suppression affects visual breathing, not stamina state.
- Steady scope: optional suppression of head animation in zoomed aiming. Off by default to retain PD's existing behavior.

3DS Options → Experimental:
- Saved 3D strength, 0–200%, default 100%, multiplied by the physical slider.

The renderer shares the 0.03 separation and 3000 camera-W convergence ceiling, reference convergence controller, robust nearest-depth statistic and aim-depth sampler. Sky and reticle scopes are tagged in PD's actual display list. Perspective coefficients come from the same list; a queued copy captures scene depth before first-person weapons. The next normal frame wait completes the copy. Menus retain convergence history and do not supply scene depth. Allocation failure retains fixed convergence. The bottom-screen menu, inventory fitting, introductory flat layers and PD-specific material work are preserved.

Touch dragging uses relative free-reticle input instead of saturating the stick. Menu/death/pause/cutscene gating discards gameplay motion; Circle Pad movement and ordinary menu navigation remain independent.

No GoldenEye watch artwork, themes, cheats, game logic, updater/version identity or ROM content is transferred. These are game-specific features, not shared input/stereo code. No source or release was published for this PD work.

Licences and acknowledgements are in distribution/CREDITS.txt and distribution/licenses/. Host checks exercise input/movement, touch aim with gyro off, locked/free aim, Classic lean/zoom, sway/steady scope, the motion backend, reference-equivalent stereo/flick math and title/gameplay/inventory rendering. They do not establish console performance or comfort.
