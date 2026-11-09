/* Compare cached and uncached rendering across held voices and live edits. */
#include <assert.h>
#include <stdio.h>
#include "../src/baseline/bridge.c"

int main(void)
{
    static const float settings[20]={2,-7,23,.73f,-.31f,.43f,4,.62f,1,3,
                                    .12f,.23f,.88f,0,.27f,.48f,.013f,.29f,.61f,.18f};
    uint32_t handles[16];int32_t audio[64];
    ab_init();assert(ab_polyphony(16));
    for(unsigned i=0;i<16;i++)handles[i]=ab_owned_note_on(40+i,80+i*2);
    assert(ab_active()==16);
    for(unsigned stage=0;stage<30;stage++){
        if(stage<20)for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter(slot,stage,settings[stage]));
        if(stage==20){
            assert(ab_vector(.23f,.78f));
            for(unsigned axis=0;axis<2;axis++){
                assert(ab_vector_lfo(axis,1,2.75f));assert(ab_vector_lfo(axis,2,.3f));
                assert(ab_vector_lfo(axis,3,1));assert(ab_vector_lfo(axis,4,axis+2));
            }
        }
        if(stage==21||stage==23||stage==25)assert(ab_tempo(stage==21?73.f:stage==23?191.f:120.f));
        if(stage==22)for(unsigned i=0;i<16;i++)for(unsigned slot=0;slot<4;slot++){
            /* More unique lock combinations than cache ways exercises eviction. */
            assert(ab_voice_parameter(handles[i],slot,1,(float)(i%9)-4.f));
            assert(ab_voice_parameter(handles[i],slot,14,.1f+.015f*i));
            assert(ab_voice_parameter(handles[i],slot,19,.1f+.02f*i));
        }
        if(stage==24)for(unsigned i=0;i<16;i++)ab_voice_parameters_clear(handles[i]);
        if(stage==26)for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter(slot,13,4));
        if(stage==27)for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter_mask_set(slot,0));
        if(stage==28)for(unsigned slot=0;slot<4;slot++)assert(ab_slot_parameter(slot,13,2));
        if(stage==29)for(unsigned i=0;i<16;i++)ab_note_off(handles[i]);
        uint32_t hash=2166136261u;
        for(unsigned block=0;block<64;block++){
            assert(ab_render(audio,32));
            for(unsigned i=0;i<64;i++){hash^=(uint32_t)audio[i];hash*=16777619u;}
        }
        printf("%u %u %u\n",stage,hash,ab_active());
    }
    return 0;
}
