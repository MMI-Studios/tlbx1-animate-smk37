/* Shared real post-mix audio analysis only; no prior engine/runtime imported. */
#include "../vendor/tlbx-display/visual.c"

#ifdef AF_HARDWARE
/* Audio owns the packed Q15 ring; the main/display loop owns its snapshot and
 * the float analyzer above. One aligned word keeps each stereo pair together.
 * Capture never performs float conversion or FFT work in the audio interrupt. */
static volatile uint32_t af_visual_ring[VIS_N],af_visual_written;
static uint32_t af_visual_snapshot[VIS_N],af_visual_consumed;
static uint16_t af_visual_q15(int32_t v){return(uint16_t)(v>32767?32767:v< -32768?-32768:v);}
#endif

void af_visual_reset(void){
 tlbx_visual_reset();
#ifdef AF_HARDWARE
 for(unsigned i=0;i<VIS_N;i++)af_visual_ring[i]=0;
 af_visual_written=0;af_visual_consumed=~0u;
#endif
}

void af_visual_capture(const int32_t *stereo,unsigned frames){
#ifdef AF_HARDWARE
 uint32_t n=af_visual_written;
 for(unsigned i=0;i<frames;i++)af_visual_ring[(n+i)&(VIS_N-1)]=
   (uint32_t)af_visual_q15(stereo[i*2])|((uint32_t)af_visual_q15(stereo[i*2+1])<<16);
 VIS_BARRIER();af_visual_written=n+frames;
#else
 for(unsigned i=0;i<frames;i++)tlbx_visual_feed(stereo[i*2]/32768.f,stereo[i*2+1]/32768.f);
#endif
}

void af_visual_prepare(unsigned mode){
#ifdef AF_HARDWARE
 uint32_t n=af_visual_written;VIS_BARRIER();
 if(n!=af_visual_consumed){
  int stable=0;
  /* An audio preemption invalidates this copy. Never disable interrupts or
   * retry indefinitely for a display: retain the previous plot if both fail. */
  for(unsigned attempt=0;attempt<2;attempt++){
   n=af_visual_written;VIS_BARRIER();
   for(unsigned i=0;i<VIS_N;i++)af_visual_snapshot[i]=af_visual_ring[(n+i)&(VIS_N-1)];
   VIS_BARRIER();if(n==af_visual_written){stable=1;break;}
  }
  if(!stable)return;
  for(unsigned i=0;i<VIS_N;i++){
   uint32_t pair=af_visual_snapshot[i];
   tlbx_visual_feed((int16_t)(pair&65535u)/32768.f,(int16_t)(pair>>16)/32768.f);
  }
  af_visual_consumed=n;
 }
#endif
 tlbx_visual_prepare(mode);
}
