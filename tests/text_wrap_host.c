#include <assert.h>
#include <stdio.h>
#include <string.h>
struct fontchar {int n;};struct font {int n;};
#include "pd_text_wrap.h"
static void measure(int *h,int *w,char *s,struct fontchar *c,struct font *f,int a){(void)c;(void)f;(void)a;*h=12;*w=(int)strlen(s)*6;}
int main(void){
 struct fontchar c={0};struct font f={0};
 struct {char before[16],out[8000],after[16];} b;memset(&b,0x55,sizeof b);
 char longtext[16000];memset(longtext,'W',sizeof longtext-1);longtext[sizeof longtext-1]=0;
 for(int width=-2;width<=280;width+=7)for(size_t cap=1;cap<=sizeof b.out;cap=cap*2+1){
  memset(&b,0x55,sizeof b);
  pd_text_wrap(width,longtext,b.out,cap,&c,&f,measure);assert(memchr(b.out,0,cap));
  for(size_t i=cap;i<sizeof b.out;i++)assert((unsigned char)b.out[i]==0x55);
  for(unsigned i=0;i<16;i++)assert((unsigned char)b.before[i]==0x55 && (unsigned char)b.after[i]==0x55);
 }
 char out[256];pd_text_wrap(60,"|Heading\n\nA normal briefing paragraph.",out,sizeof out,&c,&f,measure);
 assert(strstr(out,"|Heading\n\n") && strstr(out,"briefing"));
 pd_text_wrap(60,NULL,out,sizeof out,&c,&f,measure);assert(!out[0]);
 puts("PASS bounded briefing wrap: long words/descriptions, tiny widths/capacities, headings and buffer canaries");
}
