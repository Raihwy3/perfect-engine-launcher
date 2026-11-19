#!/usr/bin/env python3
"""Fetch the pinned PC-port source and apply the 3DS changes."""
from pathlib import Path
import argparse, subprocess
ROOT=Path(__file__).resolve().parents[1]
REV="a4b58e6dd14d5842d588e9841efa8bceb8b86fc3"
URL="https://github.com/perfect-dark-pc-port/perfect_dark.git"
def run(*args):
 return subprocess.run([str(a) for a in args],check=True)
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument("--source",default=URL,help="Git URL or local checkout for an offline build")
 args=parser.parse_args();up=ROOT/"upstream/perfect_dark";patch=ROOT/"patches/perfect-dark-3ds.patch"
 if not up.exists():
  up.parent.mkdir(parents=True,exist_ok=True)
  run("git","clone","--no-checkout","--no-hardlinks",args.source,up)
  run("git","-C",up,"checkout","--detach",REV)
 head=subprocess.check_output(["git","-C",str(up),"rev-parse","HEAD"],text=True).strip()
 if head!=REV:
  if subprocess.check_output(["git","-C",str(up),"status","--porcelain"],text=True).strip():
   raise SystemExit("Upstream has local changes; refusing to replace them.")
  run("git","-C",up,"checkout","--detach",REV)
 # A newly cloned --no-checkout tree must be populated even when HEAD is pinned.
 if not (up/"src/include/constants.h").exists():run("git","-C",up,"checkout","--detach",REV)
 check=subprocess.run(["git","-C",str(up),"apply","--check",str(patch)],capture_output=True)
 if check.returncode==0:run("git","-C",up,"apply",patch)
 elif subprocess.run(["git","-C",str(up),"apply","--reverse","--check",str(patch)],capture_output=True).returncode:
  raise SystemExit("Upstream changes conflict with the 3DS patch; no files were overwritten.")
 print("Perfect Dark source ready at",REV)
if __name__=="__main__":main()
