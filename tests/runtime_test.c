#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "baseline/bridge.h"
#include "patch.h"
static void render(unsigned blocks){int32_t pcm[64];while(blocks--)af_render_q15(pcm,32);}
static uint32_t wave_hash(void){af_key(60,100);uint32_t h=2166136261u;int32_t pcm[64];for(unsigned b=0;b<200;b++){af_render_q15(pcm,32);for(unsigned i=0;i<64;i++)h=(h^(uint32_t)pcm[i])*16777619u;}af_key(60,0);render(2000);return h;}
static int af_dirty(void){return af_get(0,0,AC_D_STATUS,0)!=0;}
static void press(unsigned b){af_button(b,1,0);af_button(b,0,1);}
static void random_apply(void){af_button(AC_SHIFT,1,0);press(AC_ARP);af_button(AC_SHIFT,0,2);}
static void roundtrip(void){unsigned size=af_project_size();assert(size);unsigned char*bytes=malloc(size);assert(bytes);for(unsigned at=0;at<size;){unsigned n=af_project_read(at,1024);assert(n);memcpy(bytes+at,af_project_buffer(),n);at+=n;}assert(af_project_begin(size));for(unsigned at=0;at<size;){unsigned n=size-at;if(n>1024)n=1024;memcpy(af_project_buffer(),bytes+at,n);assert(af_project_write(at,n));at+=n;}assert(af_project_commit());free(bytes);}
static void audio_fixture(unsigned preset,int reference,int knob,int session){
 if(reference){ab_init();ab_preset(preset);}else{af_init();assert(af_set(0,0,AC_D_GLOBAL,20,preset));}
 if(knob>=0){af_button(AC_SHIFT,1,0);af_pad(3,1);af_pad(3,0);af_button(AC_SHIFT,0,1);assert(af_control_get(0)==AC_VIEW_MIXER);af_encoder((unsigned)knob,-30);}
 if(session==1)roundtrip();
 if(session==2){unsigned char patch[1024];unsigned n=af_patch_export(patch,sizeof patch);assert(n&&af_patch_import(patch,n));}
 uint32_t hash=2166136261u;uint64_t energy=0;int32_t pcm[64];
 for(unsigned block=0;block<4630;block++){
  if(block==30){if(reference)ab_note_on(60,110);else af_key(60,110);}
  if(block==430){if(reference){ab_note_on(64,100);ab_note_on(67,90);}else{af_key(64,100);af_key(67,90);}}
  if(block==630){if(reference)ab_release_all();else{af_key(60,0);af_key(64,0);af_key(67,0);}}
  if(reference)ab_render(pcm,32);else af_render_q15(pcm,32);
  for(unsigned i=0;i<64;i++){hash=(hash^(uint32_t)pcm[i])*16777619u;energy+=(int64_t)pcm[i]*pcm[i];}
 }
 printf("%u %u %llu\n",preset,hash,(unsigned long long)energy);
}
int main(int argc,char**argv){
 af_hardware_metrics(0,0);af_hardware_worker(1,123,0);
 assert(af_get(0,0,AC_D_GLOBAL,21)==1&&af_get(0,0,AC_D_GLOBAL,22)==123);
 assert(!af_set(0,0,AC_D_GLOBAL,21,0));
 af_init();press(AC_GLOBE);press(AC_LEFT);
 assert(af_control_get(7)==5);assert(af_control_set(7,5));
 assert(af_get(0,0,AC_D_GLOBAL,48)==-1);
 uint32_t capture[8]={3000,2000,250,500,250,4,2466,1};
 af_hardware_metrics(60,0);af_hardware_overload(capture);
 assert(af_get(0,0,AC_D_GLOBAL,53)==4);
 assert(!strcmp(af_label(0,0,AC_D_GLOBAL,50),"SCAN US"));
 af_encoder(0,1);assert(af_get(0,0,AC_D_GLOBAL,48)==3000);
 assert(!af_set(0,0,AC_D_GLOBAL,48,0));
 af_draw();press(AC_RIGHT);assert(af_control_get(7)==0);
 if(argc>1){audio_fixture((unsigned)atoi(argv[1]),argc>2&&argv[2][0]=='r',argc>2&&argv[2][0]=='k'?atoi(argv[2]+1):-1,argc>2?(argv[2][0]=='p'?2:argv[2][0]=='s'):0);return 0;}
 af_init();assert(af_polyphony()==8&&af_get(0,0,AC_D_GLOBAL,1)==8);{unsigned c=af_polyphony_changes();assert(af_polyphony_restore(12)&&af_polyphony()==12&&af_polyphony_changes()==c);assert(!af_polyphony_restore(0)&&!af_polyphony_restore(17));assert(af_set(0,0,AC_D_GLOBAL,1,5)&&af_polyphony_changes()==c+1);assert(af_set(0,0,AC_D_GLOBAL,1,5)&&af_polyphony_changes()==c+1);}
 af_init();aps_state*s=af_project_state();assert(s->sequence.rate==44100);assert(s->sequence.length==128);
 assert(af_get(0,0,AC_D_GLOBAL,13)==-1);
 af_hardware_metrics(60,200);af_hardware_profile(1200,400,100);
 for(unsigned i=0;i<3;i++){assert(af_get(0,0,AC_D_GLOBAL,13+i)==(i==0?1200:i==1?400:100));assert(!af_set(0,0,AC_D_GLOBAL,13+i,0));}
 assert(!strcmp(af_label(0,0,AC_D_GLOBAL,13),"RENDER US"));
 af_key(60,100);af_key(64,100);af_key(67,100);assert(af_get(0,0,AC_D_GLOBAL,11)==3);assert(ab_active()==3);
 /* The shared host fades the victim over one block, never hard-cuts it. */
 render(20);af_shed();assert(ab_active()==3&&af_get(0,0,AC_D_GLOBAL,12)==1);
 render(2);assert(ab_active()==2);assert(af_get(0,0,AC_D_GLOBAL,11)==3);
 af_release();ab_panic();af_shed();assert(af_get(0,0,AC_D_GLOBAL,12)==1&&af_get(0,0,AC_D_GLOBAL,11)==0);
 for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter_mask(slot)==0);
 /* Equal startup sound settings: only the four waveform choices differ. */
 for(unsigned slot=0;slot<4;slot++){assert(s->base[slot][ANIMATE_SOURCE]==(float)slot);assert(s->base[slot][ANIMATE_LEVEL]==1);for(unsigned p=1;p<20;p++)assert(s->base[slot][p]==s->base[0][p]);}
 float initial[4][20];memcpy(initial,s->base,sizeof initial);float x=ab_vector_get(0),y=ab_vector_get(1);
 press(AC_PARA);
 for(unsigned slot=0;slot<4;slot++){af_pad(slot,1);af_pad(slot,0);assert(af_control_get(2)==slot);af_pad(4,1);af_pad(4,0);assert(af_control_get(0)==AC_VIEW_MOD&&af_control_get(8)==0);af_pad(5,1);af_pad(5,0);assert(af_control_get(8)==1);af_pad(slot,1);af_pad(slot,0);assert(af_control_get(0)==AC_VIEW_PARAM&&af_control_get(2)==slot);}
 af_button(AC_SHIFT,1,0);press(AC_PARA);af_button(AC_SHIFT,0,2);assert(af_control_get(0)==AC_VIEW_MOD&&af_control_get(8)==0);
 assert(!memcmp(initial,s->base,sizeof initial));assert(ab_vector_get(0)==x&&ab_vector_get(1)==y);for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter_mask(slot)==0);
 af_button(AC_SHIFT,1,0);af_pad(3,1);af_pad(3,0);af_button(AC_SHIFT,0,1);assert(af_control_get(0)==AC_VIEW_MIXER);
 af_control_set(1,9);af_control_set(2,3);af_control_set(15,2);
 for(unsigned knob=0;knob<8;knob++){
  float before[4][20];memcpy(before,s->base,sizeof before);af_encoder(knob,-1);
  for(unsigned slot=0;slot<4;slot++)for(unsigned p=0;p<20;p++)if(slot==knob/2&&p==(knob%2?ANIMATE_PAN:ANIMATE_LEVEL))assert(s->base[slot][p]<before[slot][p]);else assert(s->base[slot][p]==before[slot][p]);
 }
 assert(af_set(0,0,AC_D_GLOBAL,16,1));af_key(60,100);render(20);af_key(60,0);render(20);assert(ab_active());
 assert(af_set(0,0,AC_D_PLAYING,0,0));assert(af_get(0,0,AC_D_HOLD,0)==1);render(4000);assert(!ab_active());
 af_set(0,0,AC_D_MOD,0,.2f);af_set(0,0,AC_D_MOD,8,.7f);af_set(0,0,AC_D_GLOBAL,19,1);af_wheel(0,8191);af_wheel(1,0);assert(ab_vector_get(0)==1&&ab_vector_get(1)==0);af_set(0,0,AC_D_GLOBAL,19,0);assert(ab_vector_get(0)==.2f&&ab_vector_get(1)==.7f);
 af_set(0,0,AC_D_ROUTE,1,APS_ALL);af_set(0,0,AC_D_ROUTE,0,APS_BOTH);af_set(0,0,AC_D_ROUTE,8,1);af_set(0,0,AC_D_ROUTE,9,1);
 af_set(0,0,AC_D_PLAYING,0,1);random_apply();assert(af_get(0,0,AC_D_GLOBAL,42)==1280);render(1);assert(af_get(0,0,AC_D_GLOBAL,42)==1279);render(1279);assert(af_get(0,0,AC_D_GLOBAL,42)==0);assert(s->sequence.playing);float locked;assert(ap_lock_get(&s->locks[9][127][3],ANIMATE_PAN,&locked));
 af_set(0,0,AC_D_STEP_FIELD,127*4,74);assert(s->sequence.steps[0][127].note==74&&s->sequence.playing);
 unsigned size=af_project_size();assert(size>100000);unsigned char*saved=malloc(size);assert(saved);
 for(unsigned at=0;at<size;){unsigned n=af_project_read(at,1024);assert(n);memcpy(saved+at,af_project_buffer(),n);at+=n;}
 af_set(0,0,AC_D_PARAM,ANIMATE_PAN,.5f);assert(af_project_begin(size));saved[size-1]^=1;
 for(unsigned at=0;at<size;){unsigned n=size-at;if(n>1024)n=1024;memcpy(af_project_buffer(),saved+at,n);assert(af_project_write(at,n));at+=n;}
 assert(!af_project_commit());assert(s->sequence.playing&&s->base[0][ANIMATE_PAN]==.5f);saved[size-1]^=1;
 assert(af_project_begin(size));for(unsigned at=0;at<size;){unsigned n=size-at;if(n>1024)n=1024;memcpy(af_project_buffer(),saved+at,n);assert(af_project_write(at,n));at+=n;}assert(af_project_commit());assert(!s->sequence.playing);assert(s->sequence.steps[0][127].note==74);assert(ap_lock_get(&s->locks[9][127][3],ANIMATE_PAN,&locked));free(saved);
 assert(af_set(0,0,AC_D_GLOBAL,1,8));af_control_set(0,AC_VIEW_GLOBAL);af_control_set(7,0);af_encoder(1,1);assert(af_get(0,0,AC_D_GLOBAL,1)==9);af_encoder(1,7);assert(af_get(0,0,AC_D_GLOBAL,1)==16);af_encoder(1,5);assert(af_get(0,0,AC_D_GLOBAL,1)==16);af_encoder(1,-20);assert(af_get(0,0,AC_D_GLOBAL,1)==1);assert(!af_set(0,0,AC_D_GLOBAL,1,0));assert(!af_set(0,0,AC_D_GLOBAL,1,17));assert(af_set(0,0,AC_D_GLOBAL,1,16));
 {/* Wave bank: browsing gives the slot its own register; patches v1/v2 and sessions keep registers. */
  af_init();assert(af_get(0,0,AC_D_PARAM,21)==ab_wave_count()&&ab_wave_count()>4);
  for(unsigned r=0;r<4;r++)assert(ab_wave_get(r)==r&&af_get(0,r,AC_D_PARAM,20)==r);
  assert(!strcmp(af_label(0,0,AC_D_PARAM,20),"WAVE"));assert(!strcmp(af_value(0,0,AC_D_PARAM,20),"SINE C3"));
  uint32_t before=wave_hash();
  assert(af_set(0,0,AC_D_PARAM,20,100)&&ab_wave_get(0)==100&&af_get(0,0,AC_D_PARAM,20)==100&&af_dirty());
  assert(!strcmp(af_value(0,0,AC_D_PARAM,20),ab_wave_name(100))&&!strcmp(af_value(0,0,AC_D_PARAM,0),ab_wave_name(100)));
  assert(!af_set(0,0,AC_D_PARAM,20,(float)ab_wave_count())&&!af_set(0,0,AC_D_PARAM,20,-1)&&!af_set(0,0,AC_D_PARAM,20,1.5f));
  /* Slot B pointed at register A: browsing B reassigns it to register B and leaves A alone. */
  assert(af_set(0,1,AC_D_PARAM,0,0)&&af_get(0,1,AC_D_PARAM,20)==100);
  assert(af_set(0,1,AC_D_PARAM,20,7)&&af_get(0,1,AC_D_PARAM,0)==1&&ab_wave_get(1)==7&&ab_wave_get(0)==100);
  uint32_t after=wave_hash();assert(after!=before);
  unsigned char bytes[1024];unsigned n=af_patch_export(bytes,sizeof bytes);assert(n);
  af_set(0,0,AC_D_GLOBAL,20,0);for(unsigned r=0;r<4;r++)assert(ab_wave_get(r)==r);
  assert(af_patch_import(bytes,n)&&ab_wave_get(0)==100&&ab_wave_get(1)==7&&ab_wave_get(2)==2);
  /* A version-1 patch (no wave registers) loads with the original four waves. */
  unsigned v1=n-16;uint32_t w=1;memcpy(bytes+4,&w,4);memcpy(bytes+8,&v1,4);
  uint32_t h=2166136261u;for(unsigned i=0;i<v1;i++)h=(h^(i>=12&&i<16?0:bytes[i]))*16777619u;memcpy(bytes+12,&h,4);
  assert(af_patch_validate(bytes,v1)&&af_patch_import(bytes,v1));for(unsigned r=0;r<4;r++)assert(ab_wave_get(r)==r);
  /* Polyphony other than 8/16 survives patch and session validation. */
  assert(af_set(0,0,AC_D_GLOBAL,1,5));n=af_patch_export(bytes,sizeof bytes);assert(n&&af_patch_validate(bytes,n));
  assert(af_set(0,0,AC_D_PARAM,20,250)&&af_set(0,3,AC_D_PARAM,20,42));
  unsigned size=af_project_size();assert(size);unsigned char*file=malloc(size);for(unsigned at=0;at<size;){unsigned k=size-at>1024?1024:size-at;assert(af_project_read(at,k)==k);memcpy(file+at,af_project_buffer(),k);at+=k;}
  af_init();assert(ab_wave_get(0)==0);assert(af_project_begin(size));for(unsigned at=0;at<size;){unsigned k=size-at>1024?1024:size-at;memcpy(af_project_buffer(),file+at,k);assert(af_project_write(at,k));at+=k;}
  assert(af_project_commit());assert(ab_wave_get(0)==250&&ab_wave_get(3)==42&&af_get(0,0,AC_D_GLOBAL,1)==5);free(file);
 }
 puts("PASS runtime mixer, Hold, XY, bounded random, live edits, session validation and restore; wave bank registers, patch v1/v2 and session waves");return 0;
}
