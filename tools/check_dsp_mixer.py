#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
args=shlex.split(row['command']);args=args[:args.index('-o')]
subprocess.run([*args,'-D__3DS__','-UNDEBUG','-I'+str(root/'include'),'-fsanitize=address,undefined',str(root/'tests/dsp-host/mixer_test.c'),str(root/'src/native_dsp_mix.c'),'-lm','-o',str(out/'dsp-mixer-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'dsp-mixer-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)

subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+str(root/'include'),root/'tests/dsp-host/voice_reuse_test.c',root/'src/native_dsp_mix.c','-o',out/'dsp-voice-reuse-test'],check=True)
subprocess.run([out/'dsp-voice-reuse-test'],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
