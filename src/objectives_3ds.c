#include "pd_text_wrap.h"
/* Original Perfect Dark objective drawing, laid out for the bottom LCD. */
#include <ultra64.h>
#include "constants.h"
#include "types.h"
#include "data.h"
#include "bss.h"
#include "game/game_1531a0.h"
#include "game/gfxmemory.h"
#include "game/lang.h"
#include "game/lv.h"
#include "game/menu.h"
#include "game/menuitem.h"
#include "game/menugfx.h"
#include "lib/mtx.h"
#include "lib/vi.h"
#include "pd_objectives.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "game/mainmenu.h"
#include "pd_runtime.h"
#include "pd_menu_frame_copy.h"
#include "pd_bottom_cadence.h"
#include "system.h"

extern s32 g_StageNum;
int pd_rendering_objectives;
int pd_bottom_mission_panel_visible(void){
 return g_Vars.currentplayer && !g_MainIsEndscreen && !g_Vars.normmplayerisrunning && g_StageNum<STAGE_TITLE && g_StageNum!=STAGE_CITRAINING;
}
static struct pd_bottom_clock panel_clock;
static unsigned panel_redraws,panel_retained,panel_menus;
int pd_bottom_refresh_due(void){
 int gameplay=pd_bottom_mission_panel_visible() && !g_Menus[0].curdialog && !g_Vars.in_cutscene && !g_Vars.normmplayerisrunning;
 uint32_t key=(2166136261u^(unsigned)g_StageNum)*16777619u;
 if(gameplay){
  key=(key^(unsigned)lvGetDifficulty())*16777619u;
  key=(key^(unsigned)g_MissionConfig.stageindex)*16777619u;
  for(unsigned i=0;i<ARRAYCOUNT(g_Briefing.objectivenames);i++){
   key=(key^(unsigned)g_Briefing.objectivenames[i])*16777619u;
   key=(key^(unsigned)g_Briefing.objectivedifficulties[i])*16777619u;
   key=(key^g_ObjectiveStatuses[i])*16777619u;
  }
 }
 int due=pd_bottom_clock_due(&panel_clock,sysGetMicroseconds(),key,gameplay);
 if(!gameplay)panel_menus++;else if(due)panel_redraws++;else panel_retained++;
 return due;
}
void pd_bottom_refresh_report(void){
 sysLogPrintf(0,"Bottom cadence: gameplay redraws=%u retained=%u full-rate=%u (10Hz content; input stays full-rate)",panel_redraws,panel_retained,panel_menus);
 panel_redraws=panel_retained=panel_menus=0;
}
int pd_bottom_objectives_visible(void)
{
    if (!pd_bottom_mission_panel_visible())
        return 0;
    struct menudialog *dialog = g_Menus[0].curdialog;
    if (!dialog) return !g_Vars.in_cutscene;
    if (!dialog->definition) return 0;
    struct menuitem *items = dialog->definition->items;
    for (unsigned i = 0; i < 96 && items[i].type != MENUITEMTYPE_END; ++i)
        if (items[i].type == MENUITEMTYPE_OBJECTIVES) return 1;
    return 0;
}

/* The briefing has six slots; MAX_OBJECTIVES describes a different array. */
int pd_bottom_objective_indices(int *indices, int capacity)
{
    int count = 0;
    for (unsigned index = 0; index < ARRAYCOUNT(g_Briefing.objectivenames); ++index) {
        if (!g_Briefing.objectivenames[index] ||
            !(g_Briefing.objectivedifficulties[index] & (1 << lvGetDifficulty()))) continue;
        if (indices && count < capacity) indices[count] = index;
        ++count;
    }
    return count;
}

/* A software canvas avoids any bottom-LCD GPU target or per-frame GSP
 * output reconfiguration. Fonts still come from the user's ROM. */
static uint16_t canvas[320*240];
static uint8_t canvas_alpha[320*240];
static struct menurendercontext native_context;
static int native_context_valid;
static int preview_rects[8],preview_valid;
static Gfx scratch[2048];
static int canvas_valid,canvas_dirty;
static int idle_valid;
static unsigned refresh,last_content_key;
extern u16 var8007fb3c[];
extern s32 var8007fac4;
static int text_rotation;
int pd_bottom_native_capturing;
int pd_bottom_ui_drawing;
int pd_top_ui_drawing;
int pd_bottom_text_overlay;
extern int pd_bottom_frame_supported(void) __attribute__((weak));
static int frame_copy_mode;
static struct menudialog *native_dialog;
static int native_valid,native_map;
static float native_scale=1.f,native_left,native_top;
static int native_source_x,native_source_y;
static int native_width,native_height;
static int tab_left=10,tab_right=319,tab_center=110;
struct native_hit {int x,y,w,h,index;struct menuitem *item;union menuitemdata *data;};
static struct native_hit native_hits[128];
static unsigned native_hit_count;
static uint64_t native_capture_started;

static uint16_t rgb565(unsigned r,unsigned g,unsigned b){return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}
static void pixel_raw(int x,int y,u32 colour,unsigned coverage){
 if((unsigned)x>=320 || (unsigned)y>=240)return;
 unsigned alpha=(colour&255)*coverage/255;if(!alpha)return;
 unsigned r=colour>>24,g=(colour>>16)&255,b=(colour>>8)&255;
 unsigned old=canvas[y*320+x];
 canvas_alpha[y*320+x]=alpha+canvas_alpha[y*320+x]*(255-alpha)/255;
 unsigned or=((old>>11)&31)*255/31,og=((old>>5)&63)*255/63,ob=(old&31)*255/31;
 canvas[y*320+x]=rgb565((r*alpha+or*(255-alpha))/255,(g*alpha+og*(255-alpha))/255,(b*alpha+ob*(255-alpha))/255);
}
static int pixel_round(float v){return (int)(v+(v<0?-.5f:.5f));}
static void pixel(int x,int y,u32 colour,unsigned coverage){

 if(text_rotation){int old=x;x=text_rotation-y;y=old;}
 if(!native_map){pixel_raw(x,y,colour,coverage);return;}
 if(pd_bottom_native_capturing && (x<native_source_x || x>=native_source_x+native_dialog->width || y<native_source_y+12 || y>=native_source_y+native_dialog->height))return;
 if(native_scale==1.f){
  pixel_raw(x-native_source_x+(int)native_left,y-native_source_y+(int)native_top,colour,coverage);return;
 }
 int x1=pixel_round(native_left+(x-native_source_x)*native_scale);
 int x2=pixel_round(native_left+(x+1-native_source_x)*native_scale);
 int y1=pixel_round(native_top+(y-native_source_y)*native_scale);
 int y2=pixel_round(native_top+(y+1-native_source_y)*native_scale);
 for(int yy=y1;yy<y2;yy++)for(int xx=x1;xx<x2;xx++)pixel_raw(xx,yy,colour,coverage);
}
Gfx *pd_bottom_text(Gfx *gdl,s32 *x,s32 *y,char *text,struct fontchar *chars,struct font *font,u32 colour,s32 width,s32 height,s32 offset,s32 lineheight){
 if(!text || !chars || !font)return gdl;
 int origin=*x,top=*y,previous='H'-33;
 if(!lineheight)lineheight=chars['['].height+chars['['].baseline;
 if(lineheight<=0)lineheight=12;
 for(unsigned n=0;text[n] && n<8000;n++){
  unsigned code=(unsigned char)text[n];
  if(code==' '){*x+=5;previous='H'-33;continue;}
  if(code=='\n'){*x=origin;*y+=lineheight;previous='H'-33;continue;}
  if(code<33 || code>126)code='?';
  int index=code-33;struct fontchar *glyph=&chars[index];
  unsigned a=chars[previous].kerningindex,b=glyph->kerningindex;
  if(a<13 && b<13)*x-=var8007fac4+font->kerning[a*13+b]-1;
  if(glyph->pixeldata && glyph->width<=15 && glyph->height<=32){
   for(int yy=0;yy<glyph->height;yy++)for(int xx=0;xx<glyph->width;xx++){
    int dx=*x+xx,dy=*y+offset+glyph->baseline+yy;
    if(dx<origin || dx>=origin+width || dy<top || dy>=top+height)continue;
    // CI4, eight bytes per row, with the original one-pixel glyph border.
    unsigned sx=xx+1,sy=yy+1;
    unsigned byte=glyph->pixeldata[sy*8+(sx>>1)];
    unsigned palette=(sx&1)?byte&15:byte>>4;
    pixel(dx,dy,colour,var8007fb3c[palette]&255);
   }
  }
  *x+=glyph->width;previous=index;
 }
 return gdl;
}
static void pd_bottom_layout_rect(int,int,int,int,int *,int *,int *,int *);
Gfx *pd_bottom_line(Gfx *gdl,s32 x1,s32 y1,s32 x2,s32 y2,u32 a,u32 b,int vertical){
 if(pd_bottom_ui_drawing)return gdl;
 if(x2<=x1 || y2<=y1)return gdl;
 if(pd_bottom_native_capturing){
  int dx,dy,dw,dh;pd_bottom_layout_rect(x1,y1,x2-x1,y2-y1,&dx,&dy,&dw,&dh);
  x1=dx;y1=dy;x2=dx+dw;y2=dy+dh;
 }
 int span=vertical?y2-y1:x2-x1;
 for(int y=y1<0?0:y1;y<y2 && y<(native_map?480:240);y++)for(int x=x1<0?0:x1;x<x2 && x<(native_map?640:320);x++){
  unsigned colour=a;
  if(a!=b){unsigned f=(vertical?y-y1:x-x1)*255/span;colour=0;for(unsigned shift=0;shift<32;shift+=8)colour|=((((a>>shift)&255)*(255-f)+((b>>shift)&255)*f)/255)<<shift;}
  pixel(x,y,colour,255);
 }
 return gdl;
}
/* The original dialog's 12-pixel title strip and three body borders.
 * This draws into the existing RGB565 framebuffer, never another GPU target. */
static const struct menucolourpalette *window_palette;
static void ui_text(int x,int y,char *label,u32 colour,int small,int width){
 pd_bottom_text(scratch,&x,&y,label,
  small?g_CharsHandelGothicXs:g_CharsHandelGothicSm,
  small?g_FontHandelGothicXs:g_FontHandelGothicSm,colour,width,240-y,0,0);
}
static void ui_rect(int x,int y,int right,int bottom,u32 a,u32 b){
 pd_bottom_line(scratch,x,y,right,bottom,a,b,0);
}
static void ui_tabs(struct menudialog *dialog){
 struct menu *menu=&g_Menus[0];
 if(!dialog || menu->depth<1 || menu->depth>6)return;
 struct menulayer *layer=&menu->layers[menu->depth-1];
 if(layer->numsiblings<2 || layer->numsiblings>5 || layer->cursibling<0 || layer->cursibling>=layer->numsiblings)return;
 for(int side=0;side<2;side++){
  int index=(layer->cursibling+(side?1:layer->numsiblings-1))%layer->numsiblings;
  struct menudialog *sibling=layer->siblings[index];if(!sibling || !sibling->definition)continue;
  char title[96];snprintf(title,sizeof title,"%s",menuResolveDialogTitle(sibling->definition));
  int width=0,height=0;
  textMeasure(&height,&width,title,g_CharsHandelGothicXs,g_FontHandelGothicXs,0);
  text_rotation=side?tab_right:tab_left;
  ui_text(tab_center-width/2,0,title,window_palette->dialog_titlefg,1,180);
  text_rotation=0;
 }
}
static void ui_window(struct menudialog *dialog,char *title){
 int type=dialog?dialog->type:MENUDIALOGTYPE_DEFAULT;
 if(type<1 || type>5)type=MENUDIALOGTYPE_DEFAULT;
 window_palette=&g_MenuColours[type];
 const struct menucolourpalette *p=window_palette;
 memset(canvas,0,sizeof canvas);memset(canvas_alpha,0,sizeof canvas_alpha);
 unsigned body=p->dialog_bodybg,a=body&255;
 uint16_t body_pixel=rgb565((body>>24)*a/255,((body>>16)&255)*a/255,((body>>8)&255)*a/255);
 for(int y=20;y<232;y++)for(int x=12;x<308;x++){canvas[y*320+x]=body_pixel;canvas_alpha[y*320+x]=a;}
 ui_rect(8,8,160,20,p->dialog_border1,p->dialog_titlebg);
 ui_rect(160,8,312,20,p->dialog_titlebg,p->dialog_border2);
 ui_rect(8,8,312,9,p->dialog_border1,p->dialog_border2);
 ui_rect(11,20,12,233,p->dialog_border1,p->dialog_border1);
 ui_rect(308,20,309,233,p->dialog_border2,p->dialog_border2);
 ui_rect(11,232,309,233,p->dialog_border1,p->dialog_border2);
 ui_text(13,11,title,0x000000ff,0,294);
 ui_text(12,10,title,p->dialog_titlefg,0,294);
 ui_tabs(dialog);
}
int pd_bottom_menu_present(struct menudialog *dialog,const struct pd_bottom_row *rows,unsigned count,int paged){
 if(!g_FontHandelGothicSm || !g_FontHandelGothicXs || !dialog || !dialog->definition)return 0;
 idle_valid=0;
 canvas_valid=0; /* Invalidate cached objectives after sharing their canvas. */
 char title[128];snprintf(title,sizeof title,"%s",menuResolveDialogTitle(dialog->definition));
 ui_window(dialog,title);
 for(unsigned i=0;i<count && i<6;i++){
  int y=24+i*24;u32 colour=window_palette->item_unfocused;
  if(rows[i].focused){
   ui_rect(15,y+4,111,y+5,window_palette->item_focused_inner,window_palette->item_focused_outer);
   colour=window_palette->item_focused_inner;
  }
  ui_text(20,y,(char*)rows[i].label,colour,0,rows[i].value[0]?146:282);
  if(rows[i].value[0]){
   int h=0,w=0;textMeasure(&h,&w,(char*)rows[i].value,g_CharsHandelGothicSm,g_FontHandelGothicSm,0);
   int x=300-w;if(x<170)x=170;
   ui_text(x,y,(char*)rows[i].value,window_palette->item_unfocused,0,302-x);
  }
 }
 /* Read-only labels and briefing paragraphs also keep the game's typography. */
 int note_y=24+count*24;
 for(unsigned i=0;i<96 && note_y<190;i++){
  struct menuitem *item=&dialog->definition->items[i];
  if(item->type==MENUITEMTYPE_END)break;
  char *text=NULL;
  if(item->type==MENUITEMTYPE_LABEL)text=menuResolveParam2Text(item);
  else if(item->type==MENUITEMTYPE_SCROLLABLE)text=menuitemScrollableGetText(item->param);
  if(!text || !text[0])continue;
  char bounded[512],wrapped[4096];snprintf(bounded,sizeof bounded,"%s",text);
  pd_text_wrap(278,bounded,wrapped,sizeof wrapped,g_CharsHandelGothicSm,g_FontHandelGothicSm,textMeasure);
  int x=20,y=note_y;
  pd_bottom_text(scratch,&x,&y,wrapped,g_CharsHandelGothicSm,g_FontHandelGothicSm,
    window_palette->item_unfocused,280,196-note_y,0,12);
  note_y=y+16;
 }

 pd_bottom_bitmap_present(canvas);
 return 1;
}
/* Mirror original menu text as it is drawn, preserving the engine's columns,
 * scrolling, alignment and live values. No second menu render or handlers. */
static void pd_bottom_frame_mapping(float width,float height){
 if(width<=0 || height<=0){width=320;height=220;}
 native_scale=240.f/height;
 native_left=160.f+(native_source_x-width*.5f)*native_scale;
 native_top=native_source_y*native_scale;
}
void pd_bottom_native_begin(struct menudialog *dialog){
 pd_bottom_native_capturing=0;
#ifdef __3DS__
 /* The independent GPU pass supplies bottom hit targets and content. The
  * top pass's software raster/clear was immediately discarded by that pass. */
 if(!pd_bottom_ui_drawing)return;
#endif
 if(!dialog || dialog->width<20 || dialog->height<12){native_valid=0;return;}
 idle_valid=0;
 frame_copy_mode=pd_bottom_frame_supported && pd_bottom_frame_supported();
 native_capture_started=sysGetMicroseconds();
 native_dialog=dialog;native_valid=0;canvas_valid=0;native_hit_count=0;
 native_source_x=dialog->x;native_source_y=dialog->y;
 native_width=dialog->width;native_height=dialog->height;
 native_scale=1.f;native_left=12;native_top=8;native_map=0;
 preview_valid=0;native_context_valid=0;memset(canvas,0,sizeof canvas);memset(canvas_alpha,0,sizeof canvas_alpha);
 char title[128];snprintf(title,sizeof title,"%s",menuResolveDialogTitle(dialog->definition));
 if(!pd_bottom_ui_drawing)ui_window(dialog,title);
 pd_bottom_native_capturing=1;

}
void pd_bottom_native_end(void){
 if(!pd_bottom_native_capturing)return;
 pd_bottom_native_capturing=0;native_map=0;native_valid=1;
 pd_profile_record(13,sysGetMicroseconds()-native_capture_started);
}
/* Resize row/column containers, while leaving font pixels and line thickness
 * at native LCD size. Per-row text offsets retain the original spacing. */
static void pd_bottom_layout_rect(int x,int y,int w,int h,int *dx,int *dy,int *dw,int *dh){
 if(pd_bottom_ui_drawing){*dx=x;*dy=y;*dw=w;*dh=h;return;}
 float sx=296.f/(native_width?native_width:296);
 float sy=210.f/(native_height>13?native_height-13:210);if(sy>1.25f)sy=1.25f;
 int cy=native_context_valid?native_context.y:native_source_y+13;
 *dx=12+(int)((x-native_source_x)*sx);
 *dy=22+(int)((cy-native_source_y-13)*sy)+(y-cy);
 *dw=w>2?(int)(w*sx):w;*dh=h;
}
void pd_bottom_native_context(struct menurendercontext *ctx){
 if(!pd_bottom_native_capturing || !ctx)return;
 native_context=*ctx;native_context_valid=1;
 if(ctx->item && ctx->item->type==MENUITEMTYPE_MODEL){
  extern void pd_bottom_frame_geometry(float *,float *) __attribute__((weak));
  float width=320,height=220;if(pd_bottom_frame_geometry)pd_bottom_frame_geometry(&width,&height);
  float scale=240.f/height;
  preview_rects[0]=200+(ctx->x-width*.5f)*scale;preview_rects[1]=ctx->y*scale;
  preview_rects[2]=preview_rects[0]+ctx->width*scale;preview_rects[3]=preview_rects[1]+ctx->height*scale;
  int dx,dy,dw,dh;pd_bottom_layout_rect(ctx->x,ctx->y,ctx->width,ctx->height,&dx,&dy,&dw,&dh);
  int available=ctx->height*210/(native_height>13?native_height-13:210);
  int fitted=ctx->height*dw/(ctx->width?ctx->width:1);
  if(fitted>available){dw=ctx->width*available/(ctx->height?ctx->height:1);fitted=available;}
  preview_rects[4]=dx;preview_rects[5]=dy+(available-fitted)/2;
  preview_rects[6]=dx+dw;preview_rects[7]=preview_rects[5]+fitted;preview_valid=1;
 }
}
void pd_bottom_native_text(s32 x,s32 y,char *text,struct fontchar *chars,struct font *font,u32 colour,s32 width,s32 height,s32 offset,s32 lineheight){
 extern bool g_TextRotated90;
 if(!pd_bottom_native_capturing || g_TextRotated90 || pd_bottom_ui_drawing)return;
 int dx,dy,dw,dh;pd_bottom_layout_rect(x,y,width,height,&dx,&dy,&dw,&dh);
 if(native_context_valid){
  int th=0,tw=0;textMeasure(&th,&tw,text,chars,font,0);
  int right=native_context.x+native_context.width;
  if(x>native_context.x+native_context.width/2 && x+tw>=right-12){
   int colright=12+(right-native_source_x)*296/native_width;
   dx=colright-(right-x-tw)-tw;
  }
 }
 if(dx<12)dx=12;if(dy<21 || dy>=232)return;
 pd_bottom_text(scratch,&dx,&dy,text,chars,font,colour,308-dx,232-dy,offset,lineheight);
}
void pd_bottom_native_hit(struct menurendercontext *ctx,int index,int y,int height){
 if(!pd_bottom_native_capturing || native_hit_count>=ARRAYCOUNT(native_hits) || !ctx)return;
 pd_bottom_native_context(ctx);
 int x2,y2,w2,h2;pd_bottom_layout_rect(ctx->x,y,ctx->width,height,&x2,&y2,&w2,&h2);
 if(y2+h2<22 || y2>=232)return;
 native_hits[native_hit_count++]=(struct native_hit){x2,y2,w2,h2,index,ctx->item,ctx->data};
}
void pd_bottom_keyboard_hit(struct menurendercontext *ctx,int row,int col,int x,int y,int w,int h){
 if(!pd_bottom_native_capturing || !pd_bottom_ui_drawing || !ctx || native_hit_count>=ARRAYCOUNT(native_hits))return;
 native_hits[native_hit_count++]=(struct native_hit){x,y,w,h,row*10+col,ctx->item,ctx->data};
}
int pd_bottom_preview(int *rects){if(!preview_valid)return 0;memcpy(rects,preview_rects,sizeof preview_rects);return 1;}
const uint16_t *pd_bottom_overlay_pixels(void){return canvas;}
const uint8_t *pd_bottom_overlay_alpha(void){return canvas_alpha;}
static int objectives_gpu,standalone_window;
void pd_bottom_standalone_window(int enabled){standalone_window=enabled;}
int pd_bottom_menu_geometry(float *rect){
 if((standalone_window || (objectives_gpu && !g_Menus[0].curdialog && pd_bottom_mission_panel_visible())) && rect){
  rect[0]=320;rect[1]=240;rect[2]=14;rect[3]=8;rect[4]=292;rect[5]=224;return 1;
 }
 if(!native_valid || native_dialog!=g_Menus[0].curdialog || !rect)return 0;
 extern void pd_bottom_frame_geometry(float *,float *) __attribute__((weak));
 rect[0]=320;rect[1]=220;if(pd_bottom_frame_geometry)pd_bottom_frame_geometry(&rect[0],&rect[1]);
 rect[2]=native_source_x;rect[3]=native_source_y;rect[4]=native_width;rect[5]=native_height;
 return 1;
}
int pd_bottom_menu_canvas_valid(void){return native_valid && native_dialog==g_Menus[0].curdialog;}
int pd_bottom_native_present(void){
 if(!native_valid || native_dialog!=g_Menus[0].curdialog)return 0;
 if(!frame_copy_mode)pd_bottom_bitmap_present(canvas);return 1;
}
uintptr_t pd_bottom_frame_token(void){
 return frame_copy_mode && native_valid && native_dialog==g_Menus[0].curdialog?(uintptr_t)native_dialog:0;
}
int pd_bottom_frame_bounds(int *bounds){
 if(!pd_bottom_frame_token())return 0;
 extern void pd_bottom_frame_geometry(float *,float *) __attribute__((weak));
 float width=320,height=220;if(pd_bottom_frame_geometry)pd_bottom_frame_geometry(&width,&height);
 float scale=240.f/height;
 bounds[0]=200+(native_source_x-width*.5f)*scale;
 bounds[1]=(native_source_y+12)*scale;
 bounds[2]=200+(native_source_x+native_width-width*.5f)*scale;
 bounds[3]=(native_source_y+native_height)*scale;
 return 1;
}
int pd_bottom_native_touch(int x,int y){
 if(!native_valid || native_dialog!=g_Menus[0].curdialog)return -1;
 if((unsigned)x>=320 || (unsigned)y>=240)return 0;
 if(x<12)return 3;if(x>=308)return 4;
 if(y>=8 && y<21)return 2;
 int sx=x,sy=y;
 for(unsigned i=native_hit_count;i>0;i--){
  struct native_hit *hit=&native_hits[i-1];
  if(sx<hit->x || sx>=hit->x+hit->w || sy<hit->y || sy>=hit->y+hit->h)continue;
  native_dialog->focuseditem=hit->item;
  if(hit->item->type==MENUITEMTYPE_KEYBOARD && hit->data){
   hit->data->keyboard.row=hit->index/10;hit->data->keyboard.col=hit->index%10;
   return 1;
  }
  if(hit->item->type==MENUITEMTYPE_SLIDER)return sx<hit->x+hit->w/2?3:4;
  if(hit->index>=0 && hit->item->handler){
   if(hit->data)hit->data->list.index=hit->index;
   union handlerdata data={0};data.list.value=hit->index;
   hit->item->handler(MENUOP_LISTITEMFOCUS,hit->item,&data);
  }
  return 1;
 }
 return 0;
}
int pd_bottom_idle_present(void){
#ifdef __3DS__
 /* Native standalone windows replace the old software launcher panel. Before
  * ROM fonts load, platform code retains the themed bootstrap window. */
 extern void pd_bottom_present(const char *);
 if(!g_FontHandelGothicSm)return 0;
 return 1;
#endif
 if(!g_FontHandelGothicSm || !g_FontHandelGothicXs)return 0;
 if(idle_valid)return 1;
 canvas_valid=0;
 ui_window(NULL,"Perfect Engine Launcher");
 pd_bottom_bitmap_present(canvas);idle_valid=1;
 return 1;
}
int pd_bottom_objectives_present(void){
 if(objectives_gpu && !g_Menus[0].curdialog && pd_bottom_mission_panel_visible())return 1;
 if(!canvas_valid || !pd_bottom_objectives_visible())return 0;
 if(canvas_dirty){pd_bottom_bitmap_present(canvas);canvas_dirty=0;}
 return 1;
}
void pd_bottom_objectives_reset(void){panel_clock.valid=0;standalone_window=0;objectives_gpu=0;native_valid=native_map=pd_bottom_native_capturing=0;native_dialog=NULL;idle_valid=0;canvas_valid=canvas_dirty=0;refresh=last_content_key=0;}

Gfx *pd_render_bottom_objectives(Gfx *gdl)
{
    objectives_gpu=0;
    if(g_Menus[0].curdialog)return gdl;
    if(!g_FontHandelGothicSm){canvas_valid=0;return gdl;}
#ifdef __3DS__
    if(!pd_bottom_mission_panel_visible()){
     extern Gfx *pd_render_bottom_idle_native(Gfx *);
     canvas_valid=canvas_dirty=0;
     return pd_render_bottom_idle_native(gdl);
    }
#endif
    if (!pd_bottom_mission_panel_visible() || !g_FontHandelGothicXs)
        {canvas_valid=0;return gdl;}
#ifdef __3DS__
    /* Draw the original window and objective widgets on the bottom target. */
    extern Gfx *pd_render_bottom_objectives_native(Gfx *);
    canvas_valid=canvas_dirty=0;objectives_gpu=1;
    return pd_render_bottom_objectives_native(gdl);
#endif
    if(canvas_valid && ++refresh%6)return gdl;
    int indices[ARRAYCOUNT(g_Briefing.objectivenames)];
    int visible = pd_bottom_objective_indices(indices, ARRAYCOUNT(indices));
    unsigned key=2166136261u;
    key=(key^(unsigned)g_StageNum)*16777619u;
    key=(key^(unsigned)(g_Menus[0].curdialog!=NULL))*16777619u;
    for(int row=0;row<visible;row++){
        int i=indices[row];key=(key^(unsigned)i)*16777619u;
        key=(key^g_ObjectiveStatuses[i])*16777619u;
        char *name=langGet(g_Briefing.objectivenames[i]);
        for(unsigned j=0;name && name[j] && j<128;j++)key=(key^(unsigned char)name[j])*16777619u;
    }
    if(canvas_valid && key==last_content_key)return gdl;
    last_content_key=key;idle_valid=0;
    Gfx *original=gdl;gdl=scratch;
    for(unsigned i=0;i<320*240;i++)canvas[i]=rgb565(0,0,24);

    struct menudialog dialog = {0};
    dialog.type = dialog.type2 = MENUDIALOGTYPE_DEFAULT;
    dialog.transitionfrac = -1.f;
    dialog.width = 320;
    dialog.height = 240;
    s16 old_width = viGetWidth(), old_height = viGetHeight();
    int old_scale = g_ScaleX;
    viSetSize(320, 240);
    g_ScaleX = 1;
    textBackupAndResetBlends();
    pd_rendering_objectives = 1;

    char title[128];
    struct menudialog *active=g_Menus[0].curdialog;
    if(active && active->definition)snprintf(title,sizeof title,"%s",menuResolveDialogTitle(active->definition));
    else snprintf(title,sizeof title,"%s: %s",langGet(g_SoloStages[g_MissionConfig.stageindex].name3),langGet(L_OPTIONS_172));
    ui_window(active,title);

    int spacing = visible > 4 ? 22 : 28;
    for (int row = 0; row < visible; ++row) {
        gdl = menuitemObjectivesRenderOne(gdl, &dialog, indices[row], row + 1,
            16, 32 + row * spacing, 288, 164, true, false);
    }

    if(active){
        ui_text(22,44+visible*spacing,langGet(L_OPTIONS_173),window_palette->item_unfocused,0,280);

    }


    pd_rendering_objectives = 0;
    textRestoreBlends();
    g_ScaleX = old_scale;
    viSetSize(old_width, old_height);
    assert(gdl>=scratch && gdl<scratch+ARRAYCOUNT(scratch));
    canvas_valid=canvas_dirty=1;
    return original;
}
