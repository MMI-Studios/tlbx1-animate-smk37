#include "storage.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint8_t memory[0x100000],reply[320];static unsigned erases,programs,paused;
static af_storage af_store;static unsigned af_store_device=1,af_store_claiming;
static int af_flash_read(void*u,uint32_t a,void*p,unsigned n){(void)u;assert(a<=sizeof memory&&n<=sizeof memory-a);memcpy(p,memory+a,n);return 0;}
static int erase(void*u,uint32_t a){(void)u;assert(paused&&af_store_claiming&&af_storage_range(a,4096));erases++;memset(memory+a,255,4096);return 0;}
static int program(void*u,uint32_t a,const void*p,unsigned n){(void)u;assert(paused&&af_store_claiming&&af_storage_range(a,n));programs++;memcpy(memory+a,p,n);return 0;}
static uint32_t af_store_crc_step(uint32_t c,const uint8_t*p,unsigned n){while(n--){c^=*p++;for(unsigned k=0;k<8;k++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return c;}
static unsigned af_patch_pending(void){return 0;}static void af_patch_slots(unsigned n){(void)n;}
static void af_store_pause(void){paused=1;}static void af_store_resume(void){paused=0;}
static uint32_t fm1_ticks(void){return 1234;}static void ota_frame_done(void){}
static int ota_wire_send(const uint8_t*p,unsigned n){assert(n<=sizeof reply);memcpy(reply,p,n);return 0;}
#include "storage_protocol.h"
static unsigned command(unsigned cmd,const uint32_t*v,unsigned count){uint8_t p[32]={125,65,70,1,0,1,0};p[4]=cmd;for(unsigned i=0;i<count;i++)af_put7(p+7+i*5,v[i]);assert(af_store_sysex(p,7+count*5));return reply[8];}
int main(void){memset(memory,0x5a,sizeof memory);af_store=(af_storage){0,af_flash_read,erase,program,77,0};assert(command(1,0,0)==0);assert(!erases&&!programs);
 assert(command(2,0,0)==0);uint32_t nonce=af_backup.nonce;uint32_t provision[4]={nonce,0,AF_BACKUP_BYTES,0x434f4e46};assert(command(4,provision,4)==3);assert(!erases);
 uint32_t rd[3]={nonce+1,0,128};assert(command(3,rd,3)==3);rd[0]=nonce;rd[1]=128;assert(command(3,rd,3)==3);assert(!erases);
 for(unsigned off=0;off<AF_BACKUP_BYTES;off+=128){rd[1]=off;assert(command(3,rd,3)==0);}assert(!erases&&!programs);
 provision[1]=(~af_backup.crc)^1;assert(command(4,provision,4)==3);assert(!erases);
 provision[1]=~af_backup.crc;memory[AF_STORE_FIRST]^=1;assert(command(4,provision,4)==5);assert(!erases);memory[AF_STORE_FIRST]^=1;
 assert(command(2,0,0)==0);rd[0]=provision[0]=af_backup.nonce;for(unsigned off=0;off<AF_BACKUP_BYTES;off+=128){rd[1]=off;assert(command(3,rd,3)==0);}provision[1]=~af_backup.crc;
 assert(command(4,provision,4)==0);assert(erases==33&&programs==1&&af_store.mounted&&!paused);assert(command(4,provision,4)==3);assert(erases==33);
 for(unsigned a=0;a<sizeof memory;a++)if(!af_storage_range(a,1))assert(memory[a]==0x5a);
 puts("storage protocol incomplete/nonce/CRC/change gates, full backup and guarded provision passed");return 0;}
