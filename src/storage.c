/* SPDX-License-Identifier: GPL-3.0-only
 * Adapted from Melodee storage.c / melodee_smk_store.c root-last commits.
 * Copyright (C) 2026 Leo Kuroshita, Hugelton Instruments (original store).
 * Dedicated Animate namespace and fixed A/B sector pairs; sector tails erased. */
#include "storage.h"
#include <string.h>
#define AF_ROOT 0x31504641u
#define AF_OWNER 0x314f4641u
#define AF_LAYOUT 0x20261105u
typedef struct {uint32_t magic,slot,seq,bytes,crc,unit,reserved,hcrc;} af_store_header;
_Static_assert(sizeof(af_store_header)==32,"flash header size");
static uint32_t crc_step(uint32_t c,const void*data,unsigned bytes){const uint8_t*p=data;while(bytes--){c^=*p++;for(unsigned i=0;i<8;i++)c=(c>>1)^((0u-(c&1u))&0xedb88320u);}return c;}
static uint32_t crc(const void*p,unsigned n){return~crc_step(~0u,p,n);}
int af_storage_range(uint32_t a,unsigned n){return n&&((a>=AF_STORE_FIRST&&a<AF_STORE_END&&n<=AF_STORE_END-a)||(a>=AF_STORE_MARKER&&a<AF_STORE_MARKER+4096&&n<=AF_STORE_MARKER+4096-a));}
static uint32_t address(unsigned slot,unsigned copy){return AF_STORE_FIRST+(slot*2+copy)*4096;}
static int valid(af_storage*s,unsigned slot,unsigned copy,af_store_header*out){af_store_header h;uint8_t block[256];uint32_t a=address(slot,copy),c=~0u;
 if(s->read(s->user,a,&h,sizeof h)||h.magic!=AF_ROOT||h.slot!=slot||h.unit!=s->unit||!h.bytes||h.bytes>AF_STORE_MAX||h.hcrc!=crc(&h,28))return 0;
 for(unsigned off=0;off<h.bytes;){unsigned n=h.bytes-off;if(n>sizeof block)n=sizeof block;if(s->read(s->user,a+256+off,block,n))return 0;c=crc_step(c,block,n);off+=n;}
 if(~c!=h.crc)return 0;if(out)*out=h;return 1;}
static int current(af_storage*s,unsigned slot,af_store_header*out){af_store_header a,b;int va=valid(s,slot,0,&a),vb=valid(s,slot,1,&b);if(!va&&!vb)return -1;int copy=vb&&(!va||(b.seq!=a.seq&&b.seq-a.seq<0x80000000u));if(out)*out=copy?b:a;return copy;}
int af_storage_mount(af_storage*s,uint32_t unit){af_store_header h;s->mounted=0;s->unit=unit;if(!s->read||!s->erase||!s->program)return AF_STORE_INVALID;if(s->read(s->user,AF_STORE_MARKER,&h,sizeof h))return AF_STORE_IO;if(h.magic!=AF_OWNER||h.slot!=AF_LAYOUT||h.unit!=unit||h.bytes!=AF_STORE_SLOTS||h.hcrc!=crc(&h,28))return AF_STORE_UNOWNED;s->mounted=1;return AF_STORE_OK;}
int af_storage_initialize(af_storage*s,uint32_t unit){s->mounted=0;s->unit=unit;if(!s->read||!s->erase||!s->program)return AF_STORE_INVALID;
 if(s->erase(s->user,AF_STORE_MARKER))return AF_STORE_IO;
 uint8_t check[256];for(unsigned a=AF_STORE_FIRST;a<AF_STORE_END;a+=4096){if(s->erase(s->user,a))return AF_STORE_IO;for(unsigned off=0;off<4096;off+=256){if(s->read(s->user,a+off,check,256))return AF_STORE_IO;for(unsigned i=0;i<256;i++)if(check[i]!=255)return AF_STORE_IO;}}
 af_store_header h={AF_OWNER,AF_LAYOUT,1,AF_STORE_SLOTS,0,unit,0,0};h.hcrc=crc(&h,28);if(s->program(s->user,AF_STORE_MARKER,&h,sizeof h))return AF_STORE_IO;return af_storage_mount(s,unit);}
int af_storage_claim_erased(af_storage*s,uint32_t unit){uint8_t block[256];s->mounted=0;s->unit=unit;
 if(!s->read||!s->program)return AF_STORE_INVALID;
 for(unsigned sector=0;sector<33;sector++){uint32_t a=sector==32?AF_STORE_MARKER:AF_STORE_FIRST+sector*4096;for(unsigned off=0;off<4096;off+=256){if(s->read(s->user,a+off,block,256))return AF_STORE_IO;for(unsigned i=0;i<256;i++)if(block[i]!=255)return AF_STORE_UNOWNED;}}
 af_store_header h={AF_OWNER,AF_LAYOUT,1,AF_STORE_SLOTS,0,unit,0,0};h.hcrc=crc(&h,28);if(s->program(s->user,AF_STORE_MARKER,&h,sizeof h))return AF_STORE_IO;return af_storage_mount(s,unit);}
unsigned af_storage_slots(af_storage*s){if(!s->mounted)return 0;unsigned mask=0;for(unsigned slot=0;slot<AF_STORE_SLOTS;slot++)if(current(s,slot,0)>=0)mask|=1u<<slot;return mask;}
int af_storage_save(af_storage*s,unsigned slot,const void*data,unsigned bytes){if(!s->mounted)return AF_STORE_UNOWNED;if(slot>=AF_STORE_SLOTS||!data||!bytes||bytes>AF_STORE_MAX)return AF_STORE_INVALID;
 af_store_header old;int copy=current(s,slot,&old);uint32_t a=address(slot,copy==0?1:0);uint8_t verify[256];if(s->erase(s->user,a))return AF_STORE_IO;
 for(unsigned off=0;off<bytes;){unsigned n=bytes-off;if(n>256)n=256;if(s->program(s->user,a+256+off,(const uint8_t*)data+off,n)||s->read(s->user,a+256+off,verify,n)||memcmp(verify,(const uint8_t*)data+off,n))return AF_STORE_IO;off+=n;}
 af_store_header h={AF_ROOT,slot,copy<0?1:old.seq+1,bytes,crc(data,bytes),s->unit,0,0};h.hcrc=crc(&h,28);
 if(s->program(s->user,a,&h,sizeof h)||!valid(s,slot,copy==0?1:0,0))return AF_STORE_IO;return AF_STORE_OK;}
#define AF_SETTING 0x53504641u
typedef struct {uint32_t magic,key,value,crc;} af_setting_record;
_Static_assert(sizeof(af_setting_record)==16,"setting record size");
static int setting_ok(const af_setting_record*r){return r->magic==AF_SETTING&&r->crc==crc(r,12);}
static int setting_blank(const af_setting_record*r){return r->magic==~0u&&r->key==~0u&&r->value==~0u&&r->crc==~0u;}
/* Returns index of first blank record (AF_SETTING_RECORDS when full), negative on I/O error. */
static int setting_scan(af_storage*s,uint32_t key,uint32_t*value,unsigned*found){af_setting_record r[16];*found=0;
 for(unsigned i=0;i<AF_SETTING_RECORDS;i+=16){if(s->read(s->user,AF_STORE_MARKER+256+i*16,r,sizeof r))return AF_STORE_IO;
  for(unsigned k=0;k<16;k++){if(setting_blank(&r[k]))return(int)(i+k);if(setting_ok(&r[k])&&r[k].key==key){*value=r[k].value;*found=1;}}}
 return(int)AF_SETTING_RECORDS;}
int af_storage_setting_load(af_storage*s,uint32_t key,uint32_t*value){if(!s->mounted)return AF_STORE_UNOWNED;if(!value)return AF_STORE_INVALID;unsigned found;uint32_t v=0;int at=setting_scan(s,key,&v,&found);if(at<0)return at;if(!found)return AF_STORE_EMPTY;*value=v;return AF_STORE_OK;}
int af_storage_setting_save(af_storage*s,uint32_t key,uint32_t value){if(!s->mounted)return AF_STORE_UNOWNED;unsigned found;uint32_t old=0;int at=setting_scan(s,key,&old,&found);if(at<0)return at;if(found&&old==value)return AF_STORE_OK;
 if(at>=(int)AF_SETTING_RECORDS){af_store_header h;if(s->read(s->user,AF_STORE_MARKER,&h,sizeof h))return AF_STORE_IO;
  /* Full log: erase and immediately restore the verified ownership header first. */
  if(s->erase(s->user,AF_STORE_MARKER)||s->program(s->user,AF_STORE_MARKER,&h,sizeof h)||af_storage_mount(s,s->unit))return AF_STORE_IO;
  at=0;}
 af_setting_record r={AF_SETTING,key,value,0},check;r.crc=crc(&r,12);uint32_t a=AF_STORE_MARKER+256+(uint32_t)at*16;
 if(s->program(s->user,a,&r,sizeof r)||s->read(s->user,a,&check,sizeof check)||memcmp(&r,&check,sizeof r))return AF_STORE_IO;
 return AF_STORE_OK;}
int af_storage_load(af_storage*s,unsigned slot,void*data,unsigned capacity){if(!s->mounted)return AF_STORE_UNOWNED;if(slot>=AF_STORE_SLOTS||!data)return AF_STORE_INVALID;af_store_header h;int copy=current(s,slot,&h);if(copy<0)return AF_STORE_EMPTY;if(h.bytes>capacity)return AF_STORE_INVALID;if(s->read(s->user,address(slot,copy)+256,data,h.bytes))return AF_STORE_IO;return(int)h.bytes;}
