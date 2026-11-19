#!/usr/bin/env python3
"""Exercise the target PCM queue with a mock DSP and memory sanitizers."""
from pathlib import Path
import os,subprocess
root=Path(__file__).resolve().parents[1]
out=root/'build/audio-queue-test'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'tests/audio-host'),'-I'+str(root/'include'),str(root/'src/audio_3ds.c'),str(root/'src/native_dsp_mix.c'),str(root/'tests/audio-host/queue_test.c'),'-Wl,--wrap=calloc','-o',str(out)],check=True)
for case in [[],['--fail-linear'],['--fail-sidecar']]:
 subprocess.run([str(out),*case],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
