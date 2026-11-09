/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ANIMATE_MUSIC_H
#define ANIMATE_MUSIC_H
#include <stdint.h>
enum {ANIMATE_MAP_OFF,ANIMATE_MAP_SNAP,ANIMATE_MAP_WHITE,ANIMATE_MAP_ALL};
enum {ANIMATE_CHORD_OFF,ANIMATE_CHORD_DIA3,ANIMATE_CHORD_DIA7,ANIMATE_CHORD_MAJOR,ANIMATE_CHORD_MINOR,ANIMATE_CHORD_DOM7,ANIMATE_CHORD_MAJ7,ANIMATE_CHORD_MIN7,ANIMATE_CHORD_SUS4,ANIMATE_CHORD_POWER};
enum {ANIMATE_VOICE_CLOSE,ANIMATE_VOICE_OPEN,ANIMATE_VOICE_INV1,ANIMATE_VOICE_INV2,ANIMATE_VOICE_BASS};
typedef struct {unsigned scale,root,map,chord,voicing;int transpose;} animate_music;
unsigned animate_scale_count(void);
const char *animate_scale_name(unsigned scale,int full);
unsigned animate_scale_degrees(unsigned scale);
unsigned animate_scale_family(unsigned scale);
const char *animate_family_name(unsigned family,int full);
/* Pitches in hundredths of a cent: MIDI A4 = 690000. Negative means silent.
 * Scale degree is separate from MIDI identity; microtonal fractions are retained. */
int32_t animate_scale_pitch(const animate_music *music,int degree);
int32_t animate_music_map(const animate_music *music,unsigned midi_note,int octave);
unsigned animate_music_chord(const animate_music *music,int32_t root,int32_t output[4]);
float animate_music_hz(int32_t pitch,unsigned a4_hz);
#endif
