.PHONY: all setup renderer checks rom-checks host-checks
all: setup
	python3 -u tools/build_engine.py
	python3 -u tools/compile_renderer.py
	python3 -u tools/build_game.py
setup:
	python3 tools/setup_upstream.py
renderer:
	python3 -u tools/compile_renderer.py
checks:
	mkdir -p build
	python3 tools/check_audio_queue.py
rom-checks:
	python3 tools/check_inflate.py
	python3 tools/check_rom_verify.py
# These checks additionally require the optional desktop engine build.
host-checks:
	python3 tools/check_input_mapping.py
	python3 tools/check_video_settings.py
	python3 tools/check_runtime_policy.py
	python3 tools/check_gyro_stereo.py
	python3 tools/check_dsp_mixer.py

updater-check:
	python3 tools/check_updater.py
