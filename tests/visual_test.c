/* Hardware Q15 capture must reproduce the existing stereo analyzer. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../vendor/tlbx-display/visual.h"
void af_visual_reset(void);
void af_visual_capture(const int32_t *,unsigned);
void af_visual_prepare(unsigned);
static int32_t audio[32*2];
static int16_t points[256];
static uint16_t bins[32];
static void signal(unsigned offset){
 for(unsigned i=0;i<32;i++){
  unsigned n=offset+i;
  audio[i*2]=(int32_t)((n*117u)%60001u)-30000;
  audio[i*2+1]=(n%32u)<16u?18000:-18000;
 }
}
int main(void){
 tlbx_visual_reset();
 for(unsigned at=0;at<4096;at+=32){signal(at);for(unsigned i=0;i<32;i++)tlbx_visual_feed(audio[i*2]/32768.f,audio[i*2+1]/32768.f);}
 tlbx_visual_prepare(1);
 memcpy(points,tlbx_visual_data(),sizeof points);memcpy(bins,tlbx_visual_bins(),sizeof bins);
 af_visual_reset();
 for(unsigned at=0;at<4096;at+=32){signal(at);af_visual_capture(audio,32);}
 /* Capture alone must not run the display analyzer. */
 for(unsigned i=0;i<256;i++)assert(tlbx_visual_data()[i]==0);
 af_visual_prepare(0);
 assert(!memcmp(points,tlbx_visual_data(),sizeof points));
 /* Changing to spectrum with no new samples must still analyze that frame. */
 af_visual_prepare(1);
 assert(!memcmp(bins,tlbx_visual_bins(),sizeof bins));
 af_visual_prepare(2);
 assert(!memcmp(points,tlbx_visual_data(),sizeof points));
 af_visual_reset();af_visual_prepare(1);
 for(unsigned i=0;i<256;i++)assert(tlbx_visual_data()[i]==0);
 for(unsigned i=0;i<32;i++)assert(tlbx_visual_bins()[i]==0);
 return 0;
}
