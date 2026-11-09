#ifndef ANIMATE_PARAMETERS_H
#define ANIMATE_PARAMETERS_H
#include <stddef.h>
#include <stdint.h>
/* DESCRIPTIVE desktop inventory only. Do not use desktop_default to initialize
 * the preserved sonic baseline; see docs/SOUND_CONTRACT.md.
 * These are stable logical IDs, not a
 * packed project schema, a DSP implementation, or an agreed hardware limit.
 * UI ranges/increments describe MAIN PANEL controls, not the differing step
 * editor. Increments describe interaction intent; they do not quantize floats.
 * Increment zero means no directly exposed main-panel knob was found. */
typedef enum {
 ANIMATE_SOURCE,ANIMATE_COARSE,ANIMATE_FINE,ANIMATE_LEVEL,ANIMATE_PAN,
 ANIMATE_WT_AMOUNT,ANIMATE_WT_SHAPE,ANIMATE_WT_RATE,ANIMATE_WT_SYNC,
 ANIMATE_WT_DIVISION,ANIMATE_SAMPLE_START,ANIMATE_LOOP_START,ANIMATE_LOOP_END,
 ANIMATE_FILTER_TYPE,ANIMATE_FILTER_CUTOFF,ANIMATE_FILTER_RESONANCE,
 ANIMATE_ATTACK,ANIMATE_DECAY,ANIMATE_SUSTAIN,ANIMATE_RELEASE,
 ANIMATE_SLOT_PARAMETER_COUNT
} animate_parameter_id;
enum { ANIMATE_PARAMETER_RANGE_DISAGREEMENT=1,
 ANIMATE_PARAMETER_BEHAVIOR_DISAGREEMENT=2,ANIMATE_PARAMETER_DYNAMIC_CATALOGUE=4 };
typedef struct {
 animate_parameter_id id;
 const char *key,*label,*unit;
 float ui_min,ui_max,stored_min,stored_max,ui_increment,desktop_default;
 uint8_t random_mask_bit,flags;
} animate_parameter_metadata;
/* Source bounds are -1 placeholders: valid selectable IDs depend on catalogue.
 * No validation policy should be inferred from those placeholder bounds. */
const animate_parameter_metadata *animate_parameter_info(unsigned id);
extern const char *const animate_filter_labels[5];
extern const char *const animate_lfo_shape_labels[5];
extern const char *const animate_lfo_division_labels[8];
extern const float animate_lfo_division_beats[8];
#endif
