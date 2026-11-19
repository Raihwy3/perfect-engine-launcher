#include "pd_paths.h"
#include "native_version.h"
/* Launcher-owned startup graphics. No ROM assets are required to check a ROM. */
#include <3ds.h>
#include "pd_boot_pixels.h"
#include "pd_menu_theme.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint16_t top_pixels[400*240],bottom_pixels[320*240];
static int active;
static const char *model="Nintendo 3DS";
static uint16_t colour(unsigned rgb){return ((rgb>>8)&0xf800)|((rgb>>5)&0x7e0)|((rgb>>3)&31);}
static void rect(uint16_t *p,int width,int x,int y,int w,int h,unsigned rgb){
 for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)if((unsigned)i<(unsigned)width && (unsigned)j<240)p[j*width+i]=colour(rgb);
}
/* The NTSC Rev 1 Handel Gothic segment is read from the user's cartridge
 * image. No font pixels are compiled into the launcher. All offsets and
 * glyph extents are validated before the bootstrap uses them. */
static uint8_t rom_font[0x2280];
static int rom_font_ready;
static unsigned be32(const uint8_t *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void load_rom_font(void){
 FILE *file=fopen(PD_ROM_PATH,"rb");
 if(!file)return;
 size_t count=0;
 if(fseek(file,0x7f9d30,SEEK_SET)==0)count=fread(rom_font,1,sizeof rom_font,file);
 fclose(file);if(count!=sizeof rom_font)return;
 for(unsigned i=0;i<169;i++){int32_t k=(int32_t)be32(rom_font+i*4);if(k< -16 || k>32)return;}
 for(unsigned i=0;i<94;i++){
  const uint8_t *g=rom_font+676+i*12;
  unsigned offset=be32(g+8),bytes=(g[2]+2)*8;
  if(g[3]>15 || g[2]>32 || be32(g+4)>=13 || offset<1804
    || offset>sizeof rom_font || bytes>sizeof rom_font-offset)return;
 }
 rom_font_ready=1;
}
static void ink(uint16_t *p,int width,int x,int y,unsigned rgb,unsigned alpha){
 if((unsigned)x>=(unsigned)width || (unsigned)y>=240)return;
 unsigned old=p[y*width+x];
 unsigned r=(((rgb>>16)&255)*alpha+((old>>11)*255/31)*(255-alpha))/255;
 unsigned g=(((rgb>>8)&255)*alpha+(((old>>5)&63)*255/63)*(255-alpha))/255;
 unsigned b=((rgb&255)*alpha+((old&31)*255/31)*(255-alpha))/255;
 p[y*width+x]=colour((r<<16)|(g<<8)|b);
}
static void text(uint16_t *p,int width,int x,int y,const char *s,unsigned rgb){
 if(rom_font_ready){
  unsigned prev='H'-33;
  for(;*s;s++){
   unsigned c=(unsigned char)*s;
   if(c==' '){x+=5;prev='H'-33;continue;}
   if(c<33 || c>126)c='?';
   const uint8_t *g=rom_font+676+(c-33)*12;
   unsigned k1=be32(rom_font+676+prev*12+4),k2=be32(g+4);
   int kern=(int32_t)be32(rom_font+(k1*13+k2)*4);
   x-=kern-1;
   unsigned offset=be32(g+8);
   for(unsigned yy=0;yy<g[2];yy++)for(unsigned xx=0;xx<g[3];xx++){
    unsigned sx=xx+1,byte=rom_font[offset+(yy+1)*8+sx/2];
    unsigned index=sx&1?byte&15:byte>>4;
    unsigned alpha=index<9?0:index==15?255:(index-8)*36;
    if(alpha)ink(p,width,x+xx,y+(int8_t)g[1]+yy,rgb,alpha);
   }
   x+=g[3];prev=c-33;
  }
  return;
 }
 const ConsoleFont *font=&consoleGetDefault()->font;
 for(;*s;s++,x+=8){unsigned c=(unsigned char)*s;if(c<font->asciiOffset || c>=font->asciiOffset+font->numChars)continue;
  const uint8_t *glyph=font->gfx+(c-font->asciiOffset)*8;
  for(int j=0;j<8;j++)for(int i=0;i<8;i++)if(glyph[j]&(0x80>>i))rect(p,width,x+i,y+j,1,1,rgb);
 }
}
static unsigned mix(unsigned a,unsigned b,int n,int d){
 unsigned result=0;if(d<=0)return a;
 for(int shift=0;shift<=24;shift+=8)
  result|=(((((a>>shift)&255)*(d-n)+((b>>shift)&255)*n)/d)&255)<<shift;
 return result;
}
static void rgba_rect(uint16_t *p,int width,int x,int y,int w,int h,unsigned rgba){
 for(int yy=y;yy<y+h;yy++)for(int xx=x;xx<x+w;xx++)ink(p,width,xx,yy,rgba>>8,rgba&255);
}
static void window(uint16_t *p,int width,const char *title){
 uint32_t theme[6];pd_menu_default_theme(theme);
 rect(p,width,0,0,width,240,0);
 /* Same bounds, original palette, vertical three-stop title and body/borders
  * as dialogRender + menugfxRenderDialogBackground. */
 int left=14,right=width-14,top=8,title_bottom=19,bottom=232;
 rgba_rect(p,width,left+1,title_bottom,right-left-2,bottom-title_bottom,theme[4]);
 rgba_rect(p,width,left+1,title_bottom,1,bottom-title_bottom,theme[0]);
 rgba_rect(p,width,right-2,title_bottom,1,bottom-title_bottom,theme[2]);
 for(int x=left+1;x<right-1;x++)
  rgba_rect(p,width,x,bottom-1,1,1,mix(theme[0],theme[2],x-left-1,right-left-2));
 int middle=pd_menu_gradient_midpoint(top,title_bottom);
 for(int y=top;y<title_bottom;y++){
  unsigned c=y<middle?mix(theme[0],theme[1],y-top,middle-top)
   :mix(theme[1],theme[2],y-middle,title_bottom-middle);
  rgba_rect(p,width,left-2,y,right-left+4,1,c);
 }
 text(p,width,left+2,top+2,title,theme[3]>>8);
}
static void bar(uint16_t *p,int width,int y,unsigned percent){
 rect(p,width,24,y,width-48,7,0x003345);rect(p,width,25,y+1,(width-50)*(percent>100?100:percent)/100,5,0x009bad);
}
static void lcd(gfxScreen_t screen,const uint16_t *p,int width){
 u16 w,h;u8 *out=gfxGetFramebuffer(screen,GFX_LEFT,&w,&h);if(!out || w!=240 || h!=width)return;
 GSPGPU_FramebufferFormat format=gfxGetScreenFormat(screen);
 if(format==GSP_BGR8_OES){pd_boot_copy_bgr8(out,p,width);GSPGPU_FlushDataCache(out,width*240*3);}
 else if(format==GSP_RGB565_OES){pd_boot_copy_rgb565((u16 *)out,p,width);GSPGPU_FlushDataCache(out,width*240*2);}
}
static void show(const char *stage,unsigned task,unsigned total){
 if(!active)return;
 uint32_t theme[6];pd_menu_default_theme(theme);
 unsigned label=theme[5]>>8,value=theme[3]>>8;
 window(top_pixels,400,"PERFECT ENGINE LAUNCHER");
 text(top_pixels,400,24,60,"SYSTEM CHECK",label);text(top_pixels,400,24,78,model,value);
 text(top_pixels,400,24,110,stage,label);bar(top_pixels,400,136,task);
 text(top_pixels,400,24,166,"TOTAL PROGRESS",label);bar(top_pixels,400,184,total);
 window(bottom_pixels,320,"STARTUP STATUS");
 text(bottom_pixels,320,24,60,"Perfect Engine Launcher " PD_VERSION,label);
 text(bottom_pixels,320,24,91,stage,label);
 text(bottom_pixels,320,24,126,"Required ROM:",label);
 text(bottom_pixels,320,24,144,PD_ROM_NAME,value);
 text(bottom_pixels,320,24,164,"US retail, revision 1",label);
 lcd(GFX_TOP,top_pixels,400);lcd(GFX_BOTTOM,bottom_pixels,320);gfxFlushBuffers();
 gfxScreenSwapBuffers(GFX_TOP,false);gspWaitForVBlank();
}
void pd_boot_begin(void){
 static const char *names[]={"Nintendo 3DS","Nintendo 3DS XL","New Nintendo 3DS","Nintendo 2DS","New Nintendo 3DS XL","New Nintendo 2DS XL"};
 load_rom_font();
 u8 type=255;CFGU_GetSystemModel(&type);if(type<6)model=names[type];active=1;
 /* Keep the top LCD's existing 24-bit format throughout startup. Publish
  * complete back buffers at vblank rather than changing a live scanout. */
 gfxSet3D(false);gfxSetScreenFormat(GFX_TOP,GSP_BGR8_OES);gfxSetDoubleBuffering(GFX_TOP,true);
 show("Checking console",100,5);
}
void pd_boot_stage(const char *stage,unsigned task,unsigned total){show(stage,task,total);}
void pd_boot_rom_progress(unsigned done,unsigned total){unsigned pct=total?(unsigned)((uint64_t)done*100/total):0;show("Verifying ROM",pct,10+pct*75/100);}
void pd_boot_finish(void){
 show("Checks complete",100,100);active=0;
 window(bottom_pixels,320,"PERFECT ENGINE LAUNCHER");
 text(bottom_pixels,320,24,92,"Perfect Engine Launcher " PD_VERSION,0xb0e4ed);
 text(bottom_pixels,320,24,126,"Game files verified",0x70a8b6);lcd(GFX_BOTTOM,bottom_pixels,320);
 gfxFlushBuffers();
}

void pd_boot_wait(void){
 window(bottom_pixels,320,"PERFECT ENGINE LAUNCHER");
 text(bottom_pixels,320,24,92,"Perfect Engine Launcher " PD_VERSION,0xb0e4ed);
 text(bottom_pixels,320,24,126,"Preparing engine",0x70a8b6);
 lcd(GFX_BOTTOM,bottom_pixels,320);gfxFlushBuffers();
}

/* Update prompts use the same original blue/cyan window as file checks. */
void pd_boot_update(const char *stage,const char *message,const char *actions,const char *remote,unsigned done,unsigned total){
 window(top_pixels,400,"PERFECT ENGINE LAUNCHER");
 text(top_pixels,400,24,52,"Perfect Engine Launcher " PD_VERSION,0xb0e4ed);
 text(top_pixels,400,24,86,stage,0x70a8b6);
 if(total)bar(top_pixels,400,126,(unsigned)((uint64_t)done*100/total));
 window(bottom_pixels,320,"LAUNCHER UPDATE");
 text(bottom_pixels,320,24,38,"Installed: " NATIVE_RELEASE_TAG,0x70a8b6);
 if(remote && *remote)text(bottom_pixels,320,24,58,remote,0xb0e4ed);
 char line[160];int y=90;
 while(message && *message && y<180){
  unsigned n=0;while(message[n] && message[n]!='\n' && n<32)n++;
  if(n==32 && message[n] && message[n]!='\n'){
   unsigned split=n;while(split && message[split]!=' ')split--;if(split)n=split;
  }
  memcpy(line,message,n);line[n]=0;text(bottom_pixels,320,24,y,line,0xb0e4ed);
  message+=n;while(*message==' ' || *message=='\n')message++;y+=18;
 }
 text(bottom_pixels,320,24,202,actions,0x70a8b6);
 lcd(GFX_TOP,top_pixels,400);lcd(GFX_BOTTOM,bottom_pixels,320);gfxFlushBuffers();
 gfxScreenSwapBuffers(GFX_TOP,false);gspWaitForVBlank();
}
