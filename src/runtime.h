#ifndef ANIMATE_RUNTIME_H
#define ANIMATE_RUNTIME_H
#include <stdint.h>
#include "display.h"
#include "project_state.h"
#include "patch.h"
aps_state *af_project_state(void);
void af_project_restore_prepare(void);
void af_patch_restore_prepare(void);
float af_base_vector(unsigned axis);
void af_init(void);
#define AF_DEFAULT_POLYPHONY 8u
/* AC_D_PARAM extras: 20 = bank wave of the slot (browsing assigns the slot its own
 * register), 21 = bank size (read-only). 0..19 are animate_parameter_id. */
#define AF_PARAM_WAVE 20u
#define AF_PARAM_WAVE_COUNT 21u
/* Global voice ceiling 1..16. changes() increments on each user change (not on
 * restore); the hardware entry persists the value after it settles. */
unsigned af_polyphony(void);
unsigned af_polyphony_changes(void);
int af_polyphony_restore(unsigned voices);
void af_render_q15(int32_t *out,unsigned frames);
void af_render(unsigned frames);
float *af_output(void);
void af_button(unsigned button,int down,unsigned ms);
void af_pad(unsigned pad,int down);
void af_key(unsigned note,unsigned velocity);
void af_encoder(unsigned encoder,int delta);
void af_fader(unsigned fader,unsigned value);
void af_wheel(unsigned wheel,int value);
void af_midi(unsigned status,unsigned note,unsigned value);
void af_release(void);
void af_shed(void);
void af_draw(void);
unsigned af_pad_lights(unsigned ms);
unsigned af_button_lights(void);
void af_hardware_metrics(unsigned audio_percent,unsigned late);
void af_hardware_profile(unsigned render_us,unsigned interrupt_us,unsigned other_us);
void af_hardware_worker(unsigned online,unsigned jobs,unsigned timeouts);
void af_hardware_overload(const uint32_t values[8]);
unsigned af_voice_count(void);
const int16_t *af_visual_data(void);
const uint16_t *af_visual_bins(void);
float af_get(unsigned lane,unsigned slot,unsigned field,unsigned index);
/* Absolute setter. Returns1 applied,0invalid/unavailable; never reports a stub
 * setting as successful. Fields are AC_D_* from display.h. */
int af_set(unsigned lane,unsigned slot,unsigned field,unsigned index,float value);
const char *af_label(unsigned lane,unsigned slot,unsigned field,unsigned index);
const char *af_value(unsigned lane,unsigned slot,unsigned field,unsigned index);
unsigned af_control_get(unsigned index);
int af_control_set(unsigned index,unsigned value);
unsigned af_project_size(void);
unsigned char *af_project_buffer(void);
unsigned af_project_read(unsigned offset,unsigned count);
int af_project_begin(unsigned size);
int af_project_write(unsigned offset,unsigned count);
int af_project_commit(void);
#endif
