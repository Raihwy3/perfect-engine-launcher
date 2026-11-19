#!/usr/bin/env python3
"""Record the real Perfect Dark checkout and direct-mission boot boundary."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
UP=ROOT/'upstream/perfect_dark'
URL='https://github.com/perfect-dark-pc-port/perfect_dark.git'
REQUIRED=['CMakeLists.txt','port/src/main.c','port/src/romdata.c','port/src/libultra.c','port/fast3d/gfx_pc.cpp','src/lib/main.c','src/include/constants.h']
def main():
    missing=[name for name in REQUIRED if not (UP/name).is_file()]
    if missing:
        print('Perfect Dark source checkout is missing or incomplete.')
        print(f'git clone --depth 1 --branch port {URL} {UP}')
        return 2
    revision=subprocess.check_output(['git','-C',str(UP),'rev-parse','HEAD'],text=True).strip()
    constants=(UP/'src/include/constants.h').read_text()
    match=re.search(r'^#define\s+STAGE_DEFECTION\s+(0x[0-9a-fA-F]+|\d+)\b',constants,re.M)
    if not match:raise RuntimeError('Defection stage constant not found; inspect checkout rather than assuming an ID.')
    main_source=(UP/'port/src/main.c').read_text()
    if '"--boot-stage"' not in main_source:raise RuntimeError('Original direct-stage boot option changed.')
    rom=ROOT/'local/roms/pd.ntsc-final.z64'
    if hashlib.md5(rom.read_bytes()).hexdigest()!='e03b088b6ac9e0080440efed07c1e40f':raise RuntimeError('Expected US Rev 1 development ROM.')
    manifest={'upstream_url':URL,'revision':revision,'stage_symbol':'STAGE_DEFECTION','stage_id':int(match[1],0),'boot_option':'--boot-stage','required_source_hashes':{name:hashlib.sha256((UP/name).read_bytes()).hexdigest() for name in REQUIRED},'game_linked':False,'hardware_tested':False}
    (ROOT/'local/upstream-inspection.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2));return 0
if __name__=='__main__':sys.exit(main())
