#!/usr/bin/env python3
"""Compile the independent, known PICA backend; this does not build the game."""
from pathlib import Path
import json,subprocess
ROOT=Path(__file__).resolve().parents[1]
VENDOR=ROOT/'vendor/eog-pica'
OUT=ROOT/'build/renderer'
CC=Path('/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc')
def run(args):
    p=subprocess.run([str(a) for a in args],text=True,capture_output=True)
    if p.returncode:raise RuntimeError(p.stdout+p.stderr)
    return p.stdout

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    for name in ['native_clip','native_world']:
        shader=OUT/(name+'.shbin')
        run(['/opt/devkitpro/tools/bin/picasso','-o',shader,VENDOR/'src'/(name+'.v.pica')])
        data=shader.read_bytes()
        (OUT/(name+'_shbin.h')).write_text('#pragma once\nstatic const unsigned char '+name+'_shbin[] __attribute__((aligned(4)))={'+','.join(map(str,data))+'};\nstatic const unsigned '+name+'_shbin_size=sizeof '+name+'_shbin;\n')
    objects=[]
    arch=['-march=armv6k','-mtune=mpcore','-mfloat-abi=hard','-mtp=soft']
    for name in ['gfx_pica.cpp','pica_inputs.cpp','gfx_cc.cpp','display_3ds.c']:
        cpp=name.endswith('.cpp');obj=OUT/(name+'.o')
        flags=[*arch,'-std=gnu++20' if cpp else '-std=gnu11','-O2','-Wall','-Wextra','-Werror','-D__3DS__','-ffunction-sections','-fdata-sections',
               '-I'+str(VENDOR/'include'),'-I'+str(OUT),'-I/opt/devkitpro/libctru/include']
        if cpp:flags+=['-fno-rtti','-fno-exceptions']
        source=ROOT/'vendor/adapters/gfx_pica.cpp' if name=='gfx_pica.cpp' else VENDOR/'src'/name
        run([CC.with_name('arm-none-eabi-g++') if cpp else CC,*flags,'-c',source,'-o',obj])
        objects.append(obj)
        print('Compiled',name,flush=True)
    undefined=run([CC.with_name('arm-none-eabi-nm'),'-u',*objects])
    (OUT/'undefined-symbols.txt').write_text(undefined)
    (OUT/'validation.json').write_text(json.dumps({'renderer_arm_compile':'passed','objects':[str(p.relative_to(ROOT)) for p in objects],'game_linked':False,'defection_booted':False},indent=2)+'\n')
    print('PASS independent PICA renderer compilation; Perfect Dark engine not linked.')
if __name__=='__main__':main()
