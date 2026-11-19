"""Exercise PD's real stage flow through the mock-GPU backend, not a console."""
from pathlib import Path
import argparse,json,os,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--existing-probe',action='store_true',help='Use an already compiled, current instrumented probe')
a=p.parse_args()
out=ROOT/'build';binary=out/'desktop-audio-menu/pd-pica-probe'
if not a.existing_probe:
 env={**os.environ,'PD_HOST_BUILD':'build/desktop-audio-menu','PD_PERFORMANCE_PROBE':'1','PD_PROBE_GAMEPLAY':'1','PD_PROBE_UI':'1','PD_PROBE_INVENTORY':'1','PD_PROBE_CUSTOM':'1','PD_PROBE_GYRO':'1','PD_PROBE_FRAMES':'800'}
 with (out/'transfer-pica-final.log').open('w') as log:subprocess.run([sys.executable,ROOT/'tools/check_pica_host.py'],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
else:
 source=[p for directory in ['src','include','vendor','tests','upstream/perfect_dark/src','upstream/perfect_dark/port'] for p in (ROOT/directory).rglob('*') if p.suffix in ('.c','.cpp','.h','.pica')]
 if not binary.exists() or any(p.stat().st_mtime_ns>binary.stat().st_mtime_ns for p in source):raise SystemExit('Probe is missing or stale; run without --existing-probe.')
cases=[
 ('title-audio','0x5a',{'PD_PROBE_TITLE':'1'},True),
 ('investigation','0x33',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_GYRO':'1','PD_PROBE_CUSTOM':'1','PD_PROBE_FRAMES':'700'},False),
 ('extraction','0x22',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_GYRO':'1','PD_PROBE_CUSTOM':'1','PD_PROBE_FRAMES':'700'},False),
 ('institute','0x26',{'PD_PROBE_FRAMES':'400'},False),
 ('briefing-keyboard','0x30',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_UI':'1','PD_PROBE_BRIEFING':'1','PD_PROBE_KEYBOARD':'1','PD_PROBE_FRAMES':'800'},False),
 ('menu-stress','0x30',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_UI':'1','PD_PROBE_MENU_STRESS':'1','PD_PROBE_FRAMES':'1500'},False),
 ('results-success','0x30',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_RESULTS':'success','PD_PROBE_FRAMES':'1100'},False),
 ('results-failed','0x30',{'PD_PROBE_GAMEPLAY':'1','PD_PROBE_RESULTS':'failed','PD_PROBE_FRAMES':'1100'},False),
]
results=[];run=out/'transfer-missions'/str(time.time_ns());run.mkdir(parents=True)
for name,stage,flags,audio in cases:
 folder=run/name;folder.mkdir()
 env={k:v for k,v in os.environ.items() if not k.startswith('PD_PROBE_')};env.update(flags)
 env.update(PD_PROBE_STAGE=stage,ASAN_OPTIONS='detect_leaks=0',SDL_AUDIODRIVER='dummy',SDL_VIDEODRIVER='offscreen',LIBGL_ALWAYS_SOFTWARE='1',EGL_PLATFORM='surfaceless')
 start=time.monotonic()
 with (folder/'probe.log').open('w') as log:
  try:code=subprocess.run([binary,'--boot-stage',stage,*([] if audio else ['--no-sound']),'--no-crash-handler','--basedir',ROOT/'local/roms','--savedir','.','--log'],cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=100).returncode
  except subprocess.TimeoutExpired:code='timeout'
 text=(folder/'probe.log').read_text();passed=code==0 and ('TITLE_MENU_PRESENTED' if audio else 'DEFECTION_PRESENTED frames=') in text
 results.append(dict(name=name,stage=stage,passed=passed,exit=code,seconds=round(time.monotonic()-start,2),audio=audio,log=str((folder/'probe.log').relative_to(ROOT))))
 print(results[-1],flush=True)
 (out/'transfer-mission-matrix.json').write_text(json.dumps(dict(hardware_tested=False,mock_gpu=True,cases=results),indent=2)+'\n')
 if not passed:print(text[-4500:],flush=True)
if not all(x['passed'] for x in results):raise SystemExit('Mission probe failures; inspect transfer-mission-matrix.json')
