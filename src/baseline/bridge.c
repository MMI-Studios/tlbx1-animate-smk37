/* SPDX-License-Identifier: GPL-3.0-only
 * Animate-only host assembled from the preserved Melodee adaptation.
 * Kernel/assets and common DSP/FX remain included verbatim from that source.
 * Local core/voice/mod/params extracts remove native-engine/UI dispatch only.
 * This portable test bridge is not a board firmware or a sequencer. */
#include <stdint.h>
#if !defined(AB_FREESTANDING) || defined(AB_EXTERNAL_LIBC)
#include <string.h>
#endif
#include "melodee_tables.h"
#if !defined(AB_FREESTANDING) || defined(AB_EXTERNAL_LIBC)
#define memset donor_memset
#define memcpy donor_memcpy
#define memcmp donor_memcmp
#endif
#include "libc.c"
#undef memset
#undef memcpy
#undef memcmp
#include "core.h"
#ifdef AF_HARDWARE
#define ANIMATE_HOT __attribute__((section(".dsp_text"),noinline))
/* The older MCU compiler otherwise outlines even clamp/multiply helpers.
 * Inline these primitives into the SRAM loop, avoiding out-of-range calls. */
#define MELODEE_DSP_INLINE static inline __attribute__((always_inline))
#endif
#include "dsp.c"
/* Cache only coefficients, never a voice's integrator state. Compare actual
 * inputs on every call so parameter locks and live edits need no invalidation. */
static tsvf_t bridge_filter_cache[4];
static int32_t bridge_filter_cut[4], bridge_filter_res[4];
static uint8_t bridge_filter_valid[4];
static inline void bridge_filter_coefficients(tsvf_t *out,unsigned slot,int32_t cut,int32_t res){
 if(!bridge_filter_valid[slot]||bridge_filter_cut[slot]!=cut||bridge_filter_res[slot]!=res){
  tsvf_coef(&bridge_filter_cache[slot],cut,res);
  bridge_filter_cut[slot]=cut;bridge_filter_res[slot]=res;bridge_filter_valid[slot]=1;
 }
 *out=bridge_filter_cache[slot];
}
#define ANIMATE_FILTER_COEFFICIENTS(out,slot,cut,res) bridge_filter_coefficients(out,slot,cut,res)
#include "eng_animate.c"
#include "../animate_parameters.h"
#include "../animate_math.h"
#include "resources.c"
static animate_part_t *bridge_part;
static ANIMATE_HOT animate_part_t *animate_part(uint32_t part){
 if(!bridge_part){
  /* SRAM -> XIP can exceed the MCU's relative-call range. Allocate through
   * an absolute call only on first use; resource allocations never move. */
  void *(*volatile get)(uint32_t,uint32_t)=resource_get;
  bridge_part=get(RES_ENGINE0+part%NPART,sizeof(animate_part_t));
 }
 return bridge_part;
}
static uint32_t bridge_polyphony=8;
static uint32_t bridge_voice_cap(const track_t*t){(void)t;return bridge_polyphony;}
static uint32_t bridge_owned_note;
static float bridge_tempo=120.f;
static engine_t bridge_engine;
static const engine_t *const ENGINES[NENGINES]={[ENGI_ANIMATE]=&bridge_engine};
static uint32_t eng_idx(uint32_t id){(void)id;return ENGI_ANIMATE;}
static int eng_state_prepare(const track_t*t){return animate_part((uint32_t)(t-trk))!=0;}
static void eng_state_clear(uint32_t part){resource_release(RES_ENGINE0+part);bridge_part=0;}
#include "params.c"
#include "extensions.c"
#include "mod.c"
#include "voice.c"
static uint32_t shed_count;
#include "audio_shed_voice.h"
#include "slicer.c"
#include "fx.c"
/* The bridge caller supplies notes directly; no donor sequencer/UI events. */
static void events_block(uint32_t n){(void)n;}

void ab_preset(uint32_t preset){
 track_t*t=&trk[0];preset%=NELEM(ANIMATE_PRESETS);const preset_t*p=&ANIMATE_PRESETS[preset];
 t->engine=t->eng_req=ENGI_ANIMATE;t->preset=preset;
 for(unsigned i=0;i<8;i++)t->p[P_E0+i]=p->e[i];
 t->p[P_ATK]=p->env[0];t->p[P_DEC]=p->env[1];t->p[P_SUS]=p->env[2];t->p[P_REL]=p->env[3];
 t->p[P_ED_FLT]=p->fenv;t->p[P_VOICE]=p->mono?V_LEGATO:V_POLY;
 static const uint8_t fallback[4]={0,24,28,36};
 for(unsigned i=0;i<4;i++)t->p[P_DIST+i]=i==2?0:p->fx[i]?p->fx[i]-1:fallback[i];
 animate_preset_loaded(t,preset);
 bridge_extensions_reset();
}
void ab_init(void){
 /* Fresh process/module is the supported reset boundary until all FX reset
  * ownership is integrated. Clear track and defaults before the first note. */
 memset(trk,0,sizeof trk);memset(&song,0,sizeof song);
 bridge_engine=ENG_ANIMATE;bridge_engine.cap=bridge_voice_cap;
#ifdef AB_REFERENCE_KERNEL
 bridge_engine.render=bridge_render; /* per-sample reference, tests only */
#else
 bridge_engine.render=ak_render;bridge_engine.post=ak_post;
#endif
 ak_tables_init();
 for(unsigned i=0;i<G_COUNT;i++)song.g[i]=GP[i].def;
 for(unsigned i=0;i<P_E0;i++)trk[0].p[i]=TP[i].def;
 song.master_q12=4096;ab_preset(0);
}
int ab_param_valid(uint32_t id,int value){if(id>=P_COUNT)return 0;const param_desc_t*d=track_desc(trk,id);return value>=d->min&&value<=d->max;}
int ab_param(uint32_t id,int value){if(!ab_param_valid(id,value))return 0;trk[0].p[id]=value;return 1;}
int ab_slot(uint32_t slot,uint32_t id,int value){if(slot>3||id>=AN_COUNT)return 0;const param_desc_t*d=&AN_DESC[id];if(value<d->min||value>d->max)return 0;animate_config_of(trk)->selected=slot;int16_t*p=animate_value(trk,id);if(!p)return 0;*p=value;if(id==AN_BYPASS){if(value)bridge_masks[slot]|=1u<<20;else bridge_masks[slot]&=~(1u<<20);}return 1;}
int ab_param_get(uint32_t id){return id<P_COUNT?trk[0].p[id]:INT32_MIN;}
int ab_slot_get(uint32_t slot,uint32_t id){if(slot>3||id>=AN_COUNT)return INT32_MIN;animate_config_t*c=animate_config_of(trk);int selected=c->selected;c->selected=slot;int16_t*p=animate_value(trk,id);int value=p?*p:INT32_MIN;c->selected=selected;if(id==AN_BYPASS){if(bridge_masks[slot]&(1u<<20))value=1;else if(bridge_masks[slot]&((1u<<13)|(1u<<14)|(1u<<15)))value=0;}return value;}
uint32_t ab_note_on(uint32_t note,uint32_t vel){if(note>127||vel>127||!vel)return 0;trk_note_on(trk,note,vel);for(unsigned i=0;i<NVOICE;i++)if(trk[0].v[i].active&&trk[0].v[i].age==vage)return (vage<<4)|i;return 0;}
/* Lane/keybed ownership intentionally differs from donor same-pitch reuse.
 * Only POLY accepts this API; mono/unison ownership needs its own group model. */
uint32_t ab_owned_note_on(uint32_t note,uint32_t vel){if(trk[0].p[P_VOICE]!=V_POLY)return 0;bridge_owned_note=1;uint32_t h=ab_note_on(note,vel);bridge_owned_note=0;return h;}
void ab_note_off(uint32_t handle){unsigned i=handle&15;voice_t*v=&trk[0].v[i];if(handle&&v->active&&v->age==(handle>>4)){v->gate=0;v->stage=3;}}
int ab_polyphony(uint32_t n){if(n<1||n>NVOICE)return 0;for(unsigned i=0;i<NVOICE;i++)if(trk[0].v[i].active)return 0;bridge_polyphony=n;return 1;}
int ab_render(int32_t*out,uint32_t frames){if(!out||frames%CTL)return 0;for(unsigned i=0;i<frames;i+=CTL)mix_block(out+i*2,CTL);return 1;}
static int32_t bridge_output[256];
int32_t *ab_output(void){return bridge_output;}
uint32_t ab_active(void){uint32_t n=0;for(unsigned i=0;i<NVOICE;i++)n+=trk[0].v[i].active!=0;return n;}
int ab_tempo(float bpm){if(!(bpm>=1.f&&bpm<=1000.f))return 0;bridge_tempo=bpm;song.g[G_BPM]=(int16_t)(bpm+.5f);return 1;}
int ab_master(float gain){if(!(gain>=0.f&&gain<=1.f))return 0;song.master_q12=(uint32_t)(gain*4096.f+.5f);return 1;}
void ab_release_all(void){trk_all_off(trk);}
int ab_voice_pitch(uint32_t handle,int32_t pitch100){if(!bridge_handle_valid(handle)||pitch100<0||pitch100>1270000)return 0;voice_t*v=&trk[0].v[handle&15];int32_t p=pitch100*16/10000,r=pitch100*16-p*10000;v->pitch16=v->pitch_cur=p;v->scale_fine=r*2367/1600000;return 1;}
void ab_pitch_bend(int value){value=clamp(value,-8192,8191);trk[0].bend_raw=value;midi_bend_target[0]=value*512/(value<0?8192:8191);}
void ab_modwheel(unsigned value){trk[0].mw=(uint8_t)(value>127?127:value);}
/* Retain the public ABI, but use Melodee's release-first, bass-preserving
 * overload policy and one-block fade rather than abruptly zeroing a voice. */
void ab_shed_oldest(void){shed_voice();}
uint32_t ab_shed_count(void){return shed_count;}
void ab_panic(void){trk_all_off(trk);for(unsigned i=0;i<NVOICE;i++){voice_t*v=&trk[0].v[i];v->active=v->gate=v->stage=0;v->env=v->env_out=0;}memset(bridge_lock_masks,0,sizeof bridge_lock_masks);}
