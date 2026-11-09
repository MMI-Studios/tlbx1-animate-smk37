/* Exercise the shared Melodee host, including the actual overload policy. */
#include <assert.h>
#include <stdio.h>
#include "baseline/bridge.c"
static void blocks(unsigned n){int32_t pcm[64];while(n--)ab_render(pcm,32);}
int main(void){
 /* All slot filter inputs, including cache hits and alternating lock values,
  * must match the uncached donor coefficients exactly. */
 for(unsigned slot=0;slot<4;slot++)for(int cut=0;cut<128;cut++)for(int res=0;res<128;res++){
  tsvf_t expected,actual;
  tsvf_coef(&expected,cut<<8,res);
  for(unsigned repeat=0;repeat<2;repeat++){
   bridge_filter_coefficients(&actual,slot,cut<<8,res);
   assert(memcmp(&expected,&actual,sizeof expected)==0);
  }
  bridge_filter_coefficients(&actual,slot,(127-cut)<<8,127-res);
  bridge_filter_coefficients(&actual,slot,cut<<8,res);
  assert(memcmp(&expected,&actual,sizeof expected)==0);
 }
 /* Compare every source/window against the original modulo/int64 formula,
  * including wrap endpoints and negative interpolation truncation. */
 uint32_t random=0x12345678u;
 for(unsigned source=0;source<4;source++)for(unsigned start=0;start<animate_poc_lengths[source];start++)
 for(unsigned end=start+1;end<=animate_poc_lengths[source];end++)for(unsigned k=0;k<8;k++){
  unsigned len=end-start;random=random*1664525u+1013904223u;
  uint32_t phase=k==0?0:k==1?UINT32_MAX:random;
  unsigned offset=k&1?len-1:0;uint64_t pos=(uint64_t)phase*len;
  unsigned i=((uint32_t)(pos>>32)+offset)%len,frac=(uint32_t)pos>>16;
  const int32_t*table=animate_poc_tables[source]+start;
  int32_t a=table[i]/65536,b=table[i+1==len?0:i+1]/65536;
  int32_t expected=a+(int32_t)(((int64_t)(b-a)*frac)/65536);
  assert(animate_lookup(source,phase,start,end,offset)==expected);
 }
 ab_init();assert(ab_polyphony(16));
 /* An edit restored exactly to the native value must not permanently opt
  * every voice into extended rendering. Exercise every slot/control. */
 for(unsigned s=0;s<4;s++)for(unsigned p=0;p<20;p++){
  float original=ab_slot_parameter_get(s,p);
  if(!bridge_value_valid(p,original))continue;
  float alternate=original==0?1:0;
  if(!bridge_value_valid(p,alternate))alternate=.001f;
  assert(ab_slot_parameter(s,p,alternate));
  assert(ab_slot_parameter(s,p,original));
  assert(!(ab_slot_parameter_mask(s)&(1u<<p)));
 }
 uint32_t h[16];for(unsigned i=0;i<16;i++)h[i]=ab_owned_note_on(48+i,100);
 assert(ab_active()==16);blocks(20);assert(ab_active()==16);
 /* A released voice wins over every held voice, even when newer. */
 ab_note_off(h[15]);ab_shed_oldest();assert(trk[0].v[h[15]&15].stage==4);
 assert(ab_active()==16);blocks(1);assert(ab_active()==15);
 for(unsigned i=0;i<15;i++)assert(bridge_handle_valid(h[i]));
 /* Preserve the lowest held note, retire each other voice exactly once. */
 unsigned before=ab_shed_count();
 for(unsigned i=0;i<14;i++){ab_shed_oldest();blocks(1);}
 assert(ab_active()==1&&bridge_handle_valid(h[0]));
 assert(ab_shed_count()==before+14);ab_shed_oldest();assert(ab_shed_count()==before+14);
 /* Replacement uses normal shared allocation, retaining independent same-key ownership. */
 uint32_t a=ab_owned_note_on(72,90),b=ab_owned_note_on(72,90);
 assert(a&&b&&a!=b);ab_note_off(a);assert(trk[0].v[b&15].gate);
 ab_panic();assert(ab_active()==0);
 puts("PASS shared Melodee host: 16 voices, release-first block fades, bass protection, independent ownership");
}
