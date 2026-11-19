"""Exercise the actual bootstrap font reader/drawing with host LCD stubs."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
rom=root/'local/roms/pd.ntsc-final.z64'
if not rom.exists():
 rom=next((root/'local/roms').glob('*.z64'))
with tempfile.TemporaryDirectory() as td:
 t=Path(td);folder=t/'sdmc:/3ds/perfect-engine-launcher';folder.mkdir(parents=True)
 (folder/'pd.z64').symlink_to(rom)
 (t/'3ds.h').write_text('''#include <stdint.h>
#include <stdbool.h>
typedef uint8_t u8;typedef uint16_t u16;
typedef int gfxScreen_t;typedef int GSPGPU_FramebufferFormat;
enum {GFX_TOP,GFX_BOTTOM,GFX_LEFT,GSP_BGR8_OES,GSP_RGB565_OES};
typedef struct {const uint8_t *gfx;unsigned asciiOffset,numChars;} ConsoleFont;
typedef struct {ConsoleFont font;} Console;
static Console boot_console;
static inline Console *consoleGetDefault(void){return &boot_console;}
static inline u8 *gfxGetFramebuffer(int a,int b,u16 *c,u16 *d){(void)a;(void)b;*c=*d=0;return 0;}
static inline int gfxGetScreenFormat(int a){(void)a;return GSP_BGR8_OES;}
static inline void GSPGPU_FlushDataCache(void *p,unsigned n){(void)p;(void)n;}
static inline void gfxFlushBuffers(void){}
static inline void gfxScreenSwapBuffers(int a,bool b){(void)a;(void)b;}
static inline void gspWaitForVBlank(void){}
static inline void CFGU_GetSystemModel(u8 *p){*p=2;}
static inline void gfxSet3D(bool b){(void)b;}
static inline void gfxSetScreenFormat(int a,int b){(void)a;(void)b;}
static inline void gfxSetDoubleBuffering(int a,bool b){(void)a;(void)b;}
''')
 (t/'probe.c').write_text('''#include <assert.h>
#include <stdlib.h>
#include "'''+str(root/'src/boot_3ds.c')+'''"
void pd_menu_default_theme(uint32_t p[6]){
 const uint32_t original[]={0x0060bf7f,0x0000507f,0x00f0ff7f,0xffffffff,0x00002f7f,0x00ffffff};
 memcpy(p,original,sizeof original);
}
int main(void){
 load_rom_font();assert(rom_font_ready);
 window(bottom_pixels,320,"PERFECT ENGINE LAUNCHER");
 text(bottom_pixels,320,24,60,"Checking ROM file",0x00ffff);
 unsigned lit=0;for(int y=60;y<80;y++)for(int x=24;x<200;x++)if(bottom_pixels[y*320+x]!=colour(0x000017))lit++;
 assert(lit>100);
 /* Original title gradient varies downwards; both ends of a scanline match. */
 assert(bottom_pixels[13*320+250]==bottom_pixels[13*320+300]);
 assert(bottom_pixels[8*320+300]!=bottom_pixels[18*320+300]);
 FILE *out=fopen("preview.rgb565","wb");assert(out);fwrite(bottom_pixels,2,320*240,out);fclose(out);
 /* Reject corrupt offsets and pathological kerning before drawing. */
 remove("sdmc:/3ds/perfect-engine-launcher/pd.z64");
 rom_font[684]=rom_font[685]=rom_font[686]=rom_font[687]=255;
 out=fopen("sdmc:/3ds/perfect-engine-launcher/pd.z64","wb");assert(out);
 fseek(out,0x7f9d30,SEEK_SET);fwrite(rom_font,1,sizeof rom_font,out);fclose(out);
 rom_font_ready=0;load_rom_font();assert(!rom_font_ready);
 rom_font[684]=rom_font[685]=0;rom_font[686]=7;rom_font[687]=24;
 rom_font[0]=0x7f;rom_font[1]=rom_font[2]=rom_font[3]=255;
 out=fopen("sdmc:/3ds/perfect-engine-launcher/pd.z64","wb");assert(out);
 fseek(out,0x7f9d30,SEEK_SET);fwrite(rom_font,1,sizeof rom_font,out);fclose(out);
 load_rom_font();assert(!rom_font_ready);
 puts("PASS actual boot ROM-font rendering, corrupt glyph bounds and kerning rejection");
}
''')
 exe=t/'probe';subprocess.run(['cc','-std=c11','-fsanitize=address,undefined','-I'+str(t),'-I'+str(root/'include'),str(t/'probe.c'),'-o',str(exe)],check=True)
 import os
 subprocess.run([str(exe)],cwd=t,check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
 (root/'build/boot-theme-preview.rgb565').write_bytes((t/'preview.rgb565').read_bytes())
