/* SPDX-License-Identifier: GPL-3.0-only
 * Bounded post-output observation. FFT runs only when the display requests it.
 * No audio allocation, locking, or DSP analysis in the capture function. */
#include "visual.h"
#include <string.h>
#ifndef TLBX_STATE
#define TLBX_STATE
#endif
#define VIS_N 1024
#if defined(__GNUC__)
#define VIS_BARRIER() __asm__ __volatile__("" ::: "memory")
#else
#define VIS_BARRIER() ((void)0)
#endif
static struct {
 float ring[VIS_N*2],snapshot[VIS_N*2],real[VIS_N],imag[VIS_N];
 int16_t points[256];uint16_t bins[32];
 volatile uint32_t count;
 uint32_t analyzed;
} vis TLBX_STATE;
static float vis_abs(float x){return x<0?-x:x;}
static float vis_sin(float x){
 /* Angle radians, bounded by FFT/window callers; reduce to [-pi,pi]. */
 const float pi=3.14159265359f;
 while(x>pi)x-=2*pi;
 while(x< -pi)x+=2*pi;
 float y=1.27323954474f*x-.40528473457f*x*vis_abs(x);
 return .225f*(y*vis_abs(y)-y)+y;
}
static float vis_cos(float x){return vis_sin(x+1.57079632679f);}
static int16_t vis_sample(float x){if(x>1)x=1;if(x< -1)x=-1;return(int16_t)(x*32767);}
static uint16_t vis_db_level(float amplitude){
 if(amplitude<=.001f)return 0;
 if(amplitude>=1)return 32767;
 int exponent=0;while(amplitude<1){amplitude*=2;exponent--;}
 /* ln(m) = 2(z + z^3/3 + z^5/5), m in[1,2), z=(m-1)/(m+1). */
 float z=(amplitude-1)/(amplitude+1),z2=z*z;
 float log2m=2.88539008178f*z*(1+z2/3+z2*z2/5);
 float db=6.02059991328f*(exponent+log2m);
 return(uint16_t)((db+60)*(32767.0f/60));
}
void tlbx_visual_reset(void){memset(&vis,0,sizeof vis);vis.analyzed=~0u;}
void tlbx_visual_feed(float l,float r){
 uint32_t n=vis.count;unsigned p=(n&(VIS_N-1))*2;
 vis.ring[p]=l;vis.ring[p+1]=r;VIS_BARRIER();vis.count=n+1;
}
const int16_t *tlbx_visual_data(void){return vis.points;}
const uint16_t *tlbx_visual_bins(void){return vis.bins;}
void tlbx_visual_prepare(unsigned mode){
 uint32_t n=vis.count;VIS_BARRIER();
 /* The producer owns count. A bounded retry avoids publishing a torn copy;
  * retain the previous plot if audio preempts both copy attempts. */
 int stable=0;
 for(unsigned attempt=0;attempt<2;attempt++){
  n=vis.count;VIS_BARRIER();
  for(unsigned i=0;i<VIS_N;i++){unsigned p=((n+i)&(VIS_N-1))*2;vis.snapshot[i*2]=vis.ring[p];vis.snapshot[i*2+1]=vis.ring[p+1];}
  VIS_BARRIER();if(n==vis.count){stable=1;break;}
 }
 if(!stable)return;
 for(unsigned i=0;i<128;i++){unsigned p=(VIS_N-256+i*2)*2;vis.points[i*2]=vis_sample(vis.snapshot[p]);vis.points[i*2+1]=vis_sample(vis.snapshot[p+1]);}
 if(mode!=1||vis.analyzed==n)return;
 vis.analyzed=n;
 /* Hann window; iterative radix-two FFT, fixed 1024-sample mono spectrum.
  * Magnitudes use max + 3/8 min approximation, avoiding libm on the MCU. */
 for(unsigned i=0;i<VIS_N;i++){vis.real[i]=(vis.snapshot[i*2]+vis.snapshot[i*2+1])*.25f*(1-vis_cos(6.28318530718f*i/(VIS_N-1)));vis.imag[i]=0;}
 for(unsigned i=1,j=0;i<VIS_N;i++){
  unsigned bit=VIS_N>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;
  if(i<j){float x=vis.real[i];vis.real[i]=vis.real[j];vis.real[j]=x;}
 }
 for(unsigned len=2;len<=VIS_N;len<<=1){
  float wr=vis_cos(-6.28318530718f/len),wi=vis_sin(-6.28318530718f/len);
  for(unsigned start=0;start<VIS_N;start+=len){float ar=1,ai=0;
   for(unsigned j=0;j<len/2;j++){
    unsigned a=start+j,b=a+len/2;
    float br=vis.real[b]*ar-vis.imag[b]*ai,bi=vis.real[b]*ai+vis.imag[b]*ar;
    float rr=vis.real[a],ri=vis.imag[a];vis.real[a]=rr+br;vis.imag[a]=ri+bi;vis.real[b]=rr-br;vis.imag[b]=ri-bi;
    float next=ar*wr-ai*wi;ai=ar*wi+ai*wr;ar=next;
   }
  }
 }
 /* Quasi-logarithmic bands over bins1..511; DC is deliberately excluded. */
 static const uint16_t edges[33]={1,2,3,4,5,6,7,8,9,10,12,14,17,20,24,28,34,40,48,57,68,81,96,114,136,161,192,228,271,323,384,456,512};
 for(unsigned b=0;b<32;b++){float peak=0;
  for(unsigned i=edges[b];i<edges[b+1];i++){float a=vis_abs(vis.real[i]),c=vis_abs(vis.imag[i]);float m=a>c?a+c*.375f:c+a*.375f;if(m>peak)peak=m;}
  float amplitude=peak*(4.0f/VIS_N);if(amplitude>1)amplitude=1;
  vis.bins[b]=vis_db_level(amplitude);
 }
}
