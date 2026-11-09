/* SPDX-License-Identifier: GPL-3.0-only */
#include "runtime.h"
#include "project_state.h"
#include "patch.h"
#include "baseline/bridge.h"
#include "support/music.h"
#include "../vendor/tlbx-display/visual.h"
#include <string.h>
void af_visual_reset(void);
void af_visual_capture(const int32_t *stereo,unsigned frames);
void af_visual_prepare(unsigned mode);
#ifdef AF_HARDWARE
#define AF_STATE __attribute__((section(".pool")))
#else
#define AF_STATE
#endif
typedef struct {
 aps_state project;ac_controls ui;animate_music music;
 uint32_t keys[2][128][4],lane_handle[10],lane_generation[10];
 uint8_t down[2][128],velocity[2][128],pending_off[10],lane_step[10];
 uint64_t clock,lane_born[10];
 float level[10],master,base_xy[2];unsigned preset,hold,sustain,record,arp,repeat,xy,poly;
 struct {unsigned remaining,first,mode,scope,page,step_mask;int lane,slot;uint32_t mask[4],selected[4];float base[4][20],note_amount,param_amount;} job;
 unsigned route_mode,route_scope,lane_target,slot_target,step_mask,arp_clock,arp_key;
 int octave,pitch;unsigned mod,hardware_valid,cpu,late,shed,profile[3];
 float output[512];int32_t block[64];
} af_state;
static af_state af AF_STATE;
static uint32_t af_overload_values[8];
void af_hardware_overload(const uint32_t values[8]){memcpy(af_overload_values,values,sizeof af_overload_values);}
unsigned af_voice_count(void){return ab_active();}
aps_state *af_project_state(void){return &af.project;}
void af_project_restore_prepare(void){af.job.remaining=0;af.project.sequence.playing=0;ab_panic();memset(af.keys,0,sizeof af.keys);memset(af.down,0,sizeof af.down);memset(af.pending_off,0,sizeof af.pending_off);memset(af.lane_handle,0,sizeof af.lane_handle);af.sustain=0;}
void af_patch_restore_prepare(void){unsigned playing=af.project.sequence.playing;af_project_restore_prepare();af.project.sequence.playing=(uint8_t)playing;}
float af_base_vector(unsigned axis){return axis<2?(af.xy?af.base_xy[axis]:ab_vector_get(axis)):0;}
static float af_bound(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float af_missing(void){union{uint32_t u;float f;}v={0x7fc00000};return v.f;}
static void af_capture_base(float base[4][20]){for(unsigned s=0;s<4;s++)for(unsigned p=0;p<20;p++)base[s][p]=ab_slot_parameter_get(s,p);}
static void af_lane_parameters(unsigned lane){uint32_t h=af.lane_handle[lane];if(!h)return;ab_voice_parameters_clear(h);for(unsigned s=0;s<4;s++)for(unsigned p=0;p<20;p++){float v;int locked=ap_lock_get(&af.project.locks[lane][af.lane_step[lane]][s],p,&v);if(p==ANIMATE_LEVEL&&af.level[lane]!=1){if(!locked)v=af.project.base[s][p];v*=af.level[lane];locked=1;}if(locked)(void)ab_voice_parameter(h,s,p,v);}}
static uint32_t af_start_pitch(int32_t pitch,unsigned velocity){if(pitch<0||pitch>1270000||!velocity)return 0;unsigned note=(unsigned)(pitch/10000);uint32_t h=ab_owned_note_on(note,velocity);if(pitch!=(int32_t)(note*10000))ab_voice_pitch(h,pitch);return h;}
static void af_sequence_event(void*unused,const as_event*e){(void)unused;unsigned l=e->lane;if(e->on){int32_t pitch=animate_music_map(&af.music,e->note,0);af.lane_handle[l]=af_start_pitch(pitch,e->velocity);af.lane_generation[l]=e->generation;af.lane_step[l]=e->step;af.lane_born[l]=af.clock;af.pending_off[l]=0;af_lane_parameters(l);}else if(af.lane_generation[l]==e->generation){if(af.lane_born[l]==af.clock)af.pending_off[l]=1;else ab_note_off(af.lane_handle[l]);}}
static void af_key_release(unsigned source,unsigned key,int force){if(force||(!af.hold&&!af.sustain))for(unsigned n=0;n<4;n++){ab_note_off(af.keys[source][key][n]);af.keys[source][key][n]=0;}}
static void af_key_trigger(unsigned source,unsigned key){af_key_release(source,key,1);int32_t pitch=animate_music_map(&af.music,key,af.octave);int32_t chord[4];unsigned count=animate_music_chord(&af.music,pitch,chord);for(unsigned n=0;n<count&&n<4;n++)af.keys[source][key][n]=af_start_pitch(chord[n],af.velocity[source][key]);}
static void af_live_key(unsigned source,unsigned key,unsigned velocity){if(key>127||source>1)return;if(velocity>127)velocity=127;af.down[source][key]=velocity!=0;af.velocity[source][key]=(uint8_t)velocity;if(!velocity){af_key_release(source,key,0);return;}if(!af.arp)af_key_trigger(source,key);if(af.record){unsigned step=af.project.sequence.playing?af.project.sequence.current:af.ui.step_page*16;as_step v=af.project.sequence.steps[af.ui.lane][step];v.enabled=1;v.note=(uint8_t)key;v.velocity=(uint8_t)velocity;as_edit(&af.project.sequence,af.ui.lane,step,v);}}
static void af_hold(unsigned on){af.hold=!!on;if(!af.hold&&!af.sustain)for(unsigned source=0;source<2;source++)for(unsigned k=0;k<128;k++)if(!af.down[source][k])af_key_release(source,k,1);}
void af_release(void){ab_release_all();memset(af.keys,0,sizeof af.keys);memset(af.down,0,sizeof af.down);memset(af.pending_off,0,sizeof af.pending_off);af.sustain=0;}
void af_shed(void){unsigned before=ab_shed_count();ab_shed_oldest();if(ab_shed_count()!=before&&af.shed<16777215u)af.shed++;}
static void af_preset(unsigned preset){af.job.remaining=0;ab_preset(preset%6);for(unsigned r=0;r<4;r++)ab_wave(r,r);/* original presets use the original waves */af.preset=preset%6;af_patch_origin(af.preset);af.base_xy[0]=ab_vector_get(0);af.base_xy[1]=ab_vector_get(1);af_capture_base(af.project.base);/* Do not activate logical edits merely by projecting canonical values. */for(unsigned l=0;l<10;l++)af_lane_parameters(l);}
static unsigned af_poly_changes;
unsigned af_polyphony(void){return af.poly;}
unsigned af_polyphony_changes(void){return af_poly_changes;}
int af_polyphony_restore(unsigned n){if(n<1||n>16||!ab_polyphony(n))return 0;af.poly=n;return 1;}
void af_init(void){memset(&af,0,sizeof af);ab_init();ab_polyphony(AF_DEFAULT_POLYPHONY);af.poly=AF_DEFAULT_POLYPHONY;af.master=1;af.step_mask=15;af.route_mode=APS_PARAMS;af.route_scope=APS_SOUND;ac_control_init(&af.ui);float base[4][20];af_capture_base(base);(void)aps_init(&af.project,44100,base);for(unsigned l=0;l<10;l++)af.level[l]=1;af_visual_reset();af_patch_reset();}
static int af_random_target(unsigned first){unsigned lane=first/128,step=first%128;return (af.job.lane<0||lane==(unsigned)af.job.lane)&&(af.job.scope==APS_ALL||(af.job.scope==APS_PAGE&&step/16==af.job.page)||(af.job.scope==APS_SELECTED&&(af.job.selected[step/32]&(1u<<(step%32)))));}
static void af_random_apply(void){int lane=af.lane_target==0?af.ui.lane:af.lane_target==1?-1:(int)af.lane_target-2;int slot=af.slot_target==0?af.ui.slot:af.slot_target==1?-1:(int)af.slot_target-2;
 if(af.job.remaining)return; /* Finish the visible edit job before another apply. */
 if(af.route_scope!=APS_SOUND){af.job.first=0;af.job.remaining=0;af.job.mode=af.route_mode;af.job.scope=af.route_scope;af.job.page=af.ui.step_page;af.job.step_mask=af.step_mask;af.job.lane=lane;af.job.slot=slot;memcpy(af.job.mask,af.project.mask,sizeof af.job.mask);memcpy(af.job.selected,af.project.selected,sizeof af.job.selected);memcpy(af.job.base,af.project.base,sizeof af.job.base);af.job.note_amount=af.project.note_amount;af.job.param_amount=af.project.param_amount;for(unsigned i=0;i<1280;i++)af.job.remaining+=af_random_target(i);return;}
 (void)aps_randomize_range(&af.project,af.route_mode,af.route_scope,lane,slot,af.ui.step_page,af.step_mask,0,1280);
 if(af.route_scope==APS_SOUND)for(unsigned s=0;s<4;s++)if(slot<0||s==(unsigned)slot)for(unsigned p=0;p<20;p++)if(af.project.mask[s]&(1u<<animate_parameter_info(p)->random_mask_bit))ab_slot_parameter(s,p,af.project.base[s][p]);
 if(af.route_scope==APS_SOUND)af_patch_changed();
 for(unsigned l=0;l<10;l++)af_lane_parameters(l);
}
static void af_random_tick(void){if(!af.job.remaining)return;
 /* Only one step's eighty parameter values may be changed per audio block.
  * Capture routing/amounts at Apply; later UI edits remain independent. */
 uint32_t masks[4],selected[4];float base[4][20];float na=af.project.note_amount,pa=af.project.param_amount;
 memcpy(masks,af.project.mask,sizeof masks);memcpy(selected,af.project.selected,sizeof selected);memcpy(base,af.project.base,sizeof base);
 memcpy(af.project.mask,af.job.mask,sizeof masks);memcpy(af.project.selected,af.job.selected,sizeof selected);memcpy(af.project.base,af.job.base,sizeof base);af.project.note_amount=af.job.note_amount;af.project.param_amount=af.job.param_amount;
 while(af.job.first<1280&&!af_random_target(af.job.first))af.job.first++;
 unsigned first=af.job.first++;
 (void)aps_randomize_range(&af.project,af.job.mode,af.job.scope,af.job.lane,af.job.slot,af.job.page,af.job.step_mask,first,1);
 af.job.remaining--;
 memcpy(af.project.mask,masks,sizeof masks);memcpy(af.project.selected,selected,sizeof selected);memcpy(af.project.base,base,sizeof base);af.project.note_amount=na;af.project.param_amount=pa;
 if(first%128==af.lane_step[first/128])af_lane_parameters(first/128);
}
void af_render_q15(int32_t*out,unsigned frames){if(!out||frames%32)return;for(unsigned at=0;at<frames;at+=32){af_random_tick();as_advance(&af.project.sequence,af.clock,af_sequence_event,0);if((af.arp||af.repeat)&&(af.arp_clock+=32)>=44100u*60/(af.project.sequence.bpm*4)){af.arp_clock=0;if(af.repeat){for(unsigned s=0;s<2;s++)for(unsigned k=0;k<128;k++)if(af.down[s][k])af_key_trigger(s,k);}else{for(unsigned s=0;s<2;s++)for(unsigned k=0;k<128;k++)af_key_release(s,k,1);for(unsigned n=1;n<=256;n++){unsigned key=(af.arp_key+n)%256;if(af.down[key/128][key%128]){af.arp_key=key;af_key_trigger(key/128,key%128);break;}}}}
 ab_render(out+at*2,32);af_visual_capture(out+at*2,32);for(unsigned l=0;l<10;l++)if(af.pending_off[l]){ab_note_off(af.lane_handle[l]);af.pending_off[l]=0;}af.clock+=32;}}
void af_render(unsigned frames){if(frames>256||frames%32)return;for(unsigned at=0;at<frames;at+=32){af_render_q15(af.block,32);for(unsigned i=0;i<64;i++)af.output[at*2+i]=af.block[i]/32768.f;}}
float *af_output(void){return af.output;}
void af_key(unsigned note,unsigned velocity){af_live_key(0,note,velocity);}
void af_midi(unsigned status,unsigned note,unsigned value){if(note>127||value>127)return;switch(status&240){case 144:af_live_key(1,note,value);break;case 128:af_live_key(1,note,0);break;case 224:af_wheel(0,(int)(note|(value<<7))-8192);break;case 176:if(note==1)af_wheel(1,(int)value);else if(note==64){af.sustain=value>=64;if(!af.sustain)af_hold(af.hold);}else if(note==123)af_release();break;default:break;}}
void af_wheel(unsigned which,int value){if(which==0)af.pitch=(int)af_bound(value,-8192,8191);else if(which==1)af.mod=(unsigned)af_bound(value,0,127);else return;if(af.xy)ab_vector((af.pitch+8192)/16383.f,af.mod/127.f);else if(which==0)ab_pitch_bend(af.pitch);else ab_modwheel(af.mod);}
void af_hardware_metrics(unsigned cpu,unsigned late){af.hardware_valid=1;af.cpu=cpu;af.late=late;}
void af_hardware_profile(unsigned render_us,unsigned interrupt_us,unsigned other_us){af.profile[0]=render_us;af.profile[1]=interrupt_us;af.profile[2]=other_us;}
static unsigned af_worker_metrics[3];
void af_hardware_worker(unsigned online,unsigned jobs,unsigned timeouts){af_worker_metrics[0]=online;af_worker_metrics[1]=jobs;af_worker_metrics[2]=timeouts;}
const int16_t*af_visual_data(void){return tlbx_visual_data();}
const uint16_t*af_visual_bins(void){return tlbx_visual_bins();}
static const unsigned af_fx_ids[7]={33,34,35,36,0,39,38};
float af_get(unsigned lane,unsigned slot,unsigned field,unsigned index){if(lane>=10||slot>=4)return af_missing();as_sequence*q=&af.project.sequence;
 if(field==AC_D_STATUS){switch(index){case 0:return af_patch_dirty();case 1:return af_patch_selection();case 2:return af_patch_occupied();case 3:return af_patch_confirm_kind();case 4:return af_patch_status();case 5:return af_patch_notice();case 6:return af_patch_current();default:return af_missing();}}
 if(field==AC_D_GLOBAL&&(index==7||index==42))return af.job.remaining;
 if(field==AC_D_GLOBAL&&index==11){unsigned held=0;for(unsigned s=0;s<2;s++)for(unsigned k=0;k<128;k++)held+=af.down[s][k]!=0;return held;}
 if(field==AC_D_GLOBAL&&index==12)return af.shed;
 if(field==AC_D_GLOBAL&&index>=21&&index<=23)return af.hardware_valid?(float)af_worker_metrics[index-21]:-1;
 if(field==AC_D_GLOBAL&&index>=48&&index<56)return af.hardware_valid&&af_overload_values[6]?(float)af_overload_values[index-48]:-1;
 if(field==AC_D_GLOBAL&&index>=13&&index<=15)return af.hardware_valid?(float)af.profile[index-13]:-1;
 switch(field){
 case AC_D_PARAM:return index<20?af.project.base[slot][index]:index==AF_PARAM_WAVE?(float)ab_wave_get((unsigned)af.project.base[slot][0]&3u):index==AF_PARAM_WAVE_COUNT?(float)ab_wave_count():af_missing();
 case AC_D_LEVEL:return af.level[lane];case AC_D_PAN:return af_missing();case AC_D_MUTE:return !q->enabled[lane];
 case AC_D_STEP:return index<128?q->steps[lane][index].enabled:af_missing();
 case AC_D_STEP_FIELD:if(index/4<128){as_step*v=&q->steps[lane][index/4];return index%4==0?v->note:index%4==1?v->velocity:index%4==2?v->gate:v->probability;}return af_missing();
 case AC_D_PLAYING:return q->playing;case AC_D_PLAYHEAD:return q->current;case AC_D_HOLD:return af.hold;
 case AC_D_RANDOM_MASK:return index<19?!!(af.project.mask[slot]&(1u<<index)):af_missing();
 case AC_D_SELECTED:return index<128?!!(af.project.selected[index/32]&(1u<<(index%32))):af_missing();
 case AC_D_ROUTE:switch(index){case 0:return af.route_mode;case 1:return af.route_scope;case 8:return af.lane_target;case 9:return af.slot_target;case 10:return af.project.note_amount;case 11:return af.project.param_amount;default:return index>=2&&index<=6?!!(af.step_mask&(1u<<(index-2))):af_missing();}
 case AC_D_FX:return index<7?ab_param_get(af_fx_ids[index]):index==7?ab_slot_get(slot,AB_SOLO):af_missing();
 case AC_D_MOD:if(index<16){unsigned y=index>=8,k=index%8;if(k==0)return ab_vector_get(y);if(k<=5)return ab_vector_lfo_get(y,k-1);return af_missing();}if(index<21){static const unsigned p[5]={5,6,7,8,9};return af.project.base[slot][p[index-16]];}return af_missing();
 case AC_D_GLOBAL:switch(index){case 0:return af.master;case 1:return af.poly;case 2:return 16;case 3:return 44100;case 4:return af.hardware_valid?(float)af.cpu:-1;case 5:return ab_active();case 6:return af.hardware_valid?(float)af.late:-1;case 8:return q->bpm;case 9:return q->length;case 10:return q->division;case 16:return af.hold;case 17:return af.arp;case 18:return af.repeat;case 19:return af.xy;case 20:return af.preset;case 24:return af.music.scale;case 25:return af.music.root;case 26:return af.music.map;case 27:return af.music.transpose;case 32:return af.music.chord;case 33:return af.music.voicing;case 40:return af.pitch;case 41:return af.mod;default:return af_missing();}
 default:return af_missing();}
}
static int af_set_impl(unsigned lane,unsigned slot,unsigned field,unsigned index,float value){if(lane>=10||slot>=4||!(value>=-100000&&value<=100000))return 0;as_sequence*q=&af.project.sequence;int v=(int)value;
 switch(field){
 case AC_D_PARAM:if(index==AF_PARAM_WAVE){/* Browsing gives the slot its own register: slot A writes register A. */
  if(!(value>=0&&value<(float)ab_wave_count())||value!=(float)v)return 0;
  if(af.project.base[slot][0]!=(float)slot&&!af_set_impl(lane,slot,AC_D_PARAM,0,(float)slot))return 0;
  return ab_wave(slot,(unsigned)v);}
 if(index>=20||!ab_slot_parameter(slot,index,value))return 0;af.project.base[slot][index]=value;for(unsigned l=0;l<10;l++)af_lane_parameters(l);return 1;
 case AC_D_LEVEL:if(value<0||value>1)return 0;af.level[lane]=value;af_lane_parameters(lane);return 1;
 case AC_D_MUTE:q->enabled[lane]=!v;if(v)ab_note_off(af.lane_handle[lane]);return 1;
 case AC_D_STEP:if(index>=128)return 0;q->steps[lane][index].enabled=!!v;return 1;
 case AC_D_STEP_FIELD:if(index/4>=128)return 0;{as_step step=q->steps[lane][index/4];switch(index%4){case 0:step.note=(uint8_t)af_bound(value,0,127);break;case 1:step.velocity=(uint8_t)af_bound(value,1,127);break;case 2:step.gate=(uint8_t)af_bound(value,1,100);break;case 3:step.probability=(uint8_t)af_bound(value,0,100);break;}return as_edit(q,lane,index/4,step);}
 case AC_D_PLAYING:if(v){as_play(q,af.clock,af_sequence_event,0);}else{as_stop(q,af.clock,af_sequence_event,0);af_release();}return 1;
 case AC_D_HOLD:af_hold(v);return 1;
 case AC_D_RANDOM_MASK:if(index>=19)return 0;if(v)af.project.mask[slot]|=1u<<index;else af.project.mask[slot]&=~(1u<<index);return 1;
 case AC_D_SELECTED:if(index>=128)return 0;aps_select(&af.project,index,v);return 1;
 case AC_D_ROUTE:if(index==0){if(af.route_scope&&v>=0&&v<=2)af.route_mode=v;return af.route_scope!=0;}if(index==1){if(v<0||v>3)return 0;af.route_scope=v;if(!v)af.route_mode=APS_PARAMS;return 1;}if(index>=2&&index<=6){if(v)af.step_mask|=1u<<(index-2);else af.step_mask&=~(1u<<(index-2));return 1;}if(index==8&&v>=0&&v<=11){af.lane_target=v;return 1;}if(index==9&&v>=0&&v<=5){af.slot_target=v;return 1;}if(index==10&&value>=0&&value<=1){af.project.note_amount=value;return 1;}if(index==11&&value>=0&&value<=1){af.project.param_amount=value;return 1;}return 0;
 case AC_D_FX:if(index<7)return ab_param(af_fx_ids[index],v);return index==7?ab_slot(slot,AB_SOLO,!!v):0;
 case AC_D_MOD:if(index<16){unsigned y=index>=8,k=index%8;if(k==0){if(value<0||value>1)return 0;if(af.xy){af.base_xy[y]=value;return 1;}return ab_vector(y?ab_vector_get(0):value,y?value:ab_vector_get(1));}if(k<=5)return ab_vector_lfo(y,k-1,value);return 0;}if(index<21){static const unsigned p[5]={5,6,7,8,9};return af_set(lane,slot,AC_D_PARAM,p[index-16],value);}return 0;
 case AC_D_GLOBAL:switch(index){
 case 0:if(!ab_master(value))return 0;af.master=value;return 1;
 case 1:if(!(v>=1&&v<=16)||!ab_polyphony((unsigned)v))return 0;if(af.poly!=(unsigned)v)++af_poly_changes;af.poly=(unsigned)v;return 1;
 case 8:if(!as_tempo(q,(unsigned)v))return 0;ab_tempo(v);return 1;
 case 9:return as_length(q,(unsigned)v);case 10:return as_division(q,(unsigned)v);
 case 16:af_hold(v);return 1;
 case 17:case 18:for(unsigned s=0;s<2;s++)for(unsigned k=0;k<128;k++)af_key_release(s,k,1);af.arp=index==17&&v;af.repeat=index==18&&v;af.arp_clock=0;if(!af.arp)for(unsigned s=0;s<2;s++)for(unsigned k=0;k<128;k++)if(af.down[s][k])af_key_trigger(s,k);return 1;
 case 19:if(!!v==af.xy)return 1;if(v){af.base_xy[0]=ab_vector_get(0);af.base_xy[1]=ab_vector_get(1);}af.xy=!!v;ab_pitch_bend(0);ab_modwheel(0);if(af.xy)af_wheel(0,af.pitch);else{ab_vector(af.base_xy[0],af.base_xy[1]);ab_pitch_bend(af.pitch);ab_modwheel(af.mod);}return 1;
 case 20:if(v<0||v>5)return 0;af_preset((unsigned)v);return 1;
 case 24:if(v<0||(unsigned)v>=animate_scale_count())return 0;af.music.scale=v;return 1;
 case 25:if(v<0||v>11)return 0;af.music.root=v;return 1;
 case 26:if(v<0||v>3)return 0;af.music.map=v;return 1;
 case 27:if(v< -36||v>36)return 0;af.music.transpose=v;return 1;
 case 32:if(v<0||v>9)return 0;af.music.chord=v;return 1;
 case 33:if(v<0||v>4)return 0;af.music.voicing=v;return 1;
 default:return 0;}
 default:return 0;}
}
int af_set(unsigned lane,unsigned slot,unsigned field,unsigned index,float value){
 int vector=field==AC_D_MOD&&(index==0||index==8);
 float before=vector?af_base_vector(index/8):af_get(lane,slot,field,index);
 unsigned mask=slot<4?ab_slot_parameter_mask(slot):0;
 unsigned lfo=field==AC_D_MOD&&index<16?ab_vector_lfo_mask(index/8):0;
 if(!af_set_impl(lane,slot,field,index,value))return 0;
 int persisted=field==AC_D_PARAM||field==AC_D_MOD||field==AC_D_FX||field==AC_D_LEVEL||field==AC_D_HOLD||field==AC_D_RANDOM_MASK||field==AC_D_ROUTE||(field==AC_D_GLOBAL&&(index==0||index==1||index==8||(index>=16&&index<=20)||(index>=24&&index<=27)||index==32||index==33));
 float after=vector?af_base_vector(index/8):af_get(lane,slot,field,index);
 if(persisted&&(before!=after||(field==AC_D_PARAM&&mask!=ab_slot_parameter_mask(slot))||(field==AC_D_MOD&&index<16&&lfo!=ab_vector_lfo_mask(index/8))||(field==AC_D_GLOBAL&&index==20)))af_patch_changed();return 1;
}
static void af_adjust(unsigned lane,unsigned slot,unsigned field,unsigned index,int delta){if(field==AC_D_PARAM&&index==AF_PARAM_WAVE){int n=(int)af_get(lane,slot,field,index)+delta,top=(int)ab_wave_count()-1;af_set(lane,slot,field,index,(float)(n<0?0:n>top?top:n));return;}if(field==AC_D_MOD&&index>=16&&index<21){af_adjust(lane,slot,AC_D_PARAM,index-11,delta);return;}float current=af_get(lane,slot,field,index);if(!(current==current))return;if(field==AC_D_GLOBAL&&index==1){af_set(lane,slot,field,index,af_bound(current+delta,1,16));return;}float step=1;if(field==AC_D_PARAM){const animate_parameter_metadata*m=animate_parameter_info(index);step=m->ui_increment?m->ui_increment:.005f;float lo=index==0?0:m->stored_min,hi=index==0?3:m->stored_max;af_set(lane,slot,field,index,af_bound(current+delta*step,lo,hi));return;}if(field==AC_D_MOD){unsigned k=index%8;step=k==0||k==3?.01f:k==2?.01f:1;}if(field==AC_D_ROUTE&&index>=10)step=.01f;if(field==AC_D_GLOBAL&&index==0)step=.01f;(void)af_set(lane,slot,field,index,current+delta*step);}
static void af_dispatch(void*unused,const ac_action*a){(void)unused;unsigned l=a->lane,s=a->slot,p=a->index;switch(a->type){
 case AC_SLOT_SELECT:if(ab_slot_get(0,AB_SLOT)!=(int)s&&ab_slot_get(0,AB_SOLO))af_patch_changed();ab_slot(s,AB_SLOT,s);break;
 case AC_PLAY_TOGGLE:af_set(l,s,AC_D_PLAYING,0,!af.project.sequence.playing);break;
 case AC_RECORD_TOGGLE:af.record^=1;break;
 case AC_STEP_TOGGLE:af_set(l,s,AC_D_STEP,p,!af.project.sequence.steps[l][p].enabled);break;
 case AC_STEP_EDIT:af_adjust(l,s,AC_D_STEP_FIELD,p*4+s,a->value);break;
 case AC_STEP_CLEAR:aps_clear(&af.project,l,p*16,16,1,1);af_lane_parameters(l);break;
 case AC_PARAM_ADJUST:af_adjust(l,s,AC_D_PARAM,p,a->value);break;
 case AC_PARAM_SET:{const animate_parameter_metadata*m=animate_parameter_info(p);if(m)af_set(l,s,AC_D_PARAM,p,m->stored_min+(m->stored_max-m->stored_min)*a->value/127.f);break;}
 case AC_LEVEL_SET:af_set(l,s,AC_D_LEVEL,0,a->value/127.f);break;
 case AC_MUTE_TOGGLE:af_set(l,s,AC_D_MUTE,0,af.project.sequence.enabled[l]);break;
 case AC_MASTER_ADJUST:af_adjust(l,s,AC_D_GLOBAL,0,a->value);break;
 case AC_RANDOM_MASK_SET:af_set(l,s,AC_D_RANDOM_MASK,p,a->value);break;
 case AC_RANDOM_ROUTE_ADJUST:if(p>=2&&p<=6)af_set(l,s,AC_D_ROUTE,p,a->value);else af_adjust(l,s,AC_D_ROUTE,p,a->value);break;
 case AC_RANDOM_STEP_TOGGLE:if(af.route_scope==APS_SELECTED)af_set(l,s,AC_D_SELECTED,p,!af_get(l,s,AC_D_SELECTED,p));break;
 case AC_RANDOM_APPLY:af_random_apply();break;
 case AC_GLOBAL_ADJUST:if(p==20){if(af_patch_load((unsigned)af_bound((int)af.preset+a->value,0,5)))af.ui.view=AC_VIEW_CONFIRM;}else af_adjust(l,s,AC_D_GLOBAL,p,a->value);break;
 case AC_MOD_ADJUST:af_adjust(l,s,AC_D_MOD,p,a->value);break;
 case AC_FX_ADJUST:af_adjust(l,s,AC_D_FX,p,a->value);break;
 case AC_PRESET_ADJUST:af_patch_load((af.preset+6+(a->value>0?1:5))%6);af.ui.view=AC_VIEW_CONFIRM;break;
 case AC_OCTAVE_ADJUST:af_control_set(16,(unsigned)af_bound(af.octave+3+a->value,0,6));break;
 case AC_LENGTH_ADJUST:af_set(l,s,AC_D_GLOBAL,9,af_bound(af.project.sequence.length+a->value,1,128));break;
 case AC_ARP_TOGGLE:af_set(l,s,AC_D_GLOBAL,17,!af.arp);break;case AC_REPEAT_TOGGLE:af_set(l,s,AC_D_GLOBAL,18,!af.repeat);break;
 case AC_HOLD_TOGGLE:af_set(l,s,AC_D_HOLD,0,!af.hold);break;case AC_SCALE_TOGGLE:af_set(l,s,AC_D_GLOBAL,26,af.music.map?0:ANIMATE_MAP_SNAP);break;
 case AC_CHORD_TOGGLE:af_set(l,s,AC_D_GLOBAL,32,af.music.chord?0:ANIMATE_CHORD_MAJOR);break;case AC_XY_TOGGLE:af_set(l,s,AC_D_GLOBAL,19,!af.xy);break;
 case AC_PATCH_BROWSE:af_patch_cancel();af.ui.view=AC_VIEW_PATCHES;break;
 case AC_PATCH_SELECT:af_patch_select((unsigned)af_bound((int)af_patch_selection()+a->value,0,AF_PATCH_ENTRIES-1));break;
 case AC_PATCH_SAVE_REQUEST:if(af_patch_save(af_patch_selection()<AF_PATCH_FACTORY?0:af_patch_selection()-AF_PATCH_FACTORY))af.ui.view=AC_VIEW_CONFIRM;break;
 case AC_PATCH_LOAD_REQUEST:if(af_patch_load(af_patch_selection()))af.ui.view=AC_VIEW_CONFIRM;break;
 case AC_PATCH_CONFIRM:if(af_patch_confirm())af.ui.view=AC_VIEW_PATCHES;break;
 case AC_PATCH_CANCEL:af_patch_cancel();af.ui.view=af.ui.view==AC_VIEW_PATCHES?AC_VIEW_HOME:AC_VIEW_PATCHES;break;
 default:break;}}
void af_button(unsigned b,int down,unsigned ms){ac_control_button(&af.ui,b,down,ms,af_dispatch,0);}
void af_pad(unsigned p,int down){ac_control_pad(&af.ui,p,down,af_dispatch,0);}
void af_encoder(unsigned e,int d){ac_control_encoder(&af.ui,e,d,af_dispatch,0);}
void af_fader(unsigned f,unsigned v){ac_control_fader(&af.ui,f,v,af_dispatch,0);}
unsigned af_control_get(unsigned i){switch(i){case 0:return af.ui.view;case 1:return af.ui.lane;case 2:return af.ui.slot;case 3:return af.ui.step_page;case 4:return af.ui.param_bank;case 5:return af.ui.random_bank;case 6:return af.ui.route_bank;case 7:return af.ui.global_page;case 8:return af.ui.mod_bank;case 9:return af.ui.visualizer;case 10:return af.ui.shift;case 11:return af.preset;case 12:return(unsigned)(af.pitch+8192);case 13:return af.mod;case 14:return af.xy;case 15:return af.ui.mixer_bank;case 16:return(unsigned)(af.octave+3);default:return 0;}}
int af_control_set(unsigned i,unsigned v){switch(i){case 0:if(v>AC_VIEW_CONFIRM)return 0;af.ui.view=(ac_view)v;af.ui.edit_pads=0;break;case 1:if(v>=10)return 0;af.ui.lane=v;break;case 2:if(v>=4)return 0;if(v!=af.ui.slot&&ab_slot_get(0,AB_SOLO))af_patch_changed();af.ui.slot=v;ab_slot(v,AB_SLOT,v);break;case 3:if(v>=8)return 0;af.ui.step_page=v;break;case 4:if(v>=2)return 0;af.ui.param_bank=v;break;case 5:if(v>=3)return 0;af.ui.random_bank=v;break;case 6:if(v>=2)return 0;af.ui.route_bank=v;break;case 7:if(v>=6)return 0;af.ui.global_page=v;break;case 8:if(v>=3)return 0;af.ui.mod_bank=v;break;case 9:if(v>=3)return 0;af.ui.visualizer=v;break;case 15:if(v>=3)return 0;af.ui.mixer_bank=v;break;case 16:if(v>6)return 0;if(af.octave!=(int)v-3)af_patch_changed();af.octave=(int)v-3;break;default:return 0;}return 1;}
unsigned af_pad_lights(unsigned ms){(void)ms;unsigned mask=0;for(unsigned p=0;p<16;p++){unsigned step=af.ui.step_page*16+p;if(af.ui.view==AC_VIEW_ROUTE?af_get(af.ui.lane,af.ui.slot,AC_D_SELECTED,step):af.project.sequence.steps[af.ui.lane][step].enabled)mask|=1u<<p;}return mask;}
unsigned af_button_lights(void){return(af.project.sequence.playing?(1u<<16):0)|(af.record?(1u<<17):0)|(af.ui.shift?(1u<<11):0)|((af.hold||af.repeat)?(1u<<7):0);}
const char*af_label(unsigned lane,unsigned slot,unsigned field,unsigned index){(void)lane;(void)slot;
 static const char*global[40]={"MASTER","POLYPHONY","PCM BITS","SYNTH HZ","AUDIO %","VOICES","LATE BUFFERS","", "TEMPO","LENGTH","DIVISION","","","","","", "HOLD","ARP","REPEAT","WHEEL XY","PRESET","","","", "SCALE","ROOT","KEY MAP","TRANSPOSE","","","","", "CHORD","VOICING","","","","","",""};
 static const char*route[12]={"MODE","SCOPE","NOTE","VELOCITY","GATE","PROBABILITY","STEP ON/OFF","","LANE TARGET","SLOT TARGET","NOTE AMOUNT","PARAM AMOUNT"};
 static const char*mods[8]={"POSITION","SHAPE","RATE HZ","AMOUNT","SYNC","DIVISION","--","--"};
 static const char*wt[5]={"WT AMOUNT","WT SHAPE","WT RATE","WT SYNC","WT DIVISION"};
 static const char*fx[8]={"DISTORTION","CHORUS","DELAY SEND","REVERB SEND","TRACK LEVEL","TRACK PAN","GLIDE","SLOT SOLO"};
 if(field==AC_D_PARAM){if(index==AF_PARAM_WAVE)return"WAVE";const animate_parameter_metadata*m=animate_parameter_info(index);return m?m->label:"--";}
 if(field==AC_D_RANDOM_MASK){for(unsigned p=0;p<20;p++)if(animate_parameter_info(p)->random_mask_bit==index)return animate_parameter_info(p)->label;return"--";}
 if(field==AC_D_GLOBAL&&index>=21&&index<=23){static const char*names[]={"CPU1 ONLINE","CPU1 VOICES","CPU1 TIMEOUTS"};return names[index-21];}
 if(field==AC_D_GLOBAL&&index>=48&&index<56){static const char*names[]={"TOTAL US","RENDER US","SCAN US","USB US","OTHER US","VOICES BEFORE","LIMIT US","SCAN IRQS"};return names[index-48];}
 if(field==AC_D_GLOBAL)return index==11?"HELD KEYS":index==12?"SHED VOICES":index==13?"RENDER US":index==14?"IRQ US":index==15?"OTHER US":index==7||index==42?"RANDOM LEFT":index<40?global[index]:"--";
 if(field==AC_D_ROUTE)return index<12?route[index]:"--";
 if(field==AC_D_MOD)return index<16?mods[index%8]:index<21?wt[index-16]:"--";
 if(field==AC_D_FX)return index<8?fx[index]:"--";
 if(field==AC_D_LEVEL)return"LANE LEVEL";
 if(field==AC_D_PAN)return"LANE PAN";
 if(field==AC_D_STEP_FIELD){static const char*names[4]={"NOTE","VELOCITY","GATE","PROBABILITY"};return names[index%4];}return"";
}
static const char*af_number(float value){static char text[24];if(!(value==value)||value>999999||value< -99999)return"--";int n=(int)(value*1000+(value<0?-.5f:.5f));unsigned at=0;if(n<0){text[at++]='-';n=-n;}unsigned integer=(unsigned)n/1000,frac=(unsigned)n%1000;char rev[12];unsigned r=0;do{rev[r++]=(char)('0'+integer%10);integer/=10;}while(integer);while(r)text[at++]=rev[--r];if(frac){text[at++]='.';text[at++]=(char)('0'+frac/100);if(frac%100){text[at++]=(char)('0'+frac/10%10);if(frac%10)text[at++]=(char)('0'+frac%10);}}text[at]=0;return text;}
const char*af_value(unsigned lane,unsigned slot,unsigned field,unsigned index){float v=af_get(lane,slot,field,index);if(!(v==v))return"--";int n=(int)v;
 if(field==AC_D_STATUS){
  if(index==0)return n?"MODIFIED":"CLEAN";
  if(index==1||index==6){static char name[12];if(n>=0&&n<(int)AF_PATCH_FACTORY)return af_patch_factory_name((unsigned)n);if(n>=(int)AF_PATCH_FACTORY&&n<(int)AF_PATCH_ENTRIES){unsigned user=(unsigned)n-AF_PATCH_FACTORY+1;memcpy(name,"USER 00",8);name[5]=(char)('0'+user/10);name[6]=(char)('0'+user%10);return name;}return n==(int)AF_PATCH_IMPORTED?"IMPORTED":"SESSION";}
  if(index==2)return n?"OCCUPIED":"EMPTY";
  if(index==3)return n==AF_PATCH_SAVE?"SAVE SOUND PATCH":n==AF_PATCH_LOAD?(af_patch_selection()>=AF_PATCH_ORIGINALS&&af_patch_selection()<AF_PATCH_FACTORY?"LOAD SOUND + SEQ":"LOAD SOUND PATCH"):"";
  if(index==4){static const char*names[5]={"READY","SAVED","LOADED","STORAGE FAILED","WORKING"};return n>=0&&n<5?names[n]:"--";}
  if(index==5){static const char*names[5]={"","CONFIRM OR CANCEL","STORAGE FAILED","INVALID PATCH","RANDOMIZE BUSY"};return n>=0&&n<5?names[n]:"--";}
 }
 if(field==AC_D_RANDOM_MASK||field==AC_D_HOLD||field==AC_D_SELECTED)return n?"ON":"OFF";
 if(field==AC_D_PARAM){if(index==0)return n>=0&&n<4?ab_wave_name(ab_wave_get((unsigned)n)):"--";if(index==AF_PARAM_WAVE)return ab_wave_name((unsigned)n);if(index==6)return n>=0&&n<5?animate_lfo_shape_labels[n]:"--";if(index==9)return n>=0&&n<8?animate_lfo_division_labels[n]:"--";if(index==13)return n>=0&&n<5?animate_filter_labels[n]:"--";if(index==8)return n?"ON":"OFF";}
 if(field==AC_D_ROUTE&&index==0){static const char*names[3]={"STEPS","PARAM","BOTH"};return n>=0&&n<3?names[n]:"--";}if(field==AC_D_ROUTE&&index==1){static const char*names[4]={"SOUND","PICK","PAGE","ALL"};return n>=0&&n<4?names[n]:"--";}if(field==AC_D_ROUTE&&(index==8||index==9)&&n<2)return n?"ALL":"SEL";
 if(field==AC_D_GLOBAL&&index==24)return animate_scale_name((unsigned)n,0);
 if(field==AC_D_GLOBAL&&index==20){static const char*names[6]={"VECTOR","SINE","SAW","BELL","VOICE","USER"};return n>=0&&n<6?names[n]:"--";}
 return af_number(v);
}
static float af_display_get(void*u,unsigned lane,unsigned slot,unsigned field,unsigned index){(void)u;return af_get(lane,slot,field,index);}
static const char*af_display_label(void*u,unsigned lane,unsigned slot,unsigned field,unsigned index){(void)u;return af_label(lane,slot,field,index);}
static const char*af_display_value(void*u,unsigned lane,unsigned slot,unsigned field,unsigned index){(void)u;return af_value(lane,slot,field,index);}
void af_draw(void){if(af.ui.view==AC_VIEW_VIS)af_visual_prepare(af.ui.visualizer);const ac_display_source source={0,af_display_get,af_display_label,af_display_value};ac_display_draw(&af.ui,&source);}
