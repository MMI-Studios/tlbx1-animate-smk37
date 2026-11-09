#ifndef AC_CONTROL_H
#define AC_CONTROL_H
#include <stdint.h>
enum {AC_SHIFT,AC_PLAY,AC_REC,AC_ARP,AC_REPEAT,AC_SCALE,AC_CHORD,AC_GLOBE,AC_PATCH,AC_PARA,AC_FX,AC_SEQ,AC_OCT_DOWN,AC_OCT_UP,AC_LEFT,AC_RIGHT,AC_BUTTON_COUNT};
typedef enum {AC_VIEW_SEQ,AC_VIEW_PARAM,AC_VIEW_MIXER,AC_VIEW_LANES,AC_VIEW_MASK,AC_VIEW_ROUTE,AC_VIEW_VIS,AC_VIEW_GLOBAL,AC_VIEW_MOD,AC_VIEW_FX,AC_VIEW_HOME,AC_VIEW_PATCHES,AC_VIEW_CONFIRM} ac_view;
typedef enum {AC_LANE_SELECT,AC_SLOT_SELECT,AC_VIEW_CHANGE,AC_PLAY_TOGGLE,AC_RECORD_TOGGLE,AC_STEP_TOGGLE,AC_STEP_EDIT,AC_STEP_CLEAR,AC_PARAM_ADJUST,AC_PARAM_SET,AC_LEVEL_SET,AC_PAN_ADJUST,AC_MUTE_TOGGLE,AC_MASTER_ADJUST,AC_RANDOM_MASK_SET,AC_RANDOM_ROUTE_ADJUST,AC_RANDOM_STEP_TOGGLE,AC_RANDOM_APPLY,AC_GLOBAL_ADJUST,AC_MOD_ADJUST,AC_FX_ADJUST,AC_PRESET_ADJUST,AC_OCTAVE_ADJUST,AC_LENGTH_ADJUST,AC_ARP_TOGGLE,AC_REPEAT_TOGGLE,AC_HOLD_TOGGLE,AC_SCALE_TOGGLE,AC_CHORD_TOGGLE,AC_XY_TOGGLE,AC_PATCH_BROWSE,AC_PATCH_SELECT,AC_PATCH_SAVE_REQUEST,AC_PATCH_LOAD_REQUEST,AC_PATCH_CONFIRM,AC_PATCH_CANCEL} ac_action_type;
typedef struct {ac_action_type type;uint8_t lane,slot,index;int16_t value;} ac_action;
typedef void (*ac_emit)(void*,const ac_action*);
typedef struct {
 uint8_t lane,slot,step_page,param_bank,mixer_bank,random_bank,route_bank,global_page,mod_bank,visualizer,shift;
 ac_view view,return_view;
 uint32_t buttons,shift_since;uint16_t pads,edit_pads;
 uint8_t edit_lane[16],edit_step[16];
} ac_controls;
void ac_control_init(ac_controls*);
void ac_control_button(ac_controls*,unsigned button,int down,uint32_t ms,ac_emit,void*);
void ac_control_pad(ac_controls*,unsigned pad,int down,ac_emit,void*);
void ac_control_encoder(ac_controls*,unsigned encoder,int delta,ac_emit,void*);
void ac_control_fader(ac_controls*,unsigned fader,unsigned value,ac_emit,void*);
/* PARAM indices0..15 match animate_parameter_id; faders16..19.
 * MIXER encoders alternate slot volume/pan: K1/2 A, K3/4 B,
 * K5/6 C, K7/8 D. Emit AC_PARAM_ADJUST with explicit slot and parameter.
 * Mixer faders retain banked lane levels; arrows change that fader bank only.
 * STEP_EDIT index=absolute step, value=delta; slot carries edited field0..3.
 * ROUTE index0mode,1scope,2..6 note/velocity/gate/prob/enabled booleans,
 * 8laneTarget,9slotTarget,10noteAmount,11parameterAmount.
 * Targets: 0selected,1all,2.. explicit lane1..10 or slotA..D.
 * GLOBAL index=page*8+encoder: pagesAudio,Timing,Engine,Scale,Chord.
 * MOD index=bank*8+encoder: banksX,Y,selected-slotWT.
 * Action indices and lane/slot are zero-based. Host applies values/ranges.
 * Mode changes do not stop playback. No DSP or runtime mutation here. */
#endif
