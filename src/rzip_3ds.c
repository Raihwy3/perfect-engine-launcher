/* Rare 1172/1173 framing around the public-domain inflate core. */
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;
#include "inflate.h"
extern u8 *inbuf;extern u32 inptr;
static struct huft workspace[16384];
void *var80091558;
int rzipIs1172(void *p){u8 *s=p;return s && s[0]==0x11 && s[1]==0x72;}
int rzipIs1173(void *p){u8 *s=p;return s && s[0]==0x11 && s[1]==0x73;}
int32_t rzipInflate(void *source,void *dest,void *scratch) {
 (void)scratch;u8 *s=source;
 if(!s || !dest || (!rzipIs1172(s) && !rzipIs1173(s)))return 0;
 u32 result=decompress_entry(s+(rzipIs1173(s)?3:0),dest,workspace);
 var80091558=inbuf+inptr;
 return result;
}
u32 rzipInit(void){return 0;}
void *rzipGetSomething(void){return var80091558;}
