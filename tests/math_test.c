#include "animate_math.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>
static float maximum(float a,float b){return a>b?a:b;}
int main(void){
 float sine=0,wide=0,tanh_error=0,power=0,exponent=0,previous=-2;
 for(unsigned i=0;i<=200000;i++){
  float x=6.28318530718f*i/200000;sine=maximum(sine,fabsf(av_sin(x)-sinf(x)));
  x=-1024+2048.f*i/200000;wide=maximum(wide,fabsf(av_sin(x)-sinf(x)));
  x=-10+20.f*i/200000;float y=av_tanh(x);tanh_error=maximum(tanh_error,fabsf(y-tanhf(x)));assert(y>=previous);previous=y;
  x=-11+25.f*i/200000;float ref=exp2f(x);power=maximum(power,fabsf(av_pow2(x)-ref)/ref);
  x=-80+160.f*i/200000;ref=expf(x);exponent=maximum(exponent,fabsf(av_exp(x)-ref)/ref);
 }
 printf("math maximum errors: sine phase %.9g, sine +/-1024 %.9g, tanh absolute %.9g, pow2 relative %.9g, exp relative %.9g\n",sine,wide,tanh_error,power,exponent);
 assert(sine<.000006f&&wide<.0001f&&tanh_error<.0001f&&power<.000002f&&exponent<.00002f);
 assert(av_note_hz(69,440)==440);assert(fabsf(av_note_hz(81,440)-880)<.001f);
#ifndef AV_MATH_REFERENCE
 assert(av_pow2(-127)==0&&isfinite(av_pow2(128))&&av_pow2(128)==FLT_MAX);
 assert(av_sin(INFINITY)==0&&isnan(av_sin(NAN))&&av_tanh(INFINITY)==1&&av_tanh(-INFINITY)==-1);
#endif
 return 0;
}
