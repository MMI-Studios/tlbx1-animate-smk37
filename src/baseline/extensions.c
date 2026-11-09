/* SPDX-License-Identifier: GPL-3.0-only
 * Explicit controls added around the preserved integer Animate kernel.
 * With no extension edits/locks, call the original renderer unchanged.
 * Seconds remain linear ADSR durations; they do not replace the baseline with
 * desktop exponential envelopes. Four sources remain real wavetable cycles.
 */
static float bridge_values[4][20],bridge_locks[NVOICE][4][20];
static uint32_t bridge_masks[4],bridge_lock_masks[NVOICE][4];
static int32_t bridge_pan_l[CTL],bridge_pan_r[CTL];
static float bridge_env[NVOICE][4],bridge_filter[NVOICE][4][4];
static uint32_t bridge_noise[NVOICE][4];
static uint8_t bridge_looped[NVOICE][4];
static float bridge_vector_x,bridge_vector_y;
static unsigned bridge_vector_override;
static float bridge_vector_lfo[2][5];
static uint32_t bridge_vector_lfo_masks[2],bridge_vector_noise[NVOICE][2];
static inline __attribute__((always_inline)) float bridge_clampf(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static float bridge_log_rate(float hz){float lo=0,hi=1;for(unsigned i=0;i<24;i++){float m=(lo+hi)*.5f;if(.01f*av_pow2(m*10.9657843f)<hz)lo=m;else hi=m;}return (lo+hi)*.5f;}
static float bridge_seconds(int p){return (1.f+(float)p*p*FS/4096.f)/FS;}
static void bridge_native_values(unsigned s,float v[20]){
 int16_t*p=animate_config_of(trk)->slot[s],*w=animate_motion[0].slot[s];
 v[0]=p[0];v[1]=p[2];v[2]=0;v[3]=p[1]/127.f;v[4]=0;v[5]=w[4]/127.f;
 v[6]=w[2]==2?3:w[2]==3?2:w[2];v[7]=bridge_log_rate(w[3]*4.f/127.f);v[8]=v[9]=0;
 v[10]=v[11]=w[0]/127.f;v[12]=w[1]/127.f;v[13]=1;v[14]=p[3]/127.f;v[15]=p[4]*.95f/127.f;
 v[16]=bridge_seconds(p[6]);v[17]=bridge_seconds(p[7]);v[18]=p[8]/127.f;v[19]=bridge_seconds(p[9]);
}
static void bridge_extensions_reset(void){
 memset(bridge_masks,0,sizeof bridge_masks);memset(bridge_lock_masks,0,sizeof bridge_lock_masks);
 bridge_vector_x=trk[0].p[P_E0]/127.f;bridge_vector_y=trk[0].p[P_E1]/127.f;bridge_vector_override=0;
 memset(bridge_vector_lfo_masks,0,sizeof bridge_vector_lfo_masks);
 for(unsigned axis=0;axis<2;axis++){animate_config_t*c=animate_config_of(trk);int shape=axis?c->yshape:c->xshape;float*p=bridge_vector_lfo[axis];p[0]=shape==2?3:shape==3?2:shape;p[1]=(axis?c->yrate:trk[0].p[P_E2])*4.f/127.f;p[2]=(axis?c->ydepth:trk[0].p[P_E3])/127.f;p[3]=p[4]=0;}
 for(unsigned s=0;s<4;s++){
  int16_t*p=animate_config_of(trk)->slot[s],*w=animate_motion[0].slot[s];float*v=bridge_values[s];
  v[0]=p[0];v[1]=p[2];v[2]=0;v[3]=p[1]/127.f;v[4]=0;v[5]=w[4]/127.f;
  v[6]=w[2]==2?3:w[2]==3?2:w[2];v[7]=bridge_log_rate(w[3]*4.f/127.f);v[8]=v[9]=0;
  v[10]=v[11]=w[0]/127.f;v[12]=w[1]/127.f;v[13]=1;v[14]=p[3]/127.f;v[15]=p[4]*.95f/127.f;
  v[16]=bridge_seconds(p[6]);v[17]=bridge_seconds(p[7]);v[18]=p[8]/127.f;v[19]=bridge_seconds(p[9]);
 }
}
static int bridge_value_valid(unsigned id,float x){
 if(id>=20||!(x==x))return 0;
 static const float lo[20]={0,-48,-100,0,-1,0,0,0,0,0,0,0,0,0,.001f,0,0,0,0,0};
 static const float hi[20]={3,48,100,1,1,1,4,1,1,7,1,1,1,4,1,.95f,5,10,1,10};
 if(x<lo[id]||x>hi[id])return 0;
 return (id!=0&&id!=6&&id!=8&&id!=9&&id!=13)||x==(float)(int)x;
}
int ab_slot_parameter(unsigned slot,unsigned id,float value){
 if(slot>=4||!bridge_value_valid(id,value))return 0;
 float native[20];bridge_native_values(slot,native);
 bridge_values[slot][id]=value;
 /* Exact equality only: never quantize a genuinely extended value. Restoring
  * all filter controls also restores the native slot's bypass behavior. */
 if(value==native[id])
  bridge_masks[slot]&=~(1u<<id);
 else bridge_masks[slot]|=1u<<id;
 if(id>=13&&id<=15)bridge_masks[slot]&=~(1u<<20);
 return 1;
}
float ab_slot_parameter_get(unsigned slot,unsigned id){return slot<4&&id<20?bridge_values[slot][id]:0.f;}
uint32_t ab_slot_parameter_mask(unsigned slot){return slot<4?bridge_masks[slot]:0;}
int ab_slot_parameter_mask_set(unsigned slot,uint32_t mask){if(slot>=4||mask&~0x1fffffu)return 0;bridge_masks[slot]=mask;return 1;}
static int bridge_handle_valid(uint32_t h){return h&&trk[0].v[h&15].active&&trk[0].v[h&15].age==(h>>4);}
int ab_voice_parameter(uint32_t handle,unsigned slot,unsigned id,float value){if(!bridge_handle_valid(handle)||slot>=4||!bridge_value_valid(id,value))return 0;unsigned vi=handle&15;bridge_locks[vi][slot][id]=value;bridge_lock_masks[vi][slot]|=1u<<id;return 1;}
void ab_voice_parameters_clear(uint32_t handle){if(bridge_handle_valid(handle))memset(bridge_lock_masks[handle&15],0,sizeof bridge_lock_masks[0]);}
static void bridge_voice_started(voice_t*v){unsigned vi=(unsigned)(v-trk[0].v);memset(bridge_lock_masks[vi],0,sizeof bridge_lock_masks[0]);memset(bridge_env[vi],0,sizeof bridge_env[0]);memset(bridge_filter[vi],0,sizeof bridge_filter[0]);memset(bridge_looped[vi],0,sizeof bridge_looped[0]);for(unsigned s=0;s<4;s++)bridge_noise[vi][s]=0x9e3779b9u+vi*17+s;for(unsigned a=0;a<2;a++)bridge_vector_noise[vi][a]=0x8512ec5u+vi*19+a*733;}
static void bridge_pan_clear(void){memset(bridge_pan_l,0,sizeof bridge_pan_l);memset(bridge_pan_r,0,sizeof bridge_pan_r);}
static inline __attribute__((always_inline)) int32_t bridge_lfo(uint32_t phase,unsigned shape,uint32_t noise){return shape==2?(phase&0x80000000u?-32767:32767):shape==3?(int32_t)(phase>>16)-32768:shape==4?(int32_t)(noise>>16)-32768:shape==1?osc_tri(phase):osc_sine(phase);}
static void bridge_render(track_t*t,voice_t*v,int32_t*out,uint32_t n,const vmod_t*m);
int ab_vector(float x,float y){if(!(x>=0&&x<=1&&y>=0&&y<=1))return 0;bridge_vector_x=x;bridge_vector_y=y;bridge_vector_override=x!=trk[0].p[P_E0]/127.f||y!=trk[0].p[P_E1]/127.f;return 1;}
float ab_vector_get(unsigned axis){return axis?bridge_vector_y:bridge_vector_x;}
int ab_vector_lfo(unsigned axis,unsigned field,float value){if(axis>1||field>4||!(value==value))return 0;static const float hi[5]={4,20,1,1,7};if(value<0||value>hi[field]||((field==0||field>=3)&&value!=(float)(int)value))return 0;bridge_vector_lfo[axis][field]=value;bridge_vector_lfo_masks[axis]|=1u<<field;return 1;}
float ab_vector_lfo_get(unsigned axis,unsigned field){return axis<2&&field<5?bridge_vector_lfo[axis][field]:0;}
unsigned ab_vector_lfo_mask(unsigned axis){return axis<2?bridge_vector_lfo_masks[axis]:0;}
int ab_vector_lfo_mask_set(unsigned axis,unsigned mask){if(axis>1||mask&~31u)return 0;bridge_vector_lfo_masks[axis]=mask;return 1;}
#include "extended_render.c"
#include "voice_kernel.c"
