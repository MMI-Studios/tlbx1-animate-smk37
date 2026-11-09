#ifndef TLBX_VISUAL_H
#define TLBX_VISUAL_H
#include <stdint.h>
void tlbx_visual_reset(void);
void tlbx_visual_feed(float left,float right);
void tlbx_visual_prepare(unsigned mode);
const int16_t *tlbx_visual_data(void);
const uint16_t *tlbx_visual_bins(void);
#endif
