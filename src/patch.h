#ifndef ANIMATE_PATCH_H
#define ANIMATE_PATCH_H
#include <stdint.h>
enum {AF_PATCH_IDLE,AF_PATCH_LOAD,AF_PATCH_SAVE};
/* Browser entries: 0..5 original sounds, 6..21 factory sounds with sequences,
 * 22..37 user slots 1..16. Origins 38/39 mark an imported patch / a session. */
#define AF_PATCH_ORIGINALS 6u
#define AF_PATCH_FACTORY 22u
#define AF_PATCH_ENTRIES 38u
#define AF_PATCH_IMPORTED 38u
#define AF_PATCH_SESSION 39u
/* Name of browser entry 0..AF_PATCH_FACTORY-1. */
const char *af_patch_factory_name(unsigned index);
enum {AF_PATCH_READY,AF_PATCH_SAVED,AF_PATCH_LOADED,AF_PATCH_ERROR,AF_PATCH_BUSY};
enum {AF_PATCH_NOTICE_NONE,AF_PATCH_NOTICE_CONFIRM,AF_PATCH_NOTICE_STORAGE,AF_PATCH_NOTICE_INVALID,AF_PATCH_NOTICE_RANDOM};
void af_patch_reset(void);
void af_patch_changed(void);
unsigned af_patch_dirty(void);
unsigned af_patch_revision(void);
unsigned af_patch_status(void);
unsigned af_patch_notice(void);
unsigned af_patch_selection(void);
unsigned af_patch_current(void);
void af_patch_origin(unsigned index);
unsigned af_patch_confirm_kind(void);
unsigned af_patch_occupied(void);
void af_patch_slots(unsigned mask);
int af_patch_select(unsigned index);
int af_patch_load(unsigned index);
int af_patch_save(unsigned slot);
int af_patch_confirm(void);
void af_patch_cancel(void);
unsigned af_patch_pending(void);
unsigned af_patch_slot(void);
unsigned af_patch_request_id(void);
unsigned af_patch_bytes(void);
unsigned char *af_patch_buffer(void);
/* Backend completion: 1 acknowledged,0 stale/no matching operation.
 * Load copies bytes into buffer first; save succeeds only after durable commit.
 * Invalid loaded payload is acknowledged but sets ERROR and preserves sound. */
int af_patch_complete(unsigned id,int success,unsigned bytes);
/* Compact codec, all sound/music settings; excludes sequence and step locks. */
unsigned af_patch_export(void *destination,unsigned capacity);
int af_patch_validate(const void *bytes,unsigned size);
int af_patch_import(const void *bytes,unsigned size);
#endif
