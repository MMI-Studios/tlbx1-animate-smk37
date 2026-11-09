/* SPDX-License-Identifier: GPL-3.0-only */
/* One outstanding block. CPU0 owns runtime before submit and after join;
 * CPU1 owns the entire runtime during render. No per-voice shared-cache race.
 * Platform must stop a failed worker before join returns false. */
#pragma once
#include <stdint.h>
typedef struct { int32_t *out; unsigned frames; } af_render_job;
static void af_render_job_run(void *context)
{
    af_render_job *job=context;
    af_render_q15(job->out,job->frames);
}
#ifndef AF_DISPATCH_ATTRIBUTE
#define AF_DISPATCH_ATTRIBUTE
#endif
static AF_DISPATCH_ATTRIBUTE int af_dispatch_render(int32_t *out,unsigned frames)
{
    af_render_job job={out,frames};
    if(!AF_RENDER_SUBMIT(af_render_job_run,&job)){
        af_render_job_run(&job);
        return 0; /* Worker unavailable: render once on CPU0. */
    }
    if(AF_RENDER_JOIN())return 1;
    /* Never rerender a partly advanced block or expose its partial output. */
    for(unsigned i=0;i<frames*2u;i++)out[i]=0;
    af_release();
    return -1;
}
