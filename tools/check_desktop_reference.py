from pathlib import Path
import json,shlex,subprocess,os
root=Path(__file__).resolve().parents[1];out=root/os.environ.get('PD_HOST_BUILD','build/desktop-reference')
row=json.loads((out/'compile_commands.json').read_text())[0];args=shlex.split(row['command']);oi=args.index('-o');args[oi+1]=str(out/'frame_probe.o');args[-1]=str(root/'tests/pica-host/frame_probe.c');subprocess.run(args,cwd=out,check=True)
p=out/'CMakeFiles/pd.dir/link.txt';s=p.read_text().strip()
if 'frame_probe.o' not in s:p.write_text(s+' '+str(out/'frame_probe.o')+' -Wl,--wrap=videoEndFrame\n')
with (out/'probe-link.log').open('w') as log:subprocess.run(['cmake','--build',str(out),'-j4'],check=True,stdout=log,stderr=subprocess.STDOUT)
run=out/'run-probed';run.mkdir(exist_ok=True)
with (run/'reference.log').open('w') as log:
 result=subprocess.run([str(out/'pd.x86_64'),'--boot-stage','0x30','--no-sound','--no-crash-handler','--basedir',str(root/'local/roms'),'--savedir','.', '--log'],cwd=run,env={**os.environ,'SDL_AUDIODRIVER':'dummy','SDL_VIDEODRIVER':'offscreen','LIBGL_ALWAYS_SOFTWARE':'1','EGL_PLATFORM':'surfaceless'},stdout=log,stderr=subprocess.STDOUT,timeout=45)
 print('Reference exit:',result.returncode)
 print((run/'reference.log').read_text()[-2500:])
 assert result.returncode==0 and 'DEFECTION_PRESENTED frames=120' in (run/'reference.log').read_text()
