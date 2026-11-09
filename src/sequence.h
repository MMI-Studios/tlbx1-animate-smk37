#ifndef ANIMATE_SEQUENCE_H
#define ANIMATE_SEQUENCE_H
#include <stdint.h>
#define AS_LANES 10
#define AS_STEPS 128
typedef struct {uint8_t enabled,note,velocity,probability,gate;} as_step;
/* Velocity1..127, probability0..100 and gate1..100. Independent lane events
 * share one instrument transport/grid. No outer four-track timing implied. */
typedef struct {uint64_t sample;uint32_t generation;uint8_t lane,note,velocity,on,step;} as_event;
typedef void (*as_emit)(void *context,const as_event *event);
typedef struct {
 as_step steps[AS_LANES][AS_STEPS];
 uint8_t enabled[AS_LANES],probability[AS_LANES],active[AS_LANES],note[AS_LANES];
 uint32_t generation[AS_LANES],rng[AS_LANES];uint64_t off_at[AS_LANES];
 uint64_t now,next_step;uint32_t rate,remainder;
 uint16_t bpm,length;uint8_t division,position,current,playing;
} as_sequence;
void as_init(as_sequence *s,unsigned sample_rate);
int as_edit(as_sequence *s,unsigned lane,unsigned step,as_step value);
int as_tempo(as_sequence *s,unsigned bpm);
int as_length(as_sequence *s,unsigned length);
int as_division(as_sequence *s,unsigned steps_per_beat);
void as_play(as_sequence *s,uint64_t sample,as_emit emit,void *context);
void as_stop(as_sequence *s,uint64_t sample,as_emit emit,void *context);
void as_advance(as_sequence *s,uint64_t sample,as_emit emit,void *context);
#endif
