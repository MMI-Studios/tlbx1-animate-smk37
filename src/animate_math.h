/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ANIMATE_MATH_H
#define ANIMATE_MATH_H
/* AV_MATH_REFERENCE selects native libm for comparison builds only.
 * Production: no libm, allocation, runtime table generation or mutable state.
 * Keep exp/pow2 at parameter/rate boundaries; sin/tanh are hot-path helpers. */
#ifdef AF_HARDWARE
#define AV_MATH_HOT __attribute__((section(".dsp_text")))
#else
#define AV_MATH_HOT
#endif
AV_MATH_HOT float av_sin(float radians);
AV_MATH_HOT float av_exp(float exponent);
AV_MATH_HOT float av_pow2(float exponent);
AV_MATH_HOT float av_tanh(float input);
float av_note_hz(float midi_note,float a4_hz);
#endif
