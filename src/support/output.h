/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ANIMATE_OUTPUT_H
#define ANIMATE_OUTPUT_H
#include <stdint.h>
typedef struct {int32_t dc[2],error[2],peak;} animate_output;
void animate_output_init(animate_output *state);
/* Input is a summed Q15 bus, with up to 32x headroom. Output is Q15.
 * Fixed 44.1 kHz detector/DC timing. No low-cut or speaker bass enhancer. */
void animate_output_process(animate_output *state,int32_t *left,int32_t *right);
int32_t animate_output_from_float(float sample);
#endif
