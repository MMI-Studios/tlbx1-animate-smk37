/* Behavioral reference of the TLBX-1 Animate engine (voice, envelope and
 * filter behaviour); no Melodee synthesis code.
 * Moog recurrence follows FunDSP 0.20.0 src/moog.rs (MIT, see LICENSES/FunDSP-MIT).
 * Deliberate correction: sample_start is initial position, loop_start is the
 * subsequent wrap boundary. Desktop currently aliases both to sample_start.
 * Sample mode retains desktop's freq/440 frame increment, independent of source
 * metadata; WT mode uses freq*loop_length/render_rate. Sample interpolation is
 * nearest as desktop; WT interpolation is linear. Math backend is separately
 * selectable/reference-tested. This module is not a complete firmware runtime. */
#include "desktop_voice.h"
#include "../src/animate_math.h"
#include <string.h>
#define AV_PI 3.14159265358979323846f
static int finite_value(float x){return x==x&&x<=3.402823466e38f&&x>=-3.402823466e38f;}
static float clamp(float x,float lo,float hi){if(!finite_value(x))return lo;return x<lo?lo:x>hi?hi:x;}
static float absolute(float x){return x<0?-x:x;}
static float random_unit(uint32_t *s){*s=*s*1664525u+1013904223u;return(float)(*s>>8)*(1.f/16777216.f);}
static float coefficient(float seconds,float rate){return seconds<=.001f?0.f:av_exp(-1.f/(seconds*rate));}
static av_lfo_params clean_lfo(av_lfo_params p){p.rate=clamp(p.rate,0,1);p.amount=clamp(p.amount,0,1);if(p.shape>4)p.shape=0;if(p.division>7)p.division=7;p.sync=!!p.sync;return p;}
float av_lfo_rate(const av_lfo_params *p,float tempo){
 static const float beats[8]={.25f,.5f,1,1.333333333333f,2,4,8,16};
 if(p->sync)return clamp(tempo,20,240)/60.f/beats[p->division<8?p->division:7];
 return .01f*av_exp(7.600902459542f*clamp(p->rate,0,1));
}
void av_patch_init(av_patch *p){
 memset(p,0,sizeof *p);p->x=p->y=.5f;p->lfo_x.rate=p->lfo_y.rate=.5f;
 for(unsigned i=0;i<4;i++){av_slot_params *s=&p->slot[i];s->level=s->loop_end=1;s->cutoff=.5f;s->attack=.01f;s->decay=.1f;s->sustain=.8f;s->release=.3f;s->wt_lfo.rate=.5f;}
}
int av_prepare(av_prepared *o,const av_patch *p,const av_source sources[4],float sr,float tempo){
 if(!o||!p||!sources||!finite_value(sr)||sr<1000||sr>384000)return 0;
 for(unsigned i=0;i<4;i++)if(sources[i].length>16777216u||(!sources[i].samples&&sources[i].length))return 0;
 memset(o,0,sizeof *o);o->params=*p;o->sample_rate=sr;o->params.x=clamp(p->x,0,1);o->params.y=clamp(p->y,0,1);
 o->params.lfo_x=clean_lfo(p->lfo_x);o->params.lfo_y=clean_lfo(p->lfo_y);
 o->x_increment=av_lfo_rate(&o->params.lfo_x,tempo)/sr;o->y_increment=av_lfo_rate(&o->params.lfo_y,tempo)/sr;
 for(unsigned i=0;i<4;i++){
  av_prepared_slot *q=&o->slot[i];av_slot_params s=p->slot[i];
  s.coarse=clamp(s.coarse,-48,48);s.fine=clamp(s.fine,-100,100);s.level=clamp(s.level,0,1);s.pan=clamp(s.pan,-1,1);
  s.sample_start=clamp(s.sample_start,0,1);s.loop_start=clamp(s.loop_start,0,1);s.loop_end=clamp(s.loop_end,0,1);
  s.cutoff=clamp(s.cutoff,.001f,1);s.resonance=clamp(s.resonance,0,.95f);
  s.attack=clamp(s.attack,0,5);s.decay=clamp(s.decay,0,10);s.sustain=clamp(s.sustain,0,1);s.release=clamp(s.release,0,10);
  if(s.source_type>1)s.source_type=0;if(s.filter_type>4)s.filter_type=1;s.wt_lfo=clean_lfo(s.wt_lfo);
  q->params=s;o->params.slot[i]=s;q->source=sources[i];uint32_t n=sources[i].length;
  if(n){q->start=(uint32_t)(s.sample_start*(n-1)+.5f);if(q->start>=n)q->start=n-1;q->loop=(uint32_t)(s.loop_start*(n-1)+.5f);if(q->loop>=n)q->loop=n-1;q->end=(uint32_t)(s.loop_end*n+.5f);if(q->end<=q->loop||q->end>n)q->end=n;if(q->start>=q->end)q->start=q->end-1;}
  q->pitch_ratio=av_pow2((s.coarse+s.fine*.01f)/12.f);
  q->increment_factor=q->pitch_ratio*(s.source_type==AV_SAMPLE?1.f/440.f:(float)(q->end-q->loop)/sr);
  q->attack=coefficient(s.attack,sr);q->decay=coefficient(s.decay,sr);q->release=coefficient(s.release,sr);
  q->lfo_increment=av_lfo_rate(&s.wt_lfo,tempo)/sr;
  float hz=20.f+s.cutoff*s.cutoff*19980.f;
  q->filter_f=clamp(2.f*av_sin(AV_PI*hz/sr),0,.99f);q->filter_q=1.f-s.resonance;
  /* Keep the ladder below Nyquist at reduced render rates. At desktop44.1k
   * this clamp is inactive across the complete20..20000Hz parameter range. */
  float c=2.f*clamp(hz,0,sr*.49f)/sr;q->moog_p=c*(1.8f-.8f*c);q->moog_k=2.f*av_sin(c*AV_PI*.5f)-1;
  float t1=(1-q->moog_p)*1.386249f,t2=12+t1*t1;q->moog_rez=(.1f+s.resonance*.9f)*(t2+6*t1)/(t2-6*t1);
 }
 return 1;
}
static float lfo_tick(av_lfo_state *s,const av_lfo_params *p,float increment,uint32_t *rng){
 float value=0;switch(p->shape){case 0:value=av_sin(2*AV_PI*s->phase);break;case 1:value=1-4*absolute(s->phase-.5f);break;case 2:value=s->phase<.5f?1:-1;break;case 3:value=2*s->phase-1;break;case 4:value=s->held;break;}
 s->phase+=increment;if(s->phase>=1){s->phase-=(unsigned)s->phase;s->held=random_unit(rng)*2-1;}return value;
}
void av_vector_init(av_vector_state *s,float x,float y,uint32_t seed){memset(s,0,sizeof *s);s->x_smooth=clamp(x,0,1);s->y_smooth=clamp(y,0,1);s->random=seed;}
void av_vector_tick(av_vector_state *s,const av_prepared *p,float w[4]){
 float x=clamp(p->params.x+lfo_tick(&s->x,&p->params.lfo_x,p->x_increment,&s->random)*p->params.lfo_x.amount,0,1);
 float y=clamp(p->params.y+lfo_tick(&s->y,&p->params.lfo_y,p->y_increment,&s->random)*p->params.lfo_y.amount,0,1);
 s->x_smooth=s->x_smooth*.999f+x*.001f;s->y_smooth=s->y_smooth*.999f+y*.001f;
 w[0]=(1-s->x_smooth)*(1-s->y_smooth);w[1]=s->x_smooth*(1-s->y_smooth);w[2]=(1-s->x_smooth)*s->y_smooth;w[3]=s->x_smooth*s->y_smooth;
}
void av_voice_init(av_voice *v,uint32_t seed){memset(v,0,sizeof *v);v->random=seed;}
void av_voice_on(av_voice *v,const av_prepared *p,float note,float velocity){
 v->frequency=440.f*av_pow2((clamp(note,0,127)-69)/12);v->velocity=clamp(velocity,0,1);v->active=1;
 for(unsigned i=0;i<4;i++){v->slot[i].position=(float)p->slot[i].start;v->slot[i].envelope=0;v->slot[i].stage=1;v->slot[i].looped=0;}
}
void av_voice_off(av_voice *v){for(unsigned i=0;i<4;i++)if(v->slot[i].stage)v->slot[i].stage=4;}
float av_source_tick(av_slot_state *s,const av_prepared_slot *p,float frequency,float lfo){
 if(!p->source.samples||!p->source.length||p->end<=p->loop)return 0;
 unsigned lo=s->looped?p->loop:(p->start<p->loop?p->start:p->loop),end=p->end,width=end-lo;
 float pos=s->position;if(!finite_value(pos)||pos<lo||pos>=end)pos=(float)(s->looped?p->loop:p->start);
 unsigned base=(unsigned)pos,local=base-lo,max_offset=(unsigned)((width-1)*p->params.wt_lfo.amount+.5f);
 unsigned offset=(unsigned)((clamp(lfo,-1,1)*.5f+.5f)*max_offset+.5f),idx=lo+(local+offset)%width;
 float value=p->source.samples[idx];if(p->params.source_type==AV_WAVETABLE){unsigned next=lo+(local+offset+1)%width;value+=(p->source.samples[next]-value)*(pos-base);}
 float next=pos+clamp(frequency,0,200000)*p->increment_factor;
 if(next>=end){s->looped=1;float span=(float)(end-p->loop),distance=next-p->loop;next=p->loop+(distance-(unsigned)(distance/span)*span);if(next>=end||next<p->loop)next=(float)p->loop;}
 s->position=next;return finite_value(value)?value:0;
}
static float filter_tick(av_slot_state *s,const av_prepared_slot *p,float x){
 float low=s->low+p->filter_f*s->band,high=x-low-p->filter_q*s->band,band=p->filter_f*high+s->band;
 float low2=s->low2+p->filter_f*s->band2,high2=low-low2-p->filter_q*s->band2,band2=p->filter_f*high2+s->band2;
 float result;switch(p->params.filter_type){case AV_LOW24:result=low2;break;case AV_HIGH:result=high;break;case AV_BAND:result=band;break;
 case AV_MOOG:{float input=x-p->moog_rez*s->moog[3];s->moog[0]=(input+s->previous[0])*p->moog_p-p->moog_k*s->moog[0];for(unsigned i=1;i<4;i++)s->moog[i]=(s->moog[i-1]+s->previous[i])*p->moog_p-p->moog_k*s->moog[i];s->moog[3]=av_tanh(s->moog[3]);s->previous[0]=input;for(unsigned i=1;i<4;i++)s->previous[i]=s->moog[i-1];result=s->moog[3];break;}
 default:result=low;break;}
 if(!finite_value(result)||!finite_value(band2)){s->band=s->low=s->band2=s->low2=0;memset(s->moog,0,sizeof s->moog);memset(s->previous,0,sizeof s->previous);return x;}
 s->band=band;s->low=low;s->band2=band2;s->low2=low2;return result;
}
static void envelope_tick(av_slot_state *s,const av_prepared_slot *p){
 switch(s->stage){case 1:s->envelope=s->envelope*p->attack+1.1f*(1-p->attack);if(s->envelope>=1){s->envelope=1;s->stage=2;}break;
 case 2:s->envelope=s->envelope*p->decay+p->params.sustain*(1-p->decay);if(absolute(s->envelope-p->params.sustain)<.001f){s->envelope=p->params.sustain;s->stage=3;}break;
 case 3:s->envelope=p->params.sustain;break;case 4:s->envelope*=p->release;if(s->envelope<.0001f){s->envelope=0;s->stage=0;}break;default:s->envelope=0;break;}
}
void av_voice_tick(av_voice *v,const av_prepared *p,const float weights[4],float out[2]){
 out[0]=out[1]=0;if(!v->active)return;unsigned active=0;
 for(unsigned i=0;i<4;i++){av_slot_state *s=&v->slot[i];const av_prepared_slot *q=&p->slot[i];envelope_tick(s,q);active|=s->stage;
  float lfo=lfo_tick(&s->lfo,&q->params.wt_lfo,q->lfo_increment,&v->random),x=av_source_tick(s,q,v->frequency,lfo);
  x=filter_tick(s,q,x)*s->envelope*q->params.level*clamp(weights[i],0,1)*v->velocity;
  out[0]+=x*(q->params.pan>0?1-q->params.pan:1);out[1]+=x*(q->params.pan<0?1+q->params.pan:1);
 }v->active=!!active;
}
