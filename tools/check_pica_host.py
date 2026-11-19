from pathlib import Path
import subprocess,shlex,os,json
root=Path(__file__).resolve().parents[1];out=root/os.environ.get('PD_HOST_BUILD','build/desktop-reference');probe=root/'tests/pica-host';obj=out/'pica_probe.o'
args=['g++','-std=c++20','-O1','-g','-fsanitize=address','--param=asan-globals=0','-fno-pie','-ffunction-sections','-fdata-sections','-I'+str(probe/'mock'),'-I'+str(root/'vendor/eog-pica/include'),'-I'+str(root/'upstream/perfect_dark/port/fast3d'),'-I/opt/devkitpro/libctru/include','-c',str(probe/'backend_probe.cpp'),'-o',str(obj)]
subprocess.run(args,check=True)
planner=out/'pica_planner.o';subprocess.run(['g++','-std=c++20','-O1','-g','-fsanitize=address','--param=asan-globals=0','-fno-pie','-ffunction-sections','-fdata-sections','-I'+str(root/'vendor/eog-pica/include'),'-c',str(root/'vendor/eog-pica/src/pica_inputs.cpp'),'-o',str(planner)],check=True)
cmd=shlex.split((out/'CMakeFiles/pd.dir/link.txt').read_text());oi=cmd.index('-o');cmd[oi+1]=str(out/'pd-pica-probe')
if os.environ.get('PD_PERFORMANCE_PROBE'):
 rows=json.loads((out/'compile_commands.json').read_text())
 changed=['src/game/mainmenu.c','src/game/gamefile.c','port/src/libultra.c','port/src/config.c','src/game/title.c','src/game/bondmove.c','src/game/bondhead.c','port/src/optionsmenu.c','src/game/healthbar.c','src/game/bondgun.c','src/game/menu.c','src/game/sight.c','src/game/gfxmemory.c','src/game/savebuffer.c','port/fast3d/gfx_pc.cpp','port/src/optionsmenu.c','port/src/pdmain.c','port/src/pdsched.c','port/src/mixer.c','src/game/chraction.c','src/lib/vi.c','src/game/smoke.c','src/game/sparks.c','src/game/shards.c','src/game/propobj.c','src/game/lv.c','src/game/player.c','src/game/menuitem.c','src/game/game_1531a0.c','src/game/menugfx.c','src/game/timing.c','src/lib/audiomgr.c','src/lib/naudio/n_synthesizer.c']
 overlays=out/'performance-objects';overlays.mkdir(exist_ok=True)
 wrapper=out/'frame_probe.o'
 row=rows[0];flags=shlex.split(row['command']);oi=flags.index('-o');flags[oi+1]=str(wrapper);flags[-1]=str(root/'tests/pica-host/frame_probe.c');flags+=['-D__3DS__','-I'+str(root/'include')];subprocess.run(flags,cwd=out,check=True)
 cmd += [str(wrapper),'-Wl,--wrap=videoEndFrame']

 def compile_overlay(row,source,target):
  flags=shlex.split(row['command']);oi=flags.index('-o');old=flags[oi+1];flags[oi+1]=str(target);flags[-1]=str(source)
  if source.name=='viewmodel-gfx-probe.cpp':
   flags=[x for x in flags if not x.startswith('-O')]+[os.environ.get('PD_PROBE_RSP_OPT','-O2')]
  flags=[x for x in flags if not x.startswith('-O')]+[os.environ.get('PD_ENGINE_OPT','-O2')]
  flags += ['-D__3DS__','-I'+str(root/'include'),'-I'+str(root/'upstream/perfect_dark/port/fast3d'),'-fsanitize=address','--param=asan-globals=0','-ffunction-sections','-fdata-sections']
  if source.name=='objectives_3ds.c':flags+=['-fsanitize=bounds']
  subprocess.run(flags,cwd=out,check=True)
  return old
 for name in changed:
  row=next(r for r in rows if r['file'].endswith('/'+name));target=overlays/(name.replace('/','_')+'.o')
  source=root/'upstream/perfect_dark'/name
  if name=='port/fast3d/gfx_pc.cpp':
   instrumented=overlays/'viewmodel-gfx-probe.cpp'
   text=source.read_text();entry='static void gfx_sp_tri1(uint8_t vtx1_idx, uint8_t vtx2_idx, uint8_t vtx3_idx, bool is_rect) {'
   assert text.count(entry)==1
   text='extern "C" void pd_probe_viewmodel_triangle(void);\n'+text.replace(entry,entry+'\n    if(pd_viewmodel_scope)pd_probe_viewmodel_triangle();')
   text='extern "C" void pd_probe_top_native_scope(bool,unsigned);\nextern "C" void pd_probe_top_native_rect(float,float,float,float,float,float,float,float);\n'+text
   text='extern \"C\" void pd_probe_background_tag(unsigned);\n'+text
   text=text.replace('native_pica_menu_background(cmd->words.w1&1);','native_pica_menu_background(cmd->words.w1&1);pd_probe_background_tag((uint32_t)cmd->words.w1);')
   text=text.replace('static void pd_top_menu_begin(bool enable,unsigned width) {','static void pd_top_menu_begin(bool enable,unsigned width) {\n    pd_probe_top_native_scope(enable,width);')
   text=text.replace('    ul->x = ulxf;','    pd_probe_top_native_rect(ulx/4.f,uly/4.f,lrx/4.f,lry/4.f,(ulxf+1)*SCREEN_WIDTH/2.f,(1-ulyf)*SCREEN_HEIGHT/2.f,(lrxf+1)*SCREEN_WIDTH/2.f,(1-lryf)*SCREEN_HEIGHT/2.f);\n    ul->x = ulxf;')
   text += r'''
extern "C" void pd_probe_texture_memory_frame(int);
extern "C" void pd_probe_texture_memory_limit(unsigned);
extern "C" void pd_probe_texture_memory_test(void){
 static uint8_t keys[320][4],rgba[128*128*4];
 gfx_texture_cache_clear();pd_probe_texture_memory_limit(4*1024*1024);
 unsigned peak=0;
 for(unsigned frame=0;frame<40;frame++){
  pd_probe_texture_memory_frame(1);
  for(unsigned n=0;n<8;n++){
   TextureCacheKey key={keys[frame*8+n],{},0,0,0};
   gfx_texture_cache_lookup(0,key);gfx_rapi->upload_texture(rgba,128,128,false);
  }
  unsigned live,free_bytes;native_pica_texture_memory(&live,&free_bytes);peak=std::max(peak,live);
  if(!getenv("PD_PROBE_NO_TEXTURE_TRIM") && live>4*1024*1024+8*128*128*4)abort();
  pd_probe_texture_memory_frame(0);
 }
 unsigned old_live,old_free;native_pica_texture_memory(&old_live,&old_free);
 gfx_texture_cache_clear();
 unsigned live,free_bytes;native_pica_texture_memory(&live,&free_bytes);if(live || free_bytes!=old_free)abort();
 pd_probe_texture_memory_frame(1);pd_probe_texture_memory_frame(0);
 native_pica_texture_memory(&live,&free_bytes);if(live || free_bytes<old_free+old_live)abort();
 pd_probe_texture_memory_limit(0);
 fprintf(stderr,"TEXTURE_MEMORY_PASS 320 uploads under 4MiB allocation limit; peak=%u bytes; clear releases storage\n",peak);
}
extern "C" void pd_probe_texture_eviction_test(void) {
 static uint8_t pixels[TEXTURE_CACHE_MAX_SIZE+8][4];
 gfx_texture_cache_clear();
 TextureCacheKey pinned={pixels[0],{},0,0,0};
 gfx_texture_cache_lookup(1,pinned);
 TextureCacheNode *bound=rendering_state.textures[1];
 uint32_t id=bound->second.texture_id;
 for(unsigned n=1;n<TEXTURE_CACHE_MAX_SIZE+8;n++){
  TextureCacheKey key={pixels[n],{},0,0,0};
  gfx_texture_cache_lookup(0,key);
 }
 if(bound!=rendering_state.textures[1] || !(bound->first==pinned) || bound->second.texture_id!=id || gfx_texture_cache.map.find(pinned)==gfx_texture_cache.map.end())abort();
 gfx_texture_cache_clear();
 fprintf(stderr,"TEXTURE_EVICTION_PASS bound texture survives cache pressure\n");
}
'''
   instrumented.write_text(text);source=instrumented
  old=compile_overlay(row,source,target)
  cmd=[str(target) if token==old else token for token in cmd]
 for source in [root/'src/gyro_3ds.c',root/'src/runtime_3ds.c',root/'src/objectives_3ds.c',root/'src/native_dsp_mix.c',root/'tests/pica-host/performance_support.c']:
  target=overlays/(source.name+'.o');compile_overlay(rows[0],source,target);cmd.append(str(target))
motion=out/'motion_probe.o'
subprocess.run(['g++','-std=c++20','-O1','-g','-fsanitize=address','-fno-pie','-I'+str(root/'include'),'-c',str(root/'src/native_motion_sensor.cpp'),'-o',str(motion)],check=True)
cmd.append(str(motion))
cmd += [str(obj),str(planner),'-fsanitize=address,undefined','-no-pie','-Wl,--gc-sections','-Wl,--wrap=gfx_set_mipmap_filter,--wrap=gfx_init,--wrap=videoFramebuffersSupported,--wrap=videoCreateFramebuffer,--wrap=videoCopyFramebuffer,--wrap=videoSetAnisotropicFilter,--wrap=inputReadController,--wrap=videoGetDisplayModeIndex,--wrap=langGet']
subprocess.run(cmd,cwd=out,check=True)
title=bool(os.environ.get('PD_PROBE_TITLE'))
run=out/('run-pica-title' if title else 'run-pica');run.mkdir(exist_ok=True)
with (run/'reference.log').open('w') as log:
 result=subprocess.run([str(out/'pd-pica-probe'),'--boot-stage',('0x5a' if title else os.environ.get('PD_PROBE_STAGE','0x30')),* ([] if title else ['--no-sound']),'--no-crash-handler','--basedir',str(root/'local/roms'),'--savedir','.', '--log'],cwd=run,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0','SDL_AUDIODRIVER':'dummy','SDL_VIDEODRIVER':'offscreen','LIBGL_ALWAYS_SOFTWARE':'1','EGL_PLATFORM':'surfaceless'},stdout=log,stderr=subprocess.STDOUT,timeout=90)
 print('PICA submission probe exit:',result.returncode);print((run/'reference.log').read_text()[-3500:]);assert result.returncode==0 and ('TITLE_MENU_PRESENTED' if title and not os.environ.get('PD_PROBE_LAUNCH') else 'DEFECTION_PRESENTED frames=') in (run/'reference.log').read_text()
 if os.environ.get('PD_PROBE_LAUNCH'):
  assert 'stage='+str(int(os.environ.get('PD_PROBE_STAGE','0x30'),0))+' level_frames=' in (run/'reference.log').read_text()
