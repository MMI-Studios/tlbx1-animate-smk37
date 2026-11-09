/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AF_STORAGE_H
#define AF_STORAGE_H
#include <stdint.h>
#define AF_STORE_SLOTS 16u
#define AF_STORE_MAX 1024u
#define AF_STORE_FIRST 0x9d000u
#define AF_STORE_END 0xbd000u
#define AF_STORE_MARKER 0xfe000u
enum {AF_STORE_OK=0,AF_STORE_UNOWNED=-1,AF_STORE_IO=-2,AF_STORE_INVALID=-3,AF_STORE_EMPTY=-4};
typedef struct {
 void *user;
 int (*read)(void*,uint32_t,void*,unsigned);
 int (*erase)(void*,uint32_t);
 int (*program)(void*,uint32_t,const void*,unsigned);
 uint32_t unit;
 unsigned mounted;
} af_storage;
int af_storage_range(uint32_t address,unsigned bytes);
int af_storage_mount(af_storage *store,uint32_t unit);
/* Explicit confirmed first Save may claim only a completely erased region. */
int af_storage_claim_erased(af_storage *store,uint32_t unit);
/* Explicit destructive provisioning only after owner's backup/confirmation.
 * Never called implicitly by mount/save/load or boot. */
int af_storage_initialize(af_storage *store,uint32_t unit);
unsigned af_storage_slots(af_storage *store);
int af_storage_save(af_storage *store,unsigned slot,const void *data,unsigned bytes);
int af_storage_load(af_storage *store,unsigned slot,void *data,unsigned capacity);
/* Device settings: append-only 16-byte records after the ownership header in the
 * marker sector (never touches patch slots). Only on a mounted (owned) store. */
#define AF_SETTING_POLYPHONY 1u
#define AF_SETTING_RECORDS ((4096u-256u)/16u)
int af_storage_setting_load(af_storage *store,uint32_t key,uint32_t *value);
int af_storage_setting_save(af_storage *store,uint32_t key,uint32_t value);
#endif
