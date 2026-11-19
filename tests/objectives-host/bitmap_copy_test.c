#include "pd_bitmap_copy.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
int main(void){
 uint16_t *src=malloc((76800+2)*2),*dst=malloc((76800+2)*2);
 src[0]=src[76801]=dst[0]=dst[76801]=0xbeef;
 for(int y=0;y<240;y++)for(int x=0;x<320;x++)src[1+y*320+x]=(uint16_t)(y*997+x*17);
 pd_bitmap_copy(dst+1,src+1);
 for(int y=0;y<240;y++)for(int x=0;x<320;x++)assert(dst[1+x*240+239-y]==src[1+y*320+x]);
 assert(src[0]==0xbeef && src[76801]==0xbeef && dst[0]==0xbeef && dst[76801]==0xbeef);
 free(src);free(dst);puts("PASS all 76800 rotated LCD pixels and buffer canaries");
}
