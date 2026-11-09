#include <assert.h>
#include <stdio.h>
#include "../src/baseline/bridge.c"
int main(void){
 ab_init();assert(ab_polyphony(16));uint32_t handles[16];
 for(unsigned i=0;i<16;i++){handles[i]=ab_owned_note_on(60,100);assert(handles[i]);}
 assert(ab_active()==16);assert(!ab_polyphony(8));
 ab_note_off(handles[0]);assert(!trk[0].v[handles[0]&15].gate);
 for(unsigned i=1;i<16;i++)assert(trk[0].v[handles[i]&15].gate);
 uint32_t replacement=ab_owned_note_on(65,100);assert(replacement);
 ab_note_off(handles[0]);assert(trk[0].v[replacement&15].gate);
 for(unsigned i=1;i<16;i++)ab_note_off(handles[i]);ab_note_off(replacement);
 int32_t out[64];for(unsigned i=0;i<4000;i++)assert(ab_render(out,32));assert(!ab_active());
 assert(ab_polyphony(8));for(unsigned n=1;n<=16;n++)assert(ab_polyphony(n));assert(!ab_polyphony(0));assert(!ab_polyphony(17));assert(ab_polyphony(8));assert(!ab_slot(4,1,0));assert(!ab_param(P_COUNT,0));
 puts("PASS: sixteen owned voices; same-pitch isolation; stale handle; release; input bounds.");
}
