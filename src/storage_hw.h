/* SPDX-License-Identifier: GPL-3.0-only
 * Included by the board entry only. RAM-only flash driver retained verbatim. */
#include "storage.h"
#include "patch.h"
#include "smk37_public_profile.h"
#include "smk37_flash_profile.h"
static af_storage af_store;
static uint8_t af_store_device,af_store_claiming;
static volatile unsigned af_storage_busy;
#define FL_RANGE_OK(a,n) (af_store_device&&(af_store.mounted||af_store_claiming)&&(uint32_t)(n)>0&&(((uint32_t)(a)>=AF_STORE_FIRST&&(uint32_t)(a)<AF_STORE_END&&(uint32_t)(n)<=AF_STORE_END-(uint32_t)(a))||((uint32_t)(a)>=AF_STORE_MARKER&&(uint32_t)(a)<AF_STORE_MARKER+4096&&(uint32_t)(n)<=AF_STORE_MARKER+4096-(uint32_t)(a))))
#include "fm1_flash.h"
static uint32_t af_store_crc_step(uint32_t c,const uint8_t*p,unsigned n){while(n--){c^=*p++;for(unsigned k=0;k<8;k++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return c;}
static int af_flash_read(void*u,uint32_t a,void*p,unsigned n){(void)u;if(a>0x100000||n>0x100000-a)return -1;uint8_t*d=p;while(n){unsigned k=n>256?256:n;uint32_t f=irq_save();int rc=FL_FAR(fl_read_ram)(a,d,k);irq_restore(f);if(rc)return rc;a+=k;d+=k;n-=k;fm1_wdt_feed();}return 0;}
static int af_flash_erase(void*u,uint32_t a){(void)u;if(!FL_RANGE_OK(a,4096)||a%4096)return -1;uint32_t took;fm1_wdt_feed();return fl_erase4k(a,&took);}
static int af_flash_program(void*u,uint32_t a,const void*p,unsigned n){(void)u;if(!FL_RANGE_OK(a,n))return -1;fm1_wdt_feed();return fl_write(a,p,n);}
static void af_store_probe(void){uint8_t trim[556],block[256];uint32_t f=irq_save(),id=FL_FAR(fl_jedec_ram)();irq_restore(f);af_store=(af_storage){0,af_flash_read,af_flash_erase,af_flash_program,0,0};
 if(!smk37_flash_supported_id(id)||af_flash_read(0,0xc1000,trim,sizeof trim)||!smk37_trim_keys_valid(trim,sizeof trim))return;
 uint32_t c=~0u;for(unsigned a=0;a<0x4000;a+=256){if(af_flash_read(0,a,block,256))return;c=af_store_crc_step(c,block,256);}if(~c!=SMK37_PUBLIC_HEAD_CRC)return;
 af_store_device=1;(void)af_storage_mount(&af_store,~af_store_crc_step(~0u,trim,sizeof trim));af_patch_slots(af_storage_slots(&af_store));
 uint32_t poly;if(af_storage_setting_load(&af_store,AF_SETTING_POLYPHONY,&poly)==AF_STORE_OK){fm1_irq_off();(void)af_polyphony_restore(poly);fm1_irq_on();}}
static void af_store_pause(void){fm1_irq_off();af_storage_busy=1;memset(smk37_dma,0,sizeof smk37_dma);fm1_irq_on();}
static void af_store_resume(void){fm1_irq_off();af_storage_busy=0;fm1_irq_on();}
static void af_store_service(void){unsigned op=af_patch_pending();if(!op)return;unsigned id=af_patch_request_id(),slot=af_patch_slot(),bytes=af_patch_bytes();int rc=AF_STORE_UNOWNED;
 if(af_store_device){af_store_pause();if(!af_store.mounted&&op==AF_PATCH_SAVE){af_store_claiming=1;(void)af_storage_claim_erased(&af_store,af_store.unit);af_store_claiming=0;}
 rc=op==AF_PATCH_SAVE?af_storage_save(&af_store,slot,af_patch_buffer(),bytes):af_storage_load(&af_store,slot,af_patch_buffer(),1024);af_store_resume();}
 unsigned mask=af_storage_slots(&af_store);fm1_irq_off();af_patch_slots(mask);af_patch_complete(id,op==AF_PATCH_SAVE?rc==0:rc>0,op==AF_PATCH_LOAD&&rc>0?(unsigned)rc:bytes);fm1_irq_on();}
/* Persist POLYPHONY once it has settled for 1.5 s and no voice sounds (the write
 * pauses audio). Owned storage only: never claims, initializes or erases slots. */
static unsigned af_poly_seen,af_poly_saved;static uint32_t af_poly_at;
static void af_setting_service(uint32_t ms){unsigned changes=af_polyphony_changes();
 if(changes!=af_poly_seen){af_poly_seen=changes;af_poly_at=ms;return;}
 if(changes==af_poly_saved||ms-af_poly_at<1500u||!af_store_device||!af_store.mounted||af_patch_pending()||ab_active())return;
 af_store_pause();int rc=af_storage_setting_save(&af_store,AF_SETTING_POLYPHONY,af_polyphony());af_store_resume();
 if(rc!=AF_STORE_IO)af_poly_saved=changes;else af_poly_at=ms;}
#include "storage_protocol.h"
