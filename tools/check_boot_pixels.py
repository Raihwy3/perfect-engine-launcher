"""Check startup pixel formats and both rotated LCD sizes without hardware."""
from pathlib import Path
import os, subprocess, tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='pd-boot-pixels-') as tmp:
    binary = Path(tmp) / 'check'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-I'+str(root/'include'),
                    str(root/'tests/boot-pixels-host.c'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0'}, check=True)
