#ifndef AC_DISPLAY_H
#define AC_DISPLAY_H
#include "control.h"
typedef struct {uint32_t type,x,y,w,h,color;char text[32];} ac_draw_command;
enum {AC_D_PARAM,AC_D_LEVEL,AC_D_PAN,AC_D_MUTE,AC_D_STEP,AC_D_STEP_FIELD,AC_D_PLAYING,AC_D_PLAYHEAD,AC_D_HOLD,AC_D_RANDOM_MASK,AC_D_ROUTE,AC_D_SELECTED,AC_D_GLOBAL,AC_D_MOD,AC_D_FX,AC_D_STATUS};
/* get returns actual numeric values, not invented defaults. label/value may
 * return persistent strings; draw copies them immediately. value formats units
 * or enum names, preferred over numeric get for presentation. NULL get displays
 * '--' for unavailable fields. PARAM indices use animate_parameter_id.
 * STEP/SELECTED index=absolute step; STEP_FIELD index=step*4+field.
 * GLOBAL/MOD/ROUTE indices match control action indices. */
typedef struct {void *user;float(*get)(void*,unsigned lane,unsigned slot,unsigned field,unsigned index);const char*(*label)(void*,unsigned lane,unsigned slot,unsigned field,unsigned index);const char*(*value)(void*,unsigned lane,unsigned slot,unsigned field,unsigned index);} ac_display_source;
void ac_display_draw(const ac_controls*,const ac_display_source*);
const ac_draw_command *ac_display_data(void);
unsigned ac_display_count(void);
unsigned ac_display_stride(void);
const uint8_t *ac_display_font(void);
/* Plot command type2: text[0]0scope1spectrum2vector, same56-byte ABI as981.
 * Host supplies actual audio plot data; this module synthesizes no waveform. */
#endif
