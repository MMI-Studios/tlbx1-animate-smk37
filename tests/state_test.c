#include "project_state.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static aps_state state,restored;
static unsigned char data[110000];
static float base[4][20];
static as_event events[6000];static unsigned event_count;
static void emit(void*context,const as_event*e){(void)context;assert(event_count<6000);events[event_count++]=*e;}
static float low(unsigned p){return p==0?0:animate_parameter_info(p)->stored_min;}
static float high(unsigned p){return p==0?3:animate_parameter_info(p)->stored_max;}
static void fixture(void){for(unsigned s=0;s<4;s++)for(unsigned p=0;p<20;p++)base[s][p]=animate_parameter_info(p)->desktop_default;/* Test fixture only; production init must come from preserved baseline. */}
static void codec(void){
 assert(sizeof(ap_lock)==19);ap_lock lock;ap_lock_clear(&lock);assert(ap_lock_valid(&lock));float value;
 for(unsigned p=0;p<20;p++){
  assert(!ap_lock_get(&lock,p,&value));assert(ap_lock_set(&lock,p,low(p)));assert(ap_lock_get(&lock,p,&value)&&fabsf(value-low(p))<.00001f);
  assert(ap_lock_set(&lock,p,high(p)));assert(ap_lock_get(&lock,p,&value)&&fabsf(value-high(p))<.00001f);
  ap_lock_unset(&lock,p);assert(!ap_lock_get(&lock,p,&value));
 }
 assert(!ap_lock_set(&lock,0,4)&&!ap_lock_set(&lock,1,NAN));
 lock.bytes[18]|=128;assert(!ap_lock_valid(&lock));
 fixture();assert(aps_init(&state,8000,base));assert(aps_init(&state,8000,state.base));assert(state.base[0][ANIMATE_RELEASE]==base[0][ANIMATE_RELEASE]);
 for(unsigned l=0;l<10;l++)for(unsigned step=0;step<128;step++)for(unsigned slot=0;slot<4;slot++)for(unsigned p=0;p<20;p++){
  float v=low(p)+(high(p)-low(p))*((l*128+step+slot*37+p*19)%1001)/1000.f;
  if(p==0||p==6||p==8||p==9||p==13)v=(float)(int)(v+.5f);
  assert(aps_lock_set(&state,l,step,slot,p,v));float decoded=aps_value(&state,l,step,slot,p);
  assert(fabsf(v-decoded)<=ap_parameter_resolution(p)*.501f+.00001f);
 }
 printf("dense codec: %zu lock bytes, complete state %zu bytes\n",sizeof state.locks,sizeof state);
 assert(sizeof state.locks==97280);
}
static void sequence(void){
 as_sequence seq;as_init(&seq,8000);for(unsigned l=0;l<10;l++)for(unsigned step=0;step<128;step++)assert(as_edit(&seq,l,step,(as_step){1,60,100,100,50}));
 event_count=0;as_play(&seq,0,emit,0);assert(event_count==10);for(unsigned i=0;i<10;i++)assert(events[i].on&&events[i].lane==i&&events[i].generation==1);
 as_advance(&seq,499,emit,0);assert(event_count==10);as_advance(&seq,500,emit,0);assert(event_count==20);for(unsigned i=10;i<20;i++)assert(!events[i].on&&events[i].generation==events[i-10].generation);
 assert(as_edit(&seq,3,1,(as_step){1,87,112,100,100}));as_advance(&seq,1000,emit,0);assert(events[23].note==87&&events[23].on&&events[23].generation==2);
 as_advance(&seq,127000,emit,0);int last=0;for(unsigned i=0;i<event_count;i++)if(events[i].on&&events[i].step==127)last++;assert(last==10);
 as_stop(&seq,127010,emit,0);assert(!seq.playing);for(unsigned l=0;l<10;l++)assert(!seq.active[l]);
 /* Independent lane RNG: muting lane0 cannot perturb lane1 probability. */
 as_sequence a,b;as_init(&a,8000);as_init(&b,8000);for(unsigned l=0;l<2;l++)for(unsigned p=0;p<128;p++)a.steps[l][p]=b.steps[l][p]=(as_step){1,60,100,37,50};b.enabled[0]=0;
 as_play(&a,0,0,0);as_play(&b,0,0,0);as_advance(&a,127000,0,0);as_advance(&b,127000,0,0);assert(a.rng[1]==b.rng[1]&&a.generation[1]==b.generation[1]);
}
static void randomization(void){
 fixture();assert(aps_init(&state,8000,base));for(unsigned s=0;s<4;s++)state.mask[s]=1u<<2;
 aps_select(&state,3,1);aps_select(&state,127,1);assert(aps_randomize(&state,APS_PARAMS,APS_SELECTED,7,2,0));
 float value;assert(ap_lock_get(&state.locks[7][3][2],2,&value));assert(!ap_lock_get(&state.locks[7][4][2],2,&value));assert(!ap_lock_get(&state.locks[7][3][1],2,&value));assert(!ap_lock_get(&state.locks[6][3][2],2,&value));
 float fine=aps_value(&state,7,3,2,2);state.mask[2]=1u<<3;assert(aps_randomize(&state,APS_PARAMS,APS_SELECTED,7,2,0));assert(aps_value(&state,7,3,2,2)==fine);
 assert(aps_randomize(&state,APS_BOTH,APS_PAGE,7,2,7));assert(ap_lock_get(&state.locks[7][112][2],3,&value));assert(!ap_lock_get(&state.locks[7][111][2],3,&value));
 as_play(&state.sequence,0,0,0);assert(aps_randomize(&state,APS_BOTH,APS_ALL,-1,-1,0));assert(state.sequence.playing);
 state.param_amount=0;float old=state.base[2][3];assert(aps_randomize(&state,APS_PARAMS,APS_SOUND,-1,2,0)&&state.base[2][3]==old);
 assert(aps_copy(&state,7,112,0,0,16));assert(!memcmp(state.locks[0][0],state.locks[7][112],sizeof state.locks[0][0]));aps_clear(&state,0,0,16,1,1);assert(!ap_lock_get(&state.locks[0][0][2],3,&value));
}
static void persistence(void){
 codec();state.mask[0]=7;state.glide[9]=.75f;as_play(&state.sequence,0,0,0);unsigned size=aps_project_size(&state);assert(size<sizeof data);
 for(unsigned at=0;at<size;){unsigned n=aps_project_read(&state,at,data+at,1013);assert(n);at+=n;}
 fixture();assert(aps_init(&restored,44100,base));assert(aps_project_import(&restored,data,size));assert(restored.sequence.rate==44100&&!restored.sequence.playing&&restored.mask[0]==7&&restored.glide[9]==.75f);assert(!memcmp(restored.locks,state.locks,sizeof state.locks));
 as_play(&restored.sequence,0,0,0);data[size-1]^=128;assert(!aps_project_import(&restored,data,size)&&restored.sequence.playing);
 uint32_t crc=2166136261u;for(unsigned i=16;i<size;i++)crc=(crc^data[i])*16777619u;memcpy(data+12,&crc,4);assert(!aps_project_import(&restored,data,size)&&restored.sequence.playing);assert(!aps_project_import(&restored,data,size-1));
 printf("project serialized bytes %u; corruption rejected before state replacement\n",size);
}
int main(void){codec();sequence();randomization();persistence();puts("PASS: all5120x20 lock fields, ten live lanes, ownership generations, scopes and atomic import");return 0;}
