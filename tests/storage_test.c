#include "storage.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned char flash[0x100000],snapshot[0x100000];static int operations,fail;
static int read_flash(void*u,uint32_t a,void*p,unsigned n){(void)u;assert(a<=sizeof flash&&n<=sizeof flash-a);memcpy(p,flash+a,n);return 0;}
static int erase_flash(void*u,uint32_t a){(void)u;assert(af_storage_range(a,4096)&&!(a%4096));if(++operations==fail)return -1;memset(flash+a,255,4096);return 0;}
static int program_flash(void*u,uint32_t a,const void*p,unsigned n){(void)u;assert(af_storage_range(a,n));if(++operations==fail)return -1;const unsigned char*b=p;for(unsigned i=0;i<n;i++){assert((flash[a+i]&b[i])==b[i]);flash[a+i]&=b[i];}return 0;}
int main(void){af_storage s={0,read_flash,erase_flash,program_flash,0,0};unsigned char old[1024],next[1024],out[1024];memset(old,7,sizeof old);memset(next,19,sizeof next);memset(flash,255,sizeof flash);
 assert(af_storage_mount(&s,123)==AF_STORE_UNOWNED);assert(af_storage_save(&s,0,old,sizeof old)==AF_STORE_UNOWNED);assert(!operations);
 flash[AF_STORE_FIRST+4095]=0;assert(af_storage_claim_erased(&s,123)==AF_STORE_UNOWNED);assert(!operations);flash[AF_STORE_FIRST+4095]=255;
 assert(af_storage_claim_erased(&s,123)==0);assert(operations==1);assert(af_storage_save(&s,0,old,sizeof old)==0);memcpy(snapshot,flash,sizeof flash);
 for(int cut=1;cut<=6;cut++){memcpy(flash,snapshot,sizeof flash);operations=0;fail=cut;assert(af_storage_save(&s,0,next,sizeof next)==AF_STORE_IO);fail=0;assert(af_storage_mount(&s,123)==0);assert(af_storage_load(&s,0,out,sizeof out)==1024);assert(!memcmp(out,old,sizeof old));}
 memcpy(flash,snapshot,sizeof flash);assert(af_storage_save(&s,0,next,sizeof next)==0);assert(af_storage_load(&s,0,out,sizeof out)==1024&&!memcmp(out,next,sizeof next));assert(af_storage_slots(&s)==1);
 flash[AF_STORE_FIRST+4096+256]^=1;assert(af_storage_load(&s,0,out,sizeof out)==1024&&!memcmp(out,old,sizeof old));
 assert(af_storage_mount(&s,124)==AF_STORE_UNOWNED);assert(!af_storage_range(0xc0000,1));assert(!af_storage_range(0x9cfff,2));assert(!af_storage_range(0xbcfff,2));assert(!af_storage_range(0xff000,1));
 for(unsigned a=0;a<sizeof flash;a++)if(!af_storage_range(a,1))assert(flash[a]==255);
 {uint32_t v=0;af_storage u={0,read_flash,erase_flash,program_flash,0,0};assert(af_storage_setting_save(&u,AF_SETTING_POLYPHONY,5)==AF_STORE_UNOWNED);
  assert(af_storage_mount(&s,123)==0);assert(af_storage_setting_load(&s,AF_SETTING_POLYPHONY,&v)==AF_STORE_EMPTY);
  unsigned char slots[0x20000];memcpy(slots,flash+AF_STORE_FIRST,sizeof slots);
  for(uint32_t n=0;n<AF_SETTING_RECORDS*2+7;n++){assert(af_storage_setting_save(&s,AF_SETTING_POLYPHONY,1+n%16)==0);assert(af_storage_mount(&s,123)==0);assert(af_storage_setting_load(&s,AF_SETTING_POLYPHONY,&v)==0&&v==1+n%16);}
  operations=0;assert(af_storage_setting_save(&s,AF_SETTING_POLYPHONY,v)==0&&!operations);
  assert(!memcmp(slots,flash+AF_STORE_FIRST,sizeof slots));assert(af_storage_load(&s,0,out,sizeof out)==1024&&!memcmp(out,old,sizeof old));
  flash[AF_STORE_MARKER+256+16*2+8]^=1;memcpy(snapshot,flash,sizeof flash);
  for(int cut=1;cut<=2;cut++){memcpy(flash,snapshot,sizeof flash);operations=0;fail=cut;(void)af_storage_setting_save(&s,AF_SETTING_POLYPHONY,v==9?10:9);fail=0;assert(af_storage_mount(&s,123)==0);uint32_t w=0;assert(af_storage_setting_load(&s,AF_SETTING_POLYPHONY,&w)==0&&w>=1&&w<=16);}
  memcpy(flash,snapshot,sizeof flash);assert(af_storage_mount(&s,123)==0);}
 puts("storage blank-claim, interrupted replacement, corruption fallback and protected ranges, settings log wrap/isolation passed");return 0;}
