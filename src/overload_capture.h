#ifndef AF_OVERLOAD_CAPTURE_H
#define AF_OVERLOAD_CAPTURE_H
#include <stdint.h>
/* First triggering buffer, retained until reboot. No change to guard policy. */
typedef struct { uint32_t value[8]; unsigned valid; } af_overload_capture;
static inline void af_overload_record(af_overload_capture *c,unsigned triggered,
 uint32_t total,uint32_t render,uint32_t scan,uint32_t usb,uint32_t voices,
 uint32_t threshold,uint32_t ticks_per_us,uint32_t scan_irqs)
{
 if(c->valid||!triggered)return;
 uint32_t measured=render+scan+usb;
 c->value[0]=total/ticks_per_us;c->value[1]=render/ticks_per_us;
 c->value[2]=scan/ticks_per_us;c->value[3]=usb/ticks_per_us;
 c->value[4]=(total>measured?total-measured:0)/ticks_per_us;
 /* value[7]: scanner interrupts in this buffer, separating rate from cost.
  * Validity is c->valid / a nonzero LIMIT (value[6]), not this count. */
 c->value[5]=voices;c->value[6]=threshold/ticks_per_us;c->value[7]=scan_irqs;
 c->valid=1;
}
#endif
