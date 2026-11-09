/* Per-voice parallel rendering through the donor mailbox on a concurrent host
 * thread (CPU1 stand-in). Threaded PCM must equal serial PCM exactly. */
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "runtime.h"
static uint32_t ticks(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (uint32_t)(t.tv_sec*1000000ull+t.tv_nsec/1000);}
#define AW_WORD _Atomic uint32_t
#define AW_LOAD(p) atomic_load_explicit(p,memory_order_acquire)
#define AW_STORE(p,v) atomic_store_explicit(p,v,memory_order_release)
#define AW_TICKS() ticks()
/* Host scheduler tolerance, not the target's timeout. */
#define AW_TIMEOUT_TICKS 10000000u
#define AW_RAM_LOOP static
#include "audio_worker.h"
static _Atomic unsigned stop;
static void *worker(void*unused){(void)unused;uint32_t done=0;while(!atomic_load(&stop))audio_worker_poll(&done);return 0;}
void ab_worker_hooks(int (*submit)(void (*)(void *), void *), int (*join)(void));
int main(int argc,char**argv){
 unsigned threaded=argc>1?atoi(argv[1]):0;pthread_t thread;
 af_init();if(threaded){audio_worker_online=1;ab_worker_hooks(audio_worker_submit,audio_worker_join);assert(!pthread_create(&thread,0,worker,0));}
 uint32_t hash=2166136261u;int32_t pcm[64];
 for(unsigned block=0;block<2400;block++){
  if(block%240==0)for(unsigned n=0;n<8;n++)af_key(48+n*3,100);
  if(block%240==160)af_release();
  if(block%80==0){unsigned slot=(block/80)%4;assert(af_set(0,slot,AC_D_PARAM,0,(block/320)%4));}
  if(block==400)assert(af_set(0,0,AC_D_PARAM,14,.4f));
  if(block==600)assert(af_set(0,1,AC_D_PARAM,4,.5f));
  if(block==800){af_button(AC_PLAY,1,0);af_button(AC_PLAY,0,1);}
  af_render_q15(pcm,32);
  for(unsigned i=0;i<64;i++)hash=(hash^(uint32_t)pcm[i])*16777619u;
 }
 if(threaded){assert(audio_worker.jobs>2000);fprintf(stderr,"parallel voice jobs %u\n",audio_worker.jobs);atomic_store(&stop,1);pthread_join(thread,0);}
 printf("%u\n",hash);
}
