#include "sequence.h"
#include <string.h>
static uint32_t as_random(uint32_t*x){uint32_t v=*x;v^=v<<13;v^=v>>17;v^=v<<5;return *x=v?v:1;}
static void as_event_send(as_sequence*s,unsigned lane,unsigned on,unsigned velocity,uint64_t sample,as_emit emit,void*context){if(emit){as_event e={sample,s->generation[lane],(uint8_t)lane,s->note[lane],(uint8_t)velocity,(uint8_t)on,s->current};emit(context,&e);}}
static void as_release(as_sequence*s,unsigned lane,uint64_t sample,as_emit emit,void*context){if(s->active[lane]){s->active[lane]=0;as_event_send(s,lane,0,0,sample,emit,context);}}
void as_init(as_sequence*s,unsigned rate){memset(s,0,sizeof *s);s->rate=rate?rate:44100;s->bpm=120;s->length=128;s->division=4;static const uint8_t notes[10]={48,50,52,55,57,60,62,64,67,69};for(unsigned l=0;l<10;l++){s->enabled[l]=1;s->probability[l]=100;s->rng[l]=0x5a173b19u+l*0x9e3779b9u;for(unsigned p=0;p<128;p++)s->steps[l][p]=(as_step){0,notes[l],100,100,75};}}
int as_edit(as_sequence*s,unsigned l,unsigned p,as_step v){if(l>=10||p>=128||v.enabled>1||v.note>127||v.velocity>127||v.probability>100||!v.gate||v.gate>100)return 0;s->steps[l][p]=v;return 1;}
int as_tempo(as_sequence*s,unsigned bpm){if(bpm<20||bpm>240)return 0;s->bpm=(uint16_t)bpm;s->remainder=0;return 1;}
int as_length(as_sequence*s,unsigned length){if(!length||length>128)return 0;s->length=(uint16_t)length;if(s->position>=length)s->position=0;return 1;}
int as_division(as_sequence*s,unsigned division){if(!division||division>32)return 0;s->division=(uint8_t)division;s->remainder=0;return 1;}
void as_stop(as_sequence*s,uint64_t sample,as_emit emit,void*context){if(sample<s->now)sample=s->now;for(unsigned l=0;l<10;l++)as_release(s,l,sample,emit,context);s->playing=0;s->now=sample;}
void as_play(as_sequence*s,uint64_t sample,as_emit emit,void*context){as_stop(s,sample,emit,context);s->playing=1;s->position=s->current=0;s->remainder=0;s->next_step=s->now;as_advance(s,s->now,emit,context);}
void as_advance(as_sequence*s,uint64_t sample,as_emit emit,void*context){
 if(sample<s->now)return;
 if(!s->playing){s->now=sample;return;}
 for(;;){uint64_t due=s->next_step;for(unsigned l=0;l<10;l++)if(s->active[l]&&s->off_at[l]<due)due=s->off_at[l];if(due>sample)break;
  for(unsigned l=0;l<10;l++)if(s->active[l]&&s->off_at[l]<=due)as_release(s,l,due,emit,context);
  if(s->next_step==due){unsigned denominator=s->bpm*s->division;uint64_t numerator=(uint64_t)s->rate*60+s->remainder,duration=numerator/denominator;s->remainder=(uint32_t)(numerator%denominator);if(!duration)duration=1;s->current=s->position;
   for(unsigned l=0;l<10;l++){as_step v=s->steps[l][s->position];unsigned probability=v.probability*s->probability[l];if(s->enabled[l]&&v.enabled&&v.velocity&&probability&&(probability==10000||as_random(&s->rng[l])%10000<probability)){as_release(s,l,due,emit,context);s->note[l]=v.note;s->active[l]=1;if(!++s->generation[l])++s->generation[l];uint64_t gate=duration*v.gate/100;s->off_at[l]=due+(gate?gate:1);as_event_send(s,l,1,v.velocity,due,emit,context);}}
   s->next_step=due+duration;s->position=(uint8_t)((s->position+1)%s->length);
  }
 }
 s->now=sample;
}
