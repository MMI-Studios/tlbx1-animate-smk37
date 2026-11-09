/* SPDX-License-Identifier: GPL-3.0-only
 * TLBX-1 boot splash: the v2 app icon (assets/branding/icon-concept_v2_b.svg)
 * rendered procedurally with anti-aliased edges, outer glow and a glowing orb,
 * plus the TLBX-1 / ANIMATE wordmark and a startup bar. One pure rasterizer
 * feeds both the SMK LCD (main loop only, never audio IRQs) and the browser
 * simulator, so both show identical pixels. Output is native RGB565. */
#ifndef AF_SPLASH_H
#define AF_SPLASH_H
#include <stdint.h>
#define AF_SPLASH_FRAMES 40u    /* 40 frames at the 40 ms UI cadence: 1.6 s */
#define AF_SPLASH_FRAME_MS 40u
/* Render pixels x0..x0+w-1 of row y for frame (0..AF_SPLASH_FRAMES-1). */
void af_splash_row(unsigned frame,unsigned y,unsigned x0,unsigned w,uint16_t*out);
/* Rectangles {x,y,w,h} covering every pixel that may differ from frame-1;
 * frame 0 is the full screen. Returns the count (0 when nothing changes). */
#define AF_SPLASH_RECTS 6u
unsigned af_splash_regions(unsigned frame,unsigned rects[AF_SPLASH_RECTS][4]);
/* Pad-light sweep for the same frame: bit n lights pad n+1. */
uint16_t af_splash_pads(unsigned frame);
/* Simulator helper: whole frame into an internal 240x240 buffer. */
const uint16_t*af_splash_render(unsigned frame);
#endif
