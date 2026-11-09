#include <stdint.h>
#include <assert.h>
#define HALF_FRAMES 128u
#define FS 44100u
#define FM1_TICKS_PER_US 24u
#include "audio_budget_monitor.h"
int main(void)
{
    /* No shed_voice implementation exists: the monitor must link without it. */
    smk37_audio_budget_end(0);
    assert(!smk37_shed_req);
    smk37_audio_budget_end(1000000);
    assert(!smk37_shed_req);
    smk37_audio_budget_end(1000000);
    assert(smk37_shed_req);
    for (unsigned i=0;i<10000;i++) {
        smk37_audio_budget_begin();
        assert(!smk37_shed_req);
        smk37_audio_budget_end(1000000);
        assert(smk37_shed_req);
    }
    smk37_audio_budget_begin();
    smk37_audio_budget_end(0);
    assert(!smk37_shed_req);
    return 0;
}
