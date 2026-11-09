#include "animate_parameters.h"
#include <assert.h>
#include <string.h>
int main(void){
 unsigned mask=0;
 assert(ANIMATE_SLOT_PARAMETER_COUNT==20);
 for(unsigned i=0;i<ANIMATE_SLOT_PARAMETER_COUNT;i++){
  const animate_parameter_metadata*p=animate_parameter_info(i);
  assert(p&&p->id==(animate_parameter_id)i&&p->key[0]&&p->label[0]&&p->unit[0]);
  assert(p->random_mask_bit<19);mask|=1u<<p->random_mask_bit;
  for(unsigned j=0;j<i;j++)assert(strcmp(p->key,animate_parameter_info(j)->key));
 }
 assert(mask==((1u<<19)-1));
 assert(!animate_parameter_info(20));
 assert(animate_parameter_info(ANIMATE_LOOP_START)->random_mask_bit==animate_parameter_info(ANIMATE_LOOP_END)->random_mask_bit);
 assert(animate_parameter_info(ANIMATE_COARSE)->flags&ANIMATE_PARAMETER_RANGE_DISAGREEMENT);
 assert(animate_parameter_info(ANIMATE_COARSE)->ui_max==24&&animate_parameter_info(ANIMATE_COARSE)->stored_max==48);
 assert(animate_parameter_info(ANIMATE_DECAY)->stored_max==10);
 assert(animate_parameter_info(ANIMATE_WT_RATE)->flags&ANIMATE_PARAMETER_BEHAVIOR_DISAGREEMENT);
 assert(animate_lfo_division_beats[0]==.25f&&animate_lfo_division_beats[7]==16);
 return 0;
}
