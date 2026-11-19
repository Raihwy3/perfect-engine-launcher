#!/usr/bin/env python3
"""Link a private original-engine title/menu diagnostic against 3DS services."""
from pathlib import Path
import hashlib,json,shutil,sys,zipfile
import build_engine as build
import updater_deps
from prepare_inflate import generate
from package_elf import prepare_package_elf,validate_3dsx
ROOT=build.ROOT;OUT=build.BUILD/'game'
VERSION=(ROOT/'include/native_version.h').read_text().split('#define PD_VERSION \"',1)[1].split('\"',1)[0]
def main():
 build.prepare();OUT.mkdir(parents=True,exist_ok=True);inflate=generate()
 digest=hashlib.sha256(build.REV.encode())
 digest.update((ROOT/'icon.png').read_bytes())
 upstream_diff=build.run(['git','-C',build.UP,'diff','--binary'])
 digest.update(upstream_diff.encode())
 for folder in ['src','include','tools','vendor']:
  for path in sorted((ROOT/folder).rglob('*')):
   if path.suffix in ('.c','.cpp','.h','.py','.pica'):digest.update(str(path.relative_to(ROOT)).encode());digest.update(path.read_bytes())
 bid=digest.hexdigest()[:12]
 (build.BUILD/'include/pd_build.h').write_text('#define PD_BUILD_ID "'+bid+'"\n')
 updater_flags=updater_deps.prepare()
 objects=[]
 for source in [*sorted((ROOT/'src').glob('*.c')),*sorted((ROOT/'src').glob('*.cpp')),inflate/'inflate.c']:
  cpp=source.suffix=='.cpp';obj=OUT/(source.name+'.o')
  if source.name in ('platform_3ds.c','audio_3ds.c') or source.name.startswith('native_update'):
   flags=[*build.ARCH,'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-D__3DS__','-I/opt/devkitpro/libctru/include','-I'+str(ROOT/'include'),'-I'+str(build.BUILD/'include'),'-I'+str(ROOT/'vendor/eog-pica/include')]
  else:
   flags=[*build.flags(cpp),'-I'+str(ROOT/'vendor/eog-pica/include'),'-I'+str(inflate)]
  if source.name.startswith("native_update"):flags+=updater_flags
  build.run([build.CC.with_name('arm-none-eabi-g++') if cpp else build.CC,*flags,'-c',source,'-o',obj]);objects.append(obj);print('Compiled',source.name,flush=True)
 objects += [ROOT/'build/renderer'/name for name in ['gfx_pica.cpp.o','pica_inputs.cpp.o','display_3ds.c.o']]
 target=build.BUILD/'perfect-engine-launcher'
 build.run([build.CC.with_name('arm-none-eabi-g++'),*build.ARCH,'-specs=3dsx.specs','-L/opt/devkitpro/libctru/lib','-Wl,--gc-sections,-Map,'+str(target.with_suffix('.map')),*objects,'-Wl,--start-group',build.BUILD/'libperfect-dark.a','-Wl,--end-group','-lcitro3d',*updater_deps.libraries(),'-lctru','-lm','-o',target.with_suffix('.elf')])
 print('Linked original Perfect Dark engine:',target.with_suffix('.elf'),flush=True)
 build.run(['/opt/devkitpro/tools/bin/smdhtool','--create','Perfect Engine Launcher','Native Perfect Dark port - '+VERSION,'Raihwy3 and contributors',ROOT/'icon.png',target.with_suffix('.smdh')])
 tokens=prepare_package_elf(target.with_suffix('.elf'),target.with_suffix('.package.elf'))
 # Keep SMDH external: the embedded-icon variant failed during startup on hardware.
 build.run(['/opt/devkitpro/tools/bin/3dsxtool',target.with_suffix('.package.elf'),target.with_suffix('.3dsx')])
 print('Validated relocations:',validate_3dsx(target.with_suffix('.3dsx')),'fixed tokens:',len(tokens),flush=True)
 sd=ROOT/'local/sd/3ds/perfect-engine-launcher'
 sd.mkdir(parents=True,exist_ok=True)
 for suffix in ['.3dsx','.smdh']:shutil.copy2(target.with_suffix(suffix),sd/('perfect-engine-launcher'+suffix))
 shutil.copy2(ROOT/'distribution/README.txt',sd/'README.txt')
 shutil.copy2(build.UP/'LICENSE',sd/'upstream-LICENSE.txt')
 for source in (ROOT/'vendor/eog-pica').glob('LICENSE*.txt'):shutil.copy2(source,sd/source.name)
 (ROOT/'local/releases').mkdir(parents=True,exist_ok=True)
 archive=ROOT/'local/releases/perfect-engine-launcher.zip'
 names=['perfect-engine-launcher.3dsx','perfect-engine-launcher.smdh','README.txt','upstream-LICENSE.txt','LICENSE-native.txt','LICENSE-fast3d.txt']
 for source in (ROOT/'distribution').rglob('*'):
  if source.is_file():
   name=source.relative_to(ROOT/'distribution');(sd/name).parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,sd/name);
   if str(name) not in names:names.append(str(name))
 with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
  for name in names:z.write(sd/name,'3ds/perfect-engine-launcher/'+name)
 (build.BUILD/'package-validation.json').write_text(json.dumps({'build':bid,'version':VERSION,'upstream':build.REV,'boot_stage':0x5a,'boot_target':'title-frontend','engine_arm_units':360,'hardware_tested':False,'audio_enabled':True,'new_models_only':True,'optimization':build.OPTIMIZATION,'zip_files':names,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest()},indent=2)+'\n')
 shutil.copy2(archive,archive.with_name('perfect-engine-launcher-private.zip'))
 print('Install package:',archive,'build',bid,flush=True)
if __name__=='__main__':main()