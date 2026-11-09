/* SPDX-License-Identifier: GPL-3.0-only */
/* TLBX Animate POC: four original tables with bilinear vector mixing.
 * This is the oscillator/vector slice, not full TLBX filter/envelope parity.
 * Uses the host engine's amplitude envelope and track modulation/effects. */
#ifndef ANIMATE_HOT
#define ANIMATE_HOT
#endif
#ifndef ANIMATE_FILTER_COEFFICIENTS
#define ANIMATE_FILTER_COEFFICIENTS(out,slot,cut,res) tsvf_coef(out,cut,res)
#endif
#include "animate_poc_tables.h"
#include "animate_controls.h"
#include "animate_user_vector.h"

/* Exact replacements for C's truncating integer division by constants. The MCU
 * compiler keeps constant divisions as hardware divides (see dsp.c amp_at);
 * the per-sample voice loop below issued about thirty per sample and voice.
 * Reciprocals are exact for every magnitude below 2^31 (m*d-2^sh times 2^31 is
 * below 2^sh) and keep sign/truncation; results are bit-identical. */
MELODEE_DSP_INLINE uint32_t animate_udiv(uint32_t n,uint32_t m,unsigned sh){return (uint32_t)(((uint64_t)n*m)>>sh);}
MELODEE_DSP_INLINE int32_t animate_sdiv(int32_t n,uint32_t m,unsigned sh){return n<0?-(int32_t)animate_udiv((uint32_t)-n,m,sh):(int32_t)animate_udiv((uint32_t)n,m,sh);}
#define ANIMATE_DIV127(n) animate_sdiv((n),0x81020409u,38)     /* n / 127 */
#define ANIMATE_DIV1000(n) animate_sdiv((n),0x83126e98u,41)    /* n / 1000 */
#define ANIMATE_UDIV65535(n) animate_udiv((n),0x80008001u,47)  /* n / 65535u */
/* Signed power-of-two division, truncating toward zero exactly like '/'. */
#define ANIMATE_SDIV_POW2(n,k) (((n)+(((n)>>31)&((1<<(k))-1)))>>(k))

static ANIMATE_HOT int32_t animate_lookup(uint32_t slot, uint32_t phase, uint32_t start, uint32_t end, uint32_t offset)
{
    uint32_t len = end-start;
    uint64_t pos = (uint64_t)phase * len;
    /* Both phase index and position offset are below len. One subtraction
     * replaces a variable division in every slot/sample without changing wrap. */
    uint32_t i = (uint32_t)(pos >> 32)+offset;
    if(i>=len)i-=len;
    uint32_t frac = (uint32_t)pos >> 16;
    const int32_t *table = animate_poc_tables[slot]+start;
    /* Q31 source to Q15 before interpolation; original assets remain intact. */
    int32_t a = ANIMATE_SDIV_POW2(table[i],16), b = ANIMATE_SDIV_POW2(table[i + 1u == len ? 0 : i + 1u],16);
    /* |b-a| <=65535 and frac<=65535: the unsigned product fits32 bits.
     * Restore its sign after shifting to retain C's truncation toward zero. */
    int32_t delta=b-a;
    uint32_t magnitude=(uint32_t)(delta<0?-delta:delta)*frac;
    int32_t interpolation=(int32_t)(magnitude>>16);
    return a+(delta<0?-interpolation:interpolation);
}

static inline __attribute__((always_inline)) int32_t animate_lfo(uint32_t phase, int shape)
{
 return shape==1?osc_tri(phase):shape==2?(int32_t)(phase>>16)-32768:
        shape==3?(phase&0x80000000u?-32767:32767):osc_sine(phase);
}

static void animate_note_on(track_t *t, voice_t *v)
{
    animate_part_t *part=animate_part((uint32_t)(t-trk));
    if(part)memset(part->slot[v-t->v],0,sizeof part->slot[0]);
    animate_config_of(t);
    v->ph[0] = v->ph[1] = 0;v->ph[2]=0x40000000u;
    v->s[0] = t->p[P_E0] * 32767 / 127;
    v->s[1] = t->p[P_E1] * 32767 / 127;
}

static ANIMATE_HOT void animate_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    animate_part_t *part=animate_part((uint32_t)(t-trk));
    if(!part)return;
    animate_config_t *c=animate_config_of(t);
    animate_slot_voice_t *sv=part->slot[v-t->v];
    uint32_t lfo = v->ph[1], lfo_y=v->ph[2];
    /* RATE 0 stops; 127 = 4 Hz. A quadrature sine traces the vector plane. */
    uint32_t rate = (uint32_t)t->p[P_E2] * (uint32_t)(4294967296.0 / (127.0 * FS));
    rate *= 4u;
    uint32_t rate_y=(uint32_t)c->yrate*(uint32_t)(4294967296.0/(127.0*FS))*4u;
    uint32_t inc[4],start[4],end[4],position_rate[4];
    animate_motion_t *motion=&animate_motion[(uint32_t)(t-trk)%NPART];
    int32_t attack[4],decay[4],release[4];tsvf_t filter[4];
    for(unsigned s=0;s<4;s++){
      int16_t *p=c->slot[s];inc[s]=cents_inc(m->pitch16+p[AN_TUNE-1]*16,0,v->scale_fine);
      int16_t *w=motion->slot[s];uint32_t len=animate_poc_lengths[p[AN_WAVE-1]];
      start[s]=(uint32_t)w[0]*len/127u;if(start[s]>=len)start[s]=len-1;
      end[s]=(uint32_t)w[1]*len/127u;if(end[s]<=start[s])end[s]=start[s]+1;
      position_rate[s]=(uint32_t)w[3]*(uint32_t)(4294967296.0/(127.0*FS))*4u;
      attack[s]=32767/(1+(uint32_t)p[AN_ATK-1]*p[AN_ATK-1]*FS/4096u)+1;
      decay[s]=32767/(1+(uint32_t)p[AN_DEC-1]*p[AN_DEC-1]*FS/4096u)+1;
      release[s]=32767/(1+(uint32_t)p[AN_REL-1]*p[AN_REL-1]*FS/4096u)+1;
      ANIMATE_FILTER_COEFFICIENTS(&filter[s],s,p[AN_CUT-1]<<8,p[AN_RES-1]);
    }
    /* Loop invariants: control values are not written while a block renders. */
    const int32_t depth = t->p[P_E3] * 16383 / 127, ydepth = c->ydepth * 16383 / 127;
    const int32_t base_x = t->p[P_E0] * 32767 / 127, base_y = t->p[P_E1] * 32767 / 127;
    const int solo = animate_solo[(uint32_t)(t-trk)%NPART] != 0;
    int32_t sustain[4], level[4], mix[4];uint32_t max_offset[4];
    for(unsigned s=0;s<4;s++){
      const int16_t *p=c->slot[s],*w=motion->slot[s];
      sustain[s]=p[AN_SUS-1]*32767/127;level[s]=p[AN_LEVEL-1];mix[s]=t->p[P_E4+s];
      max_offset[s]=(end[s]-start[s]-1u)*(uint32_t)w[4]/127u;
    }
    for (uint32_t i = 0; i < n; i++) {
        int32_t lx=c->xshape==1?osc_tri(lfo):c->xshape==2?(int32_t)(lfo>>16)-32768:c->xshape==3?(lfo&0x80000000u?-32767:32767):osc_sine(lfo);
        int32_t ly=c->yshape==1?osc_tri(lfo_y):c->yshape==2?(int32_t)(lfo_y>>16)-32768:c->yshape==3?(lfo_y&0x80000000u?-32767:32767):osc_sine(lfo_y);
        int32_t x = clamp(base_x + mulq15(lx, depth), 0, 32767);
        int32_t y = clamp(base_y + mulq15(ly, ydepth), 0, 32767);
        /* TLBX's 0.999/0.001 smoothing, with at least one Q15 unit of progress. */
        int32_t dx = x - v->s[0], dy = y - v->s[1];
        int32_t qx = ANIMATE_DIV1000(dx), qy = ANIMATE_DIV1000(dy);
        v->s[0] += qx + (dx && qx == 0 ? (dx > 0 ? 1 : -1) : 0);
        v->s[1] += qy + (dy && qy == 0 ? (dy > 0 ? 1 : -1) : 0);
        x = v->s[0]; y = v->s[1];
        int32_t weights[4] = {mulq15(32767-x,32767-y), mulq15(x,32767-y),
                        mulq15(32767-x,y), mulq15(x,y)};
        int32_t sample = 0;
        if(solo)
            for(unsigned s=0;s<4;s++)weights[s]=s==(unsigned)c->selected?32767:0;
        for (uint32_t slot = 0; slot < 4; slot++){
            int16_t *p=c->slot[slot];animate_slot_voice_t *s=&sv[slot];
            if(!v->gate)s->stage=3;
            if(s->stage==0){s->env+=attack[slot];if(s->env>=32767){s->env=32767;s->stage=1;}}
            else if(s->stage==1){s->env-=decay[slot];int32_t sus=sustain[slot];
              if(s->env<=sus){s->env=sus;s->stage=2;}}
            else if(s->stage==2)s->env=sustain[slot];
            else {s->env-=release[slot];if(s->env<0)s->env=0;}
            int16_t *w=motion->slot[slot];
            uint32_t offset=max_offset[slot]?ANIMATE_UDIV65535((uint32_t)(animate_lfo(s->position_phase,w[2])+32768)*max_offset[slot]):0;
            int32_t value=animate_lookup((uint32_t)p[AN_WAVE-1],s->phase,start[slot],end[slot],offset);
            /* Cancel a live control-edit discontinuity, then decay the correction.
             * Unedited/default rendering remains sample-identical to the POC. */
            if(s->window_ready&&(s->last_start!=w[0]||s->last_end!=w[1]||s->last_shape!=w[2]||s->last_rate!=w[3]||s->last_depth!=w[4]))
                s->edit_offset=s->last_output-value;
            s->last_start=w[0];s->last_end=w[1];s->last_shape=w[2];s->last_rate=w[3];s->last_depth=w[4];s->window_ready=1;
            value=clamp(value+s->edit_offset,-32767,32767);s->last_output=value;
            if(s->edit_offset)s->edit_offset-=ANIMATE_SDIV_POW2(s->edit_offset,7)+(s->edit_offset>0?1:-1);
            s->position_phase+=position_rate[slot];
            if(!p[AN_BYPASS-1])value=tsvf_lp(&filter[slot],value,&s->ic1,&s->ic2);
            value=clamp(value,-32767,32767);
            sample+=ANIMATE_DIV127(ANIMATE_DIV127(mulq15(mulq15(value,s->env),weights[slot])*level[slot])*mix[slot]);
            s->phase+=inc[slot];
        }
        out[i] += voice_amp(mulq15(sample,v->vel*258), m, i) * 2;
        lfo += rate;
        lfo_y+=rate_y;
    }
    v->ph[1] = lfo;v->ph[2]=lfo_y;
}

static int animate_done(track_t*t,voice_t*v){
 if(v->gate)return 0;animate_part_t*p=animate_part((uint32_t)(t-trk));if(!p)return 1;
 for(unsigned s=0;s<4;s++)if(p->slot[v-t->v][s].env)return 0;return 1;
}

static const preset_t ANIMATE_PRESETS[] = {
    {"VECTOR", {64,64,24,64,127,127,127,127}, {5,60,110,55}, 0,0,FX(0,0,0,0)},
    {"SINE", {0,0,0,0,127,127,127,127}, {5,60,110,55}, 0,0,FX(0,0,0,0)},
    {"SAW", {127,0,0,0,127,127,127,127}, {5,60,110,55}, 0,0,FX(0,0,0,0)},
    {"BELL", {0,127,0,0,127,127,127,127}, {0,70,0,55}, 0,0,FX(0,0,0,0)},
    {"VOICE", {127,127,0,0,127,127,127,127}, {5,60,110,55}, 0,0,FX(0,0,0,0)},
    {"USER VECTOR", {64,64,43,94,127,127,127,127}, {5,60,110,55}, 0,0,FX(0,0,0,0)},
};
static const engine_t ENG_ANIMATE = {
    .name="ANIMATE POC", .page_title={"VECTOR","LEVELS"},
    .edit={{"X",F_PCT,0,127,64,0,0},{"Y",F_PCT,0,127,64,0,0},
           {"RATE",F_INT,0,127,24,0,0},{"DEPTH",F_PCT,0,127,64,0,0},
           {"A",F_PCT,0,127,127,0,0},{"B",F_PCT,0,127,127,0,0},
           {"C",F_PCT,0,127,127,0,0},{"D",F_PCT,0,127,127,0,0}},
    .presets=ANIMATE_PRESETS,.npresets=NELEM(ANIMATE_PRESETS),
    .note_on=animate_note_on,.render=animate_render,
    .ownenv=1,.done=animate_done,
    .knob={P_E0,P_E1,P_E2,P_E3},
};
