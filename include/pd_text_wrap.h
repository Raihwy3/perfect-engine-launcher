#pragma once
#include <stddef.h>
struct fontchar;struct font;
typedef void (*pd_wrap_measure)(int *,int *,char *,struct fontchar *,struct font *,int);
/* Bounded replacement for briefing wrapping. The original uses an unbounded
 * destination and a 32-byte word buffer, including for long descriptions. */
static inline void pd_text_wrap(int width,const char *src,char *dst,size_t capacity,struct fontchar *chars,struct font *font,pd_wrap_measure measure){
 if(!capacity)return;
 dst[0]=0;
 if(!src || !chars || !font)return;
 if(width<8)width=8;
 size_t used=0;int line=0;
 while(*src && used+1<capacity){
   if(*src=='\n'){dst[used++]='\n';src++;line=0;continue;}
   if(*src==' '){if(line){dst[used++]=' ';line+=4;}src++;continue;}
   char word[128];size_t n=0;
   while(*src && *src!=' ' && *src!='\n' && n<sizeof(word)-3){
     unsigned char c=(unsigned char)*src++;
     if(c>=128 && *src){word[n++]=(char)c;word[n++]=*src++;}
     else word[n++]=(c>=32 && c!=127)?(char)c:'?';
   }
   word[n]=0;int h=0,w=0;measure(&h,&w,word,chars,font,0);
   if(line && line+w>width){if(used+1>=capacity)break;dst[used++]='\n';line=0;}
   if(w<=width){
     if(n>=capacity-used)break;
     for(size_t i=0;i<n;i++)dst[used++]=word[i];
     line+=w;
   }else for(size_t i=0;i<n;){
     size_t count=((unsigned char)word[i]>=128 && i+1<n)?2:1;
     char glyph[3]={word[i],count==2?word[i+1]:0,0};measure(&h,&w,glyph,chars,font,0);
     if(line && line+w>width){if(used+1>=capacity)goto end;dst[used++]='\n';line=0;}
     if(count>=capacity-used)goto end;
     while(count--)dst[used++]=word[i++];
     line+=w;
   }
 }
end:dst[used]=0;
}
