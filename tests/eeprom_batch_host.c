
#include "../upstream/perfect_dark/port/src/libultra.c"
#include <assert.h>
static unsigned writes;
static u8 saved_image[EEPROM_SIZE];
const char *sysArgGetString(const char *key){(void)key;return NULL;}
s32 fsPathIsAbsolute(const char *p){(void)p;return 0;}
s32 fsPathIsCwdRelative(const char *p){(void)p;return 0;}
const char *fsFullPath(const char *p){return p;}
void sysLogPrintf(s32 level,const char *fmt,...){(void)level;(void)fmt;}
FILE *fsFileOpenRead(const char *p){(void)p;return NULL;}
FILE *fsFileOpenWrite(const char *p){(void)p;writes++;return tmpfile();}
void fsFileFree(FILE *f){fflush(f);rewind(f);assert(fread(saved_image,1,EEPROM_SIZE,f)==EEPROM_SIZE);fclose(f);}
int main(void){
 u8 bytes[8];
 pd_eeprom_batch_begin();
 for(int address=0;address<64;address++){
  memset(bytes,address,sizeof bytes);assert(osEepromLongWrite(NULL,address,bytes,8)==0);
 }
 assert(writes==0);pd_eeprom_batch_begin();
 memset(bytes,123,8);osEepromLongWrite(NULL,70,bytes,8);
 pd_eeprom_batch_end();assert(writes==0);
 pd_eeprom_batch_end();assert(writes==1);
 for(int address=0;address<64;address++)for(int i=0;i<8;i++)assert(saved_image[address*8+i]==address);
 assert(saved_image[70*8]==123);
 memset(bytes,99,8);osEepromLongWrite(NULL,0,bytes,8);assert(writes==2 && saved_image[0]==99);
 pd_eeprom_batch_begin();pd_eeprom_batch_end();assert(writes==2);
 puts("PASS EEPROM: 65 writes coalesced into one complete image, nested batches, immediate standalone persistence and no-op batches");
}
