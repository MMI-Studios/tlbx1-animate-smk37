/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ANIMATE_BASELINE_BRIDGE_H
#define ANIMATE_BASELINE_BRIDGE_H
#include <stdint.h>
enum ab_slot_parameter { AB_SLOT,AB_SOURCE,AB_LEVEL,AB_TUNE,AB_CUTOFF,AB_RESONANCE,
 AB_BYPASS,AB_ATTACK,AB_DECAY,AB_SUSTAIN,AB_RELEASE,AB_X_SHAPE,AB_X_RATE,
 AB_X_DEPTH,AB_Y_SHAPE,AB_Y_RATE,AB_Y_DEPTH,AB_START,AB_END,
 AB_POSITION_SHAPE,AB_POSITION_RATE,AB_POSITION_DEPTH,AB_SOLO,AB_SLOT_COUNT };
/* Fixed44100 Hz,32-frame control blocks. Initialize once per fresh module.
 * Params retain original donor integer units/IDs, not desktop UI metadata.
 * Not reentrant: one audio owner serializes edits, notes and rendering. */
void ab_init(void);
void ab_preset(uint32_t preset);
int ab_param(uint32_t id,int value);
int ab_param_valid(uint32_t id,int value);
int ab_slot(uint32_t slot,uint32_t id,int value);
int ab_param_get(uint32_t id); /* INT32_MIN means invalid ID. */
int ab_slot_get(uint32_t slot,uint32_t id);
/* Logical IDs from animate_parameters.h; invalid/unsupported values return0.
 * Edits keep float precision. Getters report base patch, never a lane lock. */
int ab_slot_parameter(unsigned slot,unsigned id,float value);
float ab_slot_parameter_get(unsigned slot,unsigned id);
uint32_t ab_slot_parameter_mask(unsigned slot);
/* Bits0..19 activate logical edits; bit20 preserves explicit filter bypass. */
int ab_slot_parameter_mask_set(unsigned slot,uint32_t mask);
int ab_voice_parameter(uint32_t handle,unsigned slot,unsigned id,float value);
void ab_voice_parameters_clear(uint32_t handle);
uint32_t ab_note_on(uint32_t note,uint32_t velocity);
uint32_t ab_owned_note_on(uint32_t note,uint32_t velocity);
void ab_note_off(uint32_t handle);
int ab_polyphony(uint32_t count);
int ab_render(int32_t *stereo_q15,uint32_t frames);
int32_t *ab_output(void); /*256int32 =128stereo frames for WASM clients. */
uint32_t ab_active(void);
int ab_tempo(float bpm); /* Sync LFO uses float; inherited FX uses nearest BPM. */
int ab_master(float gain); /*0..1 maps unchanged Q12 host master; unity default. */
void ab_release_all(void); /*Release envelopes, retain FX tails. */
int ab_voice_pitch(uint32_t handle,int32_t pitch100); /*Absolute hundredths of a cent: MIDI69=690000. */
void ab_pitch_bend(int value); /*Signed14-bit, inherited smoothing, +/-2semitones. */
void ab_modwheel(unsigned value); /*CC1 source for inherited modulation matrix. */
int ab_vector(float x,float y); /*Normalized0..1, preserves float base coordinates. */
float ab_vector_get(unsigned axis);
/* Axis0X/1Y; fields shape0(sine,tri,square,saw,S&H),rateHz1,depth2,sync3,division4. */
int ab_vector_lfo(unsigned axis,unsigned field,float value);
float ab_vector_lfo_get(unsigned axis,unsigned field);
unsigned ab_vector_lfo_mask(unsigned axis);
int ab_vector_lfo_mask_set(unsigned axis,unsigned mask);
void ab_shed_oldest(void); /*Melodee release-first overload selection and block fade. */
uint32_t ab_shed_count(void); /*Actual overload fade requests, not active count changes. */
void ab_panic(void); /*Immediate voice stop for project replacement. FX state retained. */
/* Optional parallel voice rendering: submit(fn,ctx) runs fn(ctx) on another
 * core and returns 1 (0 if unavailable); join() returns 1 when it completed. */
/* Wave bank: four registers (slot SOURCE 0..3), each holding any bank entry.
 * Event-time only; a load copies one frame from flash into SRAM. */
int ab_wave(unsigned reg,unsigned entry);
unsigned ab_wave_get(unsigned reg);
unsigned ab_wave_count(void);
unsigned ab_wave_frames(unsigned entry);
const char *ab_wave_name(unsigned entry);
void ab_worker_hooks(int (*submit)(void (*)(void *), void *), int (*join)(void));
#endif
