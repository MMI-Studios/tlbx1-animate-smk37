#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "runtime.h"
#include "baseline/bridge.h"
#include "patch.h"
#include "project_state.h"
static unsigned char stored[1024];
static void button(unsigned b){af_button(b,1,0);af_button(b,0,1);}
static void pad(unsigned p){af_pad(p,1);af_pad(p,0);}
int main(void){
 af_init();assert(!af_patch_dirty());assert(af_control_get(0)==AC_VIEW_HOME);
 button(AC_PATCH);assert(af_control_get(0)==AC_VIEW_PATCHES);pad(0);assert(af_control_get(0)==AC_VIEW_CONFIRM&&af_patch_confirm_kind()==AF_PATCH_LOAD);button(AC_LEFT);assert(af_control_get(0)==AC_VIEW_PATCHES&&!af_patch_confirm_kind());pad(1);assert(af_control_get(0)==AC_VIEW_HOME);
 button(AC_PATCH);pad(0);af_button(AC_SHIFT,1,0);button(AC_GLOBE);af_button(AC_SHIFT,0,2);assert(af_control_get(0)==AC_VIEW_HOME&&!af_patch_confirm_kind());
 af_control_set(0,AC_VIEW_GLOBAL);af_control_set(7,2);af_encoder(4,1);assert(af_control_get(0)==AC_VIEW_CONFIRM&&af_patch_selection()==1&&af_get(0,0,AC_D_GLOBAL,20)==0);button(AC_LEFT);assert(af_get(0,0,AC_D_GLOBAL,20)==0);
 unsigned revision=af_patch_revision();af_control_set(0,AC_VIEW_PARAM);af_control_set(2,2);assert(af_patch_revision()==revision);
 af_set(0,0,AC_D_STEP_FIELD,127*4,71);assert(!af_patch_dirty());
 for(unsigned s=0;s<4;s++){assert(af_set(0,s,AC_D_PARAM,ANIMATE_LEVEL,.23f+s*.17f));assert(af_set(0,s,AC_D_PARAM,ANIMATE_PAN,-.6f+s*.3f));assert(af_set(0,s,AC_D_PARAM,ANIMATE_ATTACK,.12f+s*.31f));assert(af_set(0,s,AC_D_PARAM,ANIMATE_WT_AMOUNT,.13f+s*.11f));}
 assert(af_set(0,0,AC_D_MOD,0,.23f));assert(af_set(0,0,AC_D_MOD,8,.79f));assert(af_set(0,0,AC_D_MOD,1,4));assert(af_set(0,0,AC_D_MOD,2,7.32f));assert(af_set(0,0,AC_D_MOD,3,.67f));assert(af_set(0,0,AC_D_MOD,4,1));assert(af_set(0,0,AC_D_MOD,5,6));
 assert(af_set(0,0,AC_D_FX,0,37));assert(af_set(0,0,AC_D_FX,2,51));assert(af_set(0,0,AC_D_GLOBAL,17,1));assert(af_set(0,0,AC_D_GLOBAL,25,3));assert(af_set(0,0,AC_D_GLOBAL,32,2));assert(af_set(0,0,AC_D_GLOBAL,8,137));assert(af_patch_dirty());
 float base[4][20];memcpy(base,af_project_state()->base,sizeof base);uint32_t masks[4];for(unsigned s=0;s<4;s++)masks[s]=ab_slot_parameter_mask(s);
 assert(af_patch_save(3));assert(af_patch_confirm_kind()==AF_PATCH_SAVE);assert(!af_patch_pending());af_patch_cancel();assert(af_patch_dirty());
 assert(af_patch_save(3));assert(af_patch_confirm());unsigned id=af_patch_request_id(),size=af_patch_bytes();assert(size<1024&&af_patch_pending()==AF_PATCH_SAVE);memcpy(stored,af_patch_buffer(),size);assert(af_patch_validate(stored,size));assert(!af_patch_complete(id+1,1,size));assert(af_patch_complete(id,0,0));assert(af_patch_status()==AF_PATCH_ERROR&&af_patch_dirty());
 assert(af_patch_save(3));assert(af_patch_confirm());id=af_patch_request_id();assert(af_patch_complete(id,1,size));assert(!af_patch_dirty());assert(af_patch_current()==25&&af_patch_occupied());
 assert(af_patch_save(3));assert(af_patch_confirm());id=af_patch_request_id();af_set(0,0,AC_D_PARAM,ANIMATE_PAN,.9f);assert(af_patch_complete(id,1,size));assert(af_patch_dirty());
 assert(af_set(0,0,AC_D_PLAYING,0,1));unsigned generation=af_project_state()->sequence.generation[0];
 assert(af_patch_load(25));assert(af_patch_confirm());id=af_patch_request_id();memcpy(af_patch_buffer(),stored,size);af_patch_buffer()[size-1]^=1;assert(af_patch_complete(id,1,size));assert(af_patch_status()==AF_PATCH_ERROR);assert(af_project_state()->base[0][ANIMATE_PAN]==.9f&&af_project_state()->sequence.playing);
 assert(af_patch_load(25));assert(af_patch_confirm());id=af_patch_request_id();memcpy(af_patch_buffer(),stored,size);assert(af_patch_complete(id,1,size));assert(!af_patch_dirty());assert(af_patch_current()==25);assert(!memcmp(base,af_project_state()->base,sizeof base));for(unsigned s=0;s<4;s++)assert(masks[s]==ab_slot_parameter_mask(s));
 assert(af_project_state()->sequence.playing&&af_project_state()->sequence.steps[0][127].note==71&&af_project_state()->sequence.generation[0]==generation);
 assert(af_get(0,0,AC_D_GLOBAL,17)==1&&af_get(0,0,AC_D_GLOBAL,8)==137);assert(af_get(0,0,AC_D_FX,2)==51);assert(af_get(0,0,AC_D_MOD,2)==7.32f&&af_get(0,0,AC_D_MOD,4)==1);assert(af_base_vector(1)==.79f);
 revision=af_patch_revision();af_patch_select(1);assert(af_patch_revision()==revision&&af_patch_current()==25);assert(af_patch_load(1));af_patch_cancel();assert(af_patch_current()==25);assert(af_patch_load(1));assert(af_patch_confirm());assert(af_patch_current()==1&&af_patch_dirty()&&!af_patch_pending());assert(af_get(0,0,AC_D_GLOBAL,8)==137);
 assert(af_set(0,0,AC_D_GLOBAL,19,1));assert(af_patch_save(0));assert(af_patch_confirm());assert(af_patch_complete(af_patch_request_id(),1,af_patch_bytes()));assert(!af_patch_dirty());af_set(0,0,AC_D_MOD,0,.71f);assert(af_patch_dirty());
 {/* Factory sounds with sequences: wave registers, whole sequence replaced, clean, transport kept. */
  af_init();assert(af_set(0,0,AC_D_PLAYING,0,1));
  for(unsigned k=AF_PATCH_ORIGINALS;k<AF_PATCH_FACTORY;k++){
   assert(af_set(3,0,AC_D_STEP_FIELD,100*4,40));
   assert(af_patch_load(k));assert(!strcmp(af_value(0,0,AC_D_STATUS,3),"LOAD SOUND + SEQ"));assert(af_patch_confirm());
   assert(af_patch_current()==k&&!af_patch_dirty()&&af_patch_status()==AF_PATCH_LOADED&&af_project_state()->sequence.playing);
   assert(!strcmp(af_value(0,0,AC_D_STATUS,6),af_patch_factory_name(k))&&strlen(af_patch_factory_name(k))>0);
   unsigned on=0;for(unsigned l=0;l<10;l++)for(unsigned st=0;st<128;st++)on+=af_project_state()->sequence.steps[l][st].enabled;assert(on>0);
   unsigned changed=0;for(unsigned r=0;r<4;r++)changed+=ab_wave_get(r)>=4;assert(changed>0);
   int32_t pcm[64];uint64_t energy=0;for(unsigned b=0;b<3000;b++){af_render_q15(pcm,32);for(unsigned i=0;i<64;i++)energy+=(int64_t)pcm[i]*pcm[i];}assert(energy>0);
  }
  /* Loading an original sound restores the original waves; user slots start at 22. */
  assert(af_patch_load(0)&&af_patch_confirm());for(unsigned r=0;r<4;r++)assert(ab_wave_get(r)==r);
  assert(!strcmp(af_patch_factory_name(6),"WAVE STATION"));af_patch_select(AF_PATCH_FACTORY);assert(!strcmp(af_value(0,0,AC_D_STATUS,1),"USER 01"));
  assert(!af_patch_select(AF_PATCH_ENTRIES));
 }
 puts("PASS compact patch: all-slot sound/mod/FX/music, live transport, dirty revisions, confirmation, durable completion and corruption atomicity; sixteen factory sounds with sequences");return 0;
}
