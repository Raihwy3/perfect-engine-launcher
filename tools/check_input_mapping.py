#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
args=shlex.split(row['command']);idx=args.index('-o');args=args[:idx]
subprocess.run([*args,'-I'+str(root/'include'),'-UNDEBUG','-fsanitize=address,undefined',str(root/'src/input_3ds.c'),str(root/'tests/input-host/mapping_test.c'),'-lm','-o',str(out/'input-mapping-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'input-mapping-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
