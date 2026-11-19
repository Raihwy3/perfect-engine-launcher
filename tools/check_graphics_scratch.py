#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
flags=shlex.split(row['command']);flags=flags[:flags.index('-o')]
subprocess.run([*flags,'-UNDEBUG','-D__3DS__','-ffunction-sections','-fdata-sections','-I'+str(root/'include'),'-fsanitize=address,undefined',str(root/'tests/gfx-host/scratch_test.c'),'-Wl,--gc-sections','-o',str(out/'graphics-scratch-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'graphics-scratch-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
