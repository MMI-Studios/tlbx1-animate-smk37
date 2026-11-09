#include "project_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static aps_state whole,chunks,masked;
int main(void){
 float base[4][20];
 for(unsigned s=0;s<4;s++)for(unsigned p=0;p<20;p++)base[s][p]=animate_parameter_info(p)->desktop_default;
 for(unsigned scope=APS_SELECTED;scope<=APS_ALL;scope++){
  assert(aps_init(&whole,44100,base));assert(aps_init(&chunks,44100,base));
  for(unsigned step=0;step<128;step+=3){aps_select(&whole,step,1);aps_select(&chunks,step,1);}
  assert(aps_randomize(&whole,APS_BOTH,scope,-1,-1,5));
  for(unsigned at=0;at<1280;at++)assert(aps_randomize_range(&chunks,APS_BOTH,scope,-1,-1,5,31,at,1));
  assert(!memcmp(whole.sequence.steps,chunks.sequence.steps,sizeof whole.sequence.steps));
  assert(!memcmp(whole.locks,chunks.locks,sizeof whole.locks));assert(whole.random==chunks.random);
 }
 assert(aps_init(&masked,44100,base));as_step original=masked.sequence.steps[9][127];
 assert(aps_randomize_range(&masked,APS_BOTH,APS_ALL,9,3,0,1,1279,1));
 as_step result=masked.sequence.steps[9][127];
 assert(result.velocity==original.velocity&&result.gate==original.gate&&result.probability==original.probability&&result.enabled==original.enabled);
 float value;assert(ap_lock_get(&masked.locks[9][127][3],ANIMATE_LEVEL,&value));
 assert(!ap_lock_get(&masked.locks[9][126][3],ANIMATE_LEVEL,&value));
 assert(!ap_lock_get(&masked.locks[9][127][2],ANIMATE_LEVEL,&value));
 assert(!aps_randomize_range(&masked,APS_BOTH,APS_ALL,-1,-1,0,31,1280,1));
 puts("random: bounded chunks match full scopes exactly; disabled fields and other steps/slots retained");
 return 0;
}
