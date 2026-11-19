#!/usr/bin/env python3
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
args=shlex.split(row['command']);args=args[:args.index('-o')]
subprocess.run([*args,'-UNDEBUG','-ffunction-sections','-fdata-sections','-I'+str(root/'include'),'-fsanitize=address,undefined',str(root/'tests/runtime-host/policy_test.c'),'-Wl,--gc-sections','-lm','-o',str(out/'runtime-policy-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'runtime-policy-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
