#include "project_state.h"
#include <string.h>
static int aps_value_valid(unsigned p,float v){if(p>=20)return 0;const animate_parameter_metadata*m=animate_parameter_info(p);if(p==ANIMATE_SOURCE)return v>=0&&v<=3&&v==(int)v;return v>=m->stored_min&&v<=m->stored_max&&((p==6||p==8||p==9||p==13)?v==(int)v:1);}
int aps_init(aps_state*s,unsigned rate,const float base[4][20]){if(!base)return 0;for(unsigned slot=0;slot<4;slot++)for(unsigned p=0;p<20;p++)if(!aps_value_valid(p,base[slot][p]))return 0;float initial[4][20];memcpy(initial,base,sizeof initial);memset(s,0,sizeof *s);as_init(&s->sequence,rate);memcpy(s->base,initial,sizeof s->base);for(unsigned l=0;l<10;l++)for(unsigned step=0;step<128;step++)for(unsigned slot=0;slot<4;slot++)ap_lock_clear(&s->locks[l][step][slot]);for(unsigned slot=0;slot<4;slot++)s->mask[slot]=(1u<<19)-1;s->note_amount=s->param_amount=1;s->random=0x2468ace1;return 1;}
int aps_base_set(aps_state*s,unsigned slot,unsigned p,float v){if(slot>=4||!aps_value_valid(p,v))return 0;s->base[slot][p]=v;return 1;}
int aps_lock_set(aps_state*s,unsigned l,unsigned step,unsigned slot,unsigned p,float v){return l<10&&step<128&&slot<4&&aps_value_valid(p,v)?ap_lock_set(&s->locks[l][step][slot],p,v):0;}
float aps_value(const aps_state*s,unsigned l,unsigned step,unsigned slot,unsigned p){if(l>=10||step>=128||slot>=4||p>=20)return 0;float value;return ap_lock_get(&s->locks[l][step][slot],p,&value)?value:s->base[slot][p];}
void aps_select(aps_state*s,unsigned step,int selected){if(step>=128)return;uint32_t bit=1u<<(step%32);if(selected)s->selected[step/32]|=bit;else s->selected[step/32]&=~bit;}
static uint32_t aps_random(aps_state*s){uint32_t x=s->random;x^=x<<13;x^=x>>17;x^=x<<5;return s->random=x?x:1;}
static float aps_unit(aps_state*s){return(aps_random(s)>>8)*(1.0f/16777216.0f);}
static float aps_random_parameter(aps_state*s,unsigned slot,unsigned p,const ap_lock*lock){const animate_parameter_metadata*m=animate_parameter_info(p);float base=s->base[slot][p],low=m->stored_min,high=m->stored_max;if(p==0)return aps_unit(s)<s->param_amount?(float)(aps_random(s)%4):base;if(p==6||p==8||p==9||p==13)return(float)(aps_random(s)%((unsigned)high+1));if(p==10)high=.95f;if(p==11)high=.9f;if(p==12){float start=s->base[slot][11];if(lock)(void)ap_lock_get(lock,11,&start);low=start+.05f;if(low>1)low=1;if(base<low)base=low;}float target=low+(high-low)*aps_unit(s);return base+(target-base)*s->param_amount;}
int aps_randomize_range(aps_state*s,unsigned mode,unsigned scope,int lane,int slot,unsigned page,unsigned step_mask,unsigned first,unsigned count){
 if(step_mask>31||first>1280||count>1280-first)return 0;
 if(mode>2||scope>3||lane < -1||lane>=10||slot < -1||slot>=4||page>=8||!(s->note_amount>=0&&s->note_amount<=1&&s->param_amount>=0&&s->param_amount<=1))return 0;
 unsigned first_slot=slot<0?0:(unsigned)slot,last_slot=slot<0?4:(unsigned)slot+1;
 if(scope==APS_SOUND){if(s->param_amount>0)for(unsigned sl=first_slot;sl<last_slot;sl++)for(unsigned p=0;p<20;p++)if(s->mask[sl]&(1u<<animate_parameter_info(p)->random_mask_bit))s->base[sl][p]=aps_random_parameter(s,sl,p,0);return 1;}
 for(unsigned at=first;at<first+count;at++){unsigned l=at/128,step=at%128;if(lane>=0&&l!=(unsigned)lane)continue;
  if(scope==APS_SELECTED&&!(s->selected[step/32]&(1u<<(step%32))))continue;
  if(scope==APS_PAGE&&step/16!=page)continue;
  if(mode!=APS_PARAMS&&s->note_amount>0){as_step*v=&s->sequence.steps[l][step];float a=s->note_amount;as_step next;
   next.enabled=aps_unit(s)>.75f-a*.5f;next.note=(uint8_t)(36+aps_random(s)%49);next.probability=(uint8_t)((1+(.25f+.75f*aps_unit(s)-1)*a)*100+.5f);next.velocity=(uint8_t)((1+(.2f+.8f*aps_unit(s)-1)*a)*127+.5f);next.gate=(uint8_t)((1+(.1f+.9f*aps_unit(s)-1)*a)*100+.5f);
   if(step_mask&1)v->note=next.note;
   if(step_mask&2)v->velocity=next.velocity;
   if(step_mask&4)v->gate=next.gate;
   if(step_mask&8)v->probability=next.probability;
   if(step_mask&16)v->enabled=next.enabled;
  }
  if(mode!=APS_STEPS&&s->param_amount>0)for(unsigned sl=first_slot;sl<last_slot;sl++)for(unsigned p=0;p<20;p++)if(s->mask[sl]&(1u<<animate_parameter_info(p)->random_mask_bit))ap_lock_set(&s->locks[l][step][sl],p,aps_random_parameter(s,sl,p,&s->locks[l][step][sl]));
 }
 return 1;
}
int aps_randomize(aps_state*s,unsigned mode,unsigned scope,int lane,int slot,unsigned page){return aps_randomize_range(s,mode,scope,lane,slot,page,31,0,1280);}
void aps_clear(aps_state*s,unsigned lane,unsigned first,unsigned count,int notes,int parameters){if(lane>=10||first>=128||count>128-first)return;for(unsigned i=first;i<first+count;i++){if(notes)s->sequence.steps[lane][i].enabled=0;if(parameters)for(unsigned sl=0;sl<4;sl++)ap_lock_clear(&s->locks[lane][i][sl]);}}
int aps_copy(aps_state*s,unsigned fl,unsigned fs,unsigned tl,unsigned ts,unsigned count){if(fl>=10||tl>=10||fs>=128||ts>=128||count>128-fs||count>128-ts)return 0;if(fl==tl&&fs==ts)return 1;/* memmove semantics without requiring a donor libc implementation. */for(unsigned n=0;n<count;n++){unsigned i=fl==tl&&ts>fs?count-1-n:n;s->sequence.steps[tl][ts+i]=s->sequence.steps[fl][fs+i];memcpy(s->locks[tl][ts+i],s->locks[fl][fs+i],sizeof s->locks[0][0]);}return 1;}
/* Little-endian IEEE754 format1; runtime clocks/voices are intentionally absent. */
typedef struct {float base[4][20],glide[10],note_amount,param_amount;uint32_t mask[4],selected[4],random;uint16_t bpm,length;uint8_t division,enabled[10],probability[10],reserved[3];} aps_settings;
#define APS_FILE_SIZE (16u+sizeof(aps_settings)+sizeof(((as_sequence*)0)->steps)+sizeof(((aps_state*)0)->locks))
static void aps_settings_make(const aps_state*s,aps_settings*p){memset(p,0,sizeof *p);memcpy(p->base,s->base,sizeof p->base);memcpy(p->glide,s->glide,sizeof p->glide);p->note_amount=s->note_amount;p->param_amount=s->param_amount;memcpy(p->mask,s->mask,sizeof p->mask);memcpy(p->selected,s->selected,sizeof p->selected);p->random=s->random;p->bpm=s->sequence.bpm;p->length=s->sequence.length;p->division=s->sequence.division;memcpy(p->enabled,s->sequence.enabled,10);memcpy(p->probability,s->sequence.probability,10);}
static uint32_t aps_crc(uint32_t h,const void*data,unsigned size){const uint8_t*p=data;while(size--)h=(h^*p++)*16777619u;return h;}
unsigned aps_project_size(aps_state*s){aps_settings settings;aps_settings_make(s,&settings);s->export_header[0]=0x50494e41;s->export_header[1]=1;s->export_header[2]=APS_FILE_SIZE;s->export_header[3]=aps_crc(aps_crc(aps_crc(2166136261u,&settings,sizeof settings),s->sequence.steps,sizeof s->sequence.steps),s->locks,sizeof s->locks);return APS_FILE_SIZE;}
unsigned aps_project_read(const aps_state*s,unsigned offset,void*out,unsigned count){if(offset>=APS_FILE_SIZE||!out)return 0;if(count>APS_FILE_SIZE-offset)count=APS_FILE_SIZE-offset;aps_settings settings;aps_settings_make(s,&settings);const uint8_t*segments[4]={(const uint8_t*)s->export_header,(const uint8_t*)&settings,(const uint8_t*)s->sequence.steps,(const uint8_t*)s->locks};unsigned sizes[4]={16,sizeof settings,sizeof s->sequence.steps,sizeof s->locks},written=0;for(unsigned i=0;i<4&&written<count;i++){if(offset>=sizes[i]){offset-=sizes[i];continue;}unsigned n=sizes[i]-offset;if(n>count-written)n=count-written;memcpy((uint8_t*)out+written,segments[i]+offset,n);written+=n;offset=0;}return written;}
int aps_project_import(aps_state*s,const void*data,unsigned size){
 if(!data||size!=APS_FILE_SIZE)return 0;
 const uint8_t*bytes=data;uint32_t header[4];memcpy(header,bytes,16);if(header[0]!=0x50494e41||header[1]!=1||header[2]!=size||header[3]!=aps_crc(2166136261u,bytes+16,size-16))return 0;
 aps_settings p;memcpy(&p,bytes+16,sizeof p);if(p.bpm<20||p.bpm>240||!p.length||p.length>128||!p.division||p.division>32||!p.random||!(p.note_amount>=0&&p.note_amount<=1&&p.param_amount>=0&&p.param_amount<=1))return 0;
 for(unsigned sl=0;sl<4;sl++){if(p.mask[sl]&~((1u<<19)-1))return 0;for(unsigned i=0;i<20;i++)if(!aps_value_valid(i,p.base[sl][i]))return 0;}
 for(unsigned l=0;l<10;l++)if(p.enabled[l]>1||p.probability[l]>100||!(p.glide[l]>=0&&p.glide[l]<=1))return 0;
 const uint8_t*step_bytes=bytes+16+sizeof p;for(unsigned i=0;i<1280;i++){as_step v;memcpy(&v,step_bytes+i*sizeof v,sizeof v);if(v.enabled>1||v.note>127||v.velocity>127||v.probability>100||!v.gate||v.gate>100)return 0;}
 const uint8_t*lock_bytes=step_bytes+sizeof s->sequence.steps;for(unsigned i=0;i<5120;i++){ap_lock lock;memcpy(&lock,lock_bytes+i*sizeof lock,sizeof lock);if(!ap_lock_valid(&lock))return 0;}
 if(!s)return 1;
 unsigned rate=s->sequence.rate;if(!aps_init(s,rate,p.base))return 0;memcpy(s->sequence.steps,step_bytes,sizeof s->sequence.steps);memcpy(s->locks,lock_bytes,sizeof s->locks);memcpy(s->glide,p.glide,sizeof p.glide);memcpy(s->mask,p.mask,sizeof p.mask);memcpy(s->selected,p.selected,sizeof p.selected);s->note_amount=p.note_amount;s->param_amount=p.param_amount;s->random=p.random;s->sequence.bpm=p.bpm;s->sequence.length=p.length;s->sequence.division=p.division;memcpy(s->sequence.enabled,p.enabled,10);memcpy(s->sequence.probability,p.probability,10);return 1;
}
int aps_project_validate(const void*bytes,unsigned size){return aps_project_import(0,bytes,size);}
