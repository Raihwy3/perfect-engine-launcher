#!/usr/bin/env python3
"""Check bottom objective ownership rules and near-camera triangle facing."""
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
flags=shlex.split(row['command']);flags=flags[:flags.index('-o')]
subprocess.run([*flags,'-UNDEBUG','-ffunction-sections','-fdata-sections','-I'+str(root/'include'),'-fsanitize=address,undefined',str(root/'tests/objectives-host/policy_test.c'),'-Wl,--gc-sections','-o',str(out/'objectives-policy-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'objectives-policy-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
subprocess.run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'tests/objectives-host/facing_test.cpp'),'-o',str(out/'triangle-facing-test')],check=True)
subprocess.run([str(out/'triangle-facing-test')],check=True)

# Check the real tiled LCD copy against every coordinate of the reference map.
subprocess.run(['cc','-std=c11','-O2','-fsanitize=address,undefined','-I'+str(root/'include'),str(root/'tests/objectives-host/bitmap_copy_test.c'),'-o',str(out/'bitmap-copy-test')],check=True)
subprocess.run([str(out/'bitmap-copy-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)

subprocess.run(['cc','-std=c11','-O2','-fsanitize=address,undefined','-I'+str(root/'include'),str(root/'tests/objectives-host/menu_frame_copy_test.c'),'-o',str(out/'menu-frame-copy-test')],check=True)
subprocess.run([str(out/'menu-frame-copy-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)

subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'include'),str(root/'tests/objectives-host/cadence_test.c'),'-o',str(out/'bottom-cadence-test')],check=True)
subprocess.run([str(out/'bottom-cadence-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
