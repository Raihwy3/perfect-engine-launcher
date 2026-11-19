#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
args=shlex.split(row['command']);idx=args.index('-o');args=args[1:idx];args=[x for x in args if not x.startswith('-std=') and not x.startswith('-Wno-')]
subprocess.run(['g++',*args,'-std=c++20','-UNDEBUG','-ffunction-sections','-fdata-sections','-I'+str(root/'vendor/eog-pica/include'),'-I'+str(root/'include'),'-I'+str(root/'upstream/perfect_dark/port/fast3d'),str(root/'tests/video-host/settings_test.cpp'),'-Wl,--gc-sections','-o',str(out/'video-settings-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'video-settings-test')],check=True)
