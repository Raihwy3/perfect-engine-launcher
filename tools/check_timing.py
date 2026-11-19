#!/usr/bin/env python3
"""Exercise the actual 3DS timing code with controlled counter stalls."""
from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/'build'
row=json.loads((out/'desktop-audio-menu/compile_commands.json').read_text())[0]
flags=shlex.split(row['command']);flags=flags[:flags.index('-o')]
subprocess.run([*flags,'-UNDEBUG','-ffunction-sections','-fdata-sections','-fsanitize=address,undefined',str(root/'tests/timing-host/policy_test.c'),'-Wl,--gc-sections','-o',str(out/'timing-policy-test')],cwd=out/'desktop-audio-menu',check=True)
subprocess.run([str(out/'timing-policy-test')],env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'},check=True)
