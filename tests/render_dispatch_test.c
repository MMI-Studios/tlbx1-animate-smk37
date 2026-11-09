#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static unsigned calls,releases,mode;
static void (*pending)(void*);
static void *context;
static void af_render_q15(int32_t*out,unsigned frames){calls++;for(unsigned i=0;i<frames*2;i++)out[i]=(int32_t)(i+calls);}
static void af_release(void){releases++;}
static int submit(void(*fn)(void*),void*p){if(!mode)return 0;assert(!pending);pending=fn;context=p;return 1;}
static int join(void){assert(pending);pending(context);pending=0;return mode==1;}
#define AF_RENDER_SUBMIT(fn,ctx) submit(fn,ctx)
#define AF_RENDER_JOIN() join()
#include "../src/render_dispatch.h"
int main(void){
 int32_t out[64];
 assert(af_dispatch_render(out,32)==0&&calls==1&&!releases);
 mode=1;assert(af_dispatch_render(out,32)==1&&calls==2&&!pending&&!releases);
 for(unsigned i=0;i<64;i++)assert(out[i]==(int32_t)i+2);
 mode=2;assert(af_dispatch_render(out,32)==-1&&calls==3&&releases==1&&!pending);
 for(unsigned i=0;i<64;i++)assert(out[i]==0);
 mode=0;assert(af_dispatch_render(out,32)==0&&calls==4);
 puts("PASS block worker dispatch: serial, joined, partial failure, serial fallback");
}
