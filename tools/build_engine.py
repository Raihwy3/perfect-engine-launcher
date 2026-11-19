#!/usr/bin/env python3
"""Cross-compile the original PD source list into a private ARM bring-up archive."""
from pathlib import Path
import concurrent.futures,hashlib,json,os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];UP=ROOT/'upstream/perfect_dark';BUILD=ROOT/'build';CC=Path('/opt/devkitpro/devkitARM/bin/arm-none-eabi-gcc')
REV='a4b58e6dd14d5842d588e9841efa8bceb8b86fc3'
OPTIMIZATION=os.environ.get('PD_ENGINE_OPT','-O2')
if OPTIMIZATION not in ('-Og','-O2'):raise ValueError('PD_ENGINE_OPT must be -Og or -O2')
ARCH=['-march=armv6k','-mtune=mpcore','-mfloat-abi=hard','-mtp=soft']
def run(args,**kwargs):
 p=subprocess.run([str(a) for a in args],text=True,capture_output=True,**kwargs)
 if p.returncode:raise RuntimeError(' '.join(str(a) for a in args[:4])+'\n'+p.stdout+p.stderr)
 return p.stdout

def prepare():
 assert run(['git','-C',UP,'rev-parse','HEAD']).strip()==REV
 inc=BUILD/'include';inc.mkdir(parents=True,exist_ok=True)
 platform=(UP/'src/include/platform.h').read_text().replace('#if defined(_WIN32)','#if defined(__3DS__)\n\t#define PLATFORM_3DS 1\n\t#define PLATFORM_POSIX 1\n#elif defined(_WIN32)',1).replace('#define PLATFORM_ARM 7','#define PLATFORM_ARM 6')
 (inc/'platform.h').write_text(platform)
 version=(UP/'port/include/versioninfo.h.in').read_text()
 for key,val in {'TARGET':'armv6k-3ds','ARCH':'armv6k','ROMID':'ntsc-final','BUILD':'bringup','HASH':REV[:12],'BRANCH':'3ds-defection'}.items():version=version.replace('@VERSION_'+key+'@',val)
 (inc/'versioninfo.h').write_text(version)
 (inc/'prelude.h').write_text('#include "platform.h"\n#include <stddef.h>\n#include <stdint.h>\n#ifndef _WINT_T\n#define _WINT_T\ntypedef __WINT_TYPE__ wint_t;\n#endif\n')
 env={**os.environ,'ROMID':'ntsc-final'}
 assets=UP/'src/assets/ntsc-final'
 for folder,tool,extra in [('lang','mklang',['en']),('pads','mkpads',[]),('tiles','mktiles',[])]:
  for source in sorted((assets/folder).glob('*.json')):run([sys.executable,UP/'tools/assetmgr'/tool,source,*extra,'--headers-only'],cwd=BUILD,env=env)
 for name,tool in [('animations','mkanims'),('sequences','mksequences'),('textures','mktextures')]:run([sys.executable,UP/'tools/assetmgr'/tool,assets/(name+'.json'),'--headers-only'],cwd=BUILD,env=env)

def flags(cpp=False):
 includes=[BUILD/'include',ROOT/'include',UP/'include',UP/'include/PR',UP/'src/include',BUILD/'src/generated/ntsc-final',UP/'src/lib/ultra/audio',UP/'port/include',UP/'port/fast3d',Path('/opt/devkitpro/libctru/include'),Path('/opt/devkitpro/portlibs/3ds/include')]
 return [*ARCH,'-std=gnu++20' if cpp else '-std=gnu11',OPTIMIZATION,'-fno-strict-aliasing','-funsigned-char','-fwrapv','-fno-stack-protector','-ffunction-sections','-fdata-sections','-w',*(['-fno-exceptions','-fno-rtti'] if cpp else ['-fpermissive','-fplan9-extensions']),'-D__3DS__','-DMATCHING=0','-DPIRACYCHECKS=0','-DROM_SIZE=32','-DAVOID_UB=1','-D_LANGUAGE_C=1','-DPAL=0','-DVERSION=2','-DosGetTime=pd_osGetTime','-DgfxSwapBuffers=pd_gfxSwapBuffers','-DfsInit=pd_fsInit',*['-I'+str(p) for p in includes],'-include',BUILD/'include/prelude.h','-include','versioninfo.h']

def sources():
 cmake=(UP/'CMakeLists.txt').read_text();block=cmake.split('set(SRC_LIB\n',1)[1].split('\n)',1)[0]
 files=[*UP.glob('src/game/**/*.c'),*UP.glob('src/lib/mp3/*.c'),*UP.glob('src/lib/naudio/*.c'),*UP.glob('src/lib/ultra/audio/*.c'),*UP.glob('src/lib/ultra/gu/*.c'),*[UP/p for p in re.findall(r'^\s*(src/\S+\.c)',block,re.M)]]
 excluded={'audio.c','input.c','system.c','video.c','crash.c'}
 files += [p for p in UP.glob('port/src/**/*.c') if not (p.parent==UP/'port/src' and p.name in excluded)]
 files += [UP/'port/fast3d/gfx_pc.cpp',UP/'port/fast3d/gfx_cc.cpp']
 return sorted(set(files))

def compile_one(source):
 rel=source.relative_to(UP);obj=BUILD/'engine'/rel.with_suffix('.o');obj.parent.mkdir(parents=True,exist_ok=True)
 cpp=source.suffix=='.cpp';args=flags(cpp)
 if rel==Path('port/src/main.c'):args+=['-Dmain=pd_engine_main']
 run([CC.with_name('arm-none-eabi-g++') if cpp else CC,*args,'-c',source,'-o',obj])
 return obj

def archive_objects(objects):
 # Duplicate basenames (game/player.c and modeldata/player.c) must coexist.
 # Updating an existing archive by basename can replace the wrong member.
 with tempfile.TemporaryDirectory(dir=BUILD,prefix='engine-archive-') as temp:
  archive=Path(temp)/'libperfect-dark.a'
  run([CC.with_name('arm-none-eabi-ar'),'rcs',archive,*sorted(objects)])
  os.replace(archive,BUILD/'libperfect-dark.a')

def main():
 prepare();files=sources();print('Compiling',len(files),'original Perfect Dark units for ARM',flush=True)
 errors=[];objects=[]
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
  jobs={pool.submit(compile_one,p):p for p in files}
  for future in concurrent.futures.as_completed(jobs):
   try:objects.append(future.result())
   except Exception as e:errors.append({'source':str(jobs[future].relative_to(UP)),'error':str(e)})
 report={'revision':REV,'optimization':OPTIMIZATION,'total':len(files),'compiled':len(objects),'errors':errors,'game_linked':False,'hardware_tested':False}
 (BUILD/'engine-compile.json').write_text(json.dumps(report,indent=2)+'\n')
 print('Compiled:',len(objects),'Failed:',len(errors),flush=True)
 for error in errors[:12]:print(error['source'],error['error'][:2200],flush=True)
 if errors:return 1
 archive_objects(objects)
 print('Original-engine ARM archive:',BUILD/'libperfect-dark.a',flush=True);return 0
if __name__=='__main__':sys.exit(main())
