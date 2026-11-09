/* Abandoned desktop-port experiment. Not the canonical Animate kernel and not
 * included by the runtime/build. Retained solely as a source comparison. */
#ifndef ANIMATE_VOICE_H
#define ANIMATE_VOICE_H
#include <stdint.h>
/* Borrowed immutable first-channel PCM. Lifetime must cover every render using it.
 * No ownership, decoder, file import, instrument count or asset capacity implied.
 * Float playhead indexing is supported through 2^24 frames, not a storage promise. */
typedef struct {const float *samples;uint32_t length;} av_source;
enum {AV_WAVETABLE,AV_SAMPLE};
enum {AV_LOW24,AV_LOW12,AV_HIGH,AV_BAND,AV_MOOG};
typedef struct {float rate,amount;uint8_t shape,sync,division;} av_lfo_params;
typedef struct {
 float coarse,fine,level,pan,sample_start,loop_start,loop_end;
 float cutoff,resonance,attack,decay,sustain,release;
 uint8_t source_type,filter_type;
 av_lfo_params wt_lfo;
} av_slot_params;
typedef struct {av_slot_params slot[4];float x,y;av_lfo_params lfo_x,lfo_y;} av_patch;
typedef struct {
 av_source source;av_slot_params params;uint32_t start,loop,end;
 float pitch_ratio,increment_factor,attack,decay,release,filter_f,filter_q;
 float moog_p,moog_k,moog_rez,lfo_increment;
} av_prepared_slot;
typedef struct {av_prepared_slot slot[4];av_patch params;float sample_rate,x_increment,y_increment;} av_prepared;
typedef struct {float phase,held;} av_lfo_state;
typedef struct {av_lfo_state x,y;float x_smooth,y_smooth;uint32_t random;} av_vector_state;
typedef struct {
 float position,envelope,band,low,band2,low2;
 float moog[4],previous[4];av_lfo_state lfo;uint8_t stage,looped;
} av_slot_state;
typedef struct {av_slot_state slot[4];float frequency,velocity;uint32_t random;uint8_t active;} av_voice;
void av_patch_init(av_patch *patch);
/* Prepare whenever parameters, source buffers, render rate or tempo change.
 * Rejects invalid source lengths/rate; other numeric inputs sanitize to ranges.
 * Never call while another thread reads this prepared object. */
int av_prepare(av_prepared *out,const av_patch *patch,const av_source sources[4],float sample_rate,float tempo);
void av_vector_init(av_vector_state *state,float x,float y,uint32_t seed);
void av_vector_tick(av_vector_state *state,const av_prepared *prepared,float weights[4]);
void av_voice_init(av_voice *voice,uint32_t seed);
/* Retrigger resets playheads/envelopes; filter/LFO history persists until init. */
void av_voice_on(av_voice *voice,const av_prepared *prepared,float midi_note,float velocity);
void av_voice_off(av_voice *voice);
void av_voice_tick(av_voice *voice,const av_prepared *prepared,const float weights[4],float stereo[2]);
/* Low-level source reader for differential/bounds tests; bypasses filter/envelope. */
float av_source_tick(av_slot_state *state,const av_prepared_slot *slot,float frequency,float lfo_value);
float av_lfo_rate(const av_lfo_params *params,float tempo);
#endif
