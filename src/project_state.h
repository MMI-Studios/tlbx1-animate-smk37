#ifndef ANIMATE_PROJECT_STATE_H
#define ANIMATE_PROJECT_STATE_H
#include "parameter_codec.h"
#include "sequence.h"
enum {APS_STEPS,APS_PARAMS,APS_BOTH};
enum {APS_SOUND,APS_SELECTED,APS_PAGE,APS_ALL};
typedef struct {
 as_sequence sequence;
 ap_lock locks[AS_LANES][AS_STEPS][4];
 float base[4][20],glide[AS_LANES],note_amount,param_amount;
 uint32_t mask[4],selected[4],random;
 uint32_t export_header[4];
} aps_state;
/* Baseline bridge supplies actual base settings. Desktop defaults are not
 * installed by this module; no synthesis/output implementation is involved. */
int aps_init(aps_state *s,unsigned rate,const float base[4][20]);
int aps_base_set(aps_state *s,unsigned slot,unsigned parameter,float value);
int aps_lock_set(aps_state *s,unsigned lane,unsigned step,unsigned slot,unsigned parameter,float value);
float aps_value(const aps_state *s,unsigned lane,unsigned step,unsigned slot,unsigned parameter);
void aps_select(aps_state *s,unsigned step,int selected);
/* lane/slot -1 means all. All scopes apply to the instrument's single128-step
 * pattern. Sound scope modifies base values only. Notes are MIDI36..84; the
 * caller's shared musical-input mapper applies selected scale/chord semantics. */
int aps_randomize(aps_state *s,unsigned mode,unsigned scope,int lane,int slot,unsigned page);
/* Bounded live-work variant: first/count address the flattened ten-lane step
 * array (0..1279). Step mask bits: note,velocity,gate,probability,enabled.
 * Call successive chunks with unchanged routing to cover the full scope. */
int aps_randomize_range(aps_state *s,unsigned mode,unsigned scope,int lane,int slot,unsigned page,unsigned step_mask,unsigned first,unsigned count);
void aps_clear(aps_state *s,unsigned lane,unsigned first,unsigned count,int notes,int parameters);
int aps_copy(aps_state *s,unsigned from_lane,unsigned from_step,unsigned to_lane,unsigned to_step,unsigned count);
/* One serialized owner must read every export chunk before editing state.
 * Import receives immutable complete bytes, validates everything before commit,
 * and needs no second live lock-array allocation. Storage transport is separate. */
unsigned aps_project_size(aps_state *s);
unsigned aps_project_read(const aps_state *s,unsigned offset,void *out,unsigned count);
int aps_project_import(aps_state *s,const void *bytes,unsigned size);
int aps_project_validate(const void *bytes,unsigned size);
#endif
