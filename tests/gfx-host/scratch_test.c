#include "../../upstream/perfect_dark/src/game/gfxmemory.c"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
void sysLogPrintf(int level,const char *format,...){(void)level;(void)format;}
void sysFatalError(const char *format,...){(void)format;abort();}
int main(void){
 _Alignas(16) unsigned char pool[2][64];memset(pool,0x5a,sizeof pool);
 g_VtxBuffers[0]=pool[0];g_VtxBuffers[1]=pool[1];g_VtxBuffers[2]=pool[1]+64;
 g_GfxActiveBufferIndex=0;g_GfxMemPos=pool[0];
 unsigned char *base=gfxAllocate(48);assert(base==pool[0]);memset(base,0x11,48);
 unsigned char *matrix=gfxAllocateMatrix();assert(matrix!=pool[0] && !((uintptr_t)matrix&15));memset(matrix,0x22,sizeof(Mtx));
 assert((void *)gfxAllocateColours(1)==pool[0]+48);
 unsigned char *vertices=(unsigned char *)gfxAllocateVertices(20);assert(!((uintptr_t)vertices&15));memset(vertices,0x33,20*sizeof(Vtx));
 for(unsigned i=0;i<48;i++)assert(pool[0][i]==0x11);
 for(unsigned i=0;i<64;i++)assert(pool[1][i]==0x5a);
 assert(pd_spills[0] && !pd_spills[1]);
 gfxSwapBuffers();assert(g_GfxActiveBufferIndex==1 && pd_spills[0] && !pd_spills[1]);
 for(unsigned i=0;i<sizeof(Mtx);i++)assert(matrix[i]==0x22);
 unsigned char *other=gfxAllocate(128);memset(other,0x44,128);assert(pd_spills[1]);
 gfxSwapBuffers();assert(g_GfxActiveBufferIndex==0 && !pd_spills[0] && pd_spills[1]);
 for(unsigned i=0;i<128;i++)assert(other[i]==0x44);
 pdGfxFreeSpills(1);assert(!pd_spills[1]);
 puts("PASS graphics scratch: bounded pool, aligned overflow, intact adjacent buffer, two-frame spill lifetime and reclamation");
}
