#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint32_t regs[0x60000/4];
#define SMK37_SCAN_REG(a) regs[(a)/4]
#define SMK37_SCAN_CACHED_INDICATORS 1
#include "../vendor/melodee/firmware/hal/smk37_scanner.h"
int main(void){
 /* Exhaust every possible combination, checking emitted words and commons. */
 for(uint32_t bits=0;bits<(1u<<19);++bits){
  smk37_scan_indicators_set(bits);
  for(unsigned bank=0;bank<2;bank++){
   uint32_t mask=UINT32_MAX;
   for(unsigned i=0;i<19;i++)if((bits&(1u<<i))&&(smk37_indicator_order[i]>>4)==bank)mask&=smk37_indicator_mask(i);
   assert(smk37_scan_indicator_masks[bank]==mask);
   smk37_indicator_phase=bank*2;smk37_scan_slot=7;smk37_scan_analog_mask=0xffffffe7u;
   smk37_scan_next();
   assert(smk37_scan_word==(smk37_scan_select[7]&0xffffffe7u&mask));
   assert(smk37_indicator_pending==(mask!=UINT32_MAX?bank+1:0));
  }
 }
 smk37_scan_indicators_set(0);assert(smk37_scan_indicator_masks[0]==UINT32_MAX&&smk37_scan_indicator_masks[1]==UINT32_MAX);
 puts("PASS: all 524288 indicator combinations preserve scanner words and common-bank selection");
}
