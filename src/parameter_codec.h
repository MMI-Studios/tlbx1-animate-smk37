#ifndef ANIMATE_PARAMETER_CODEC_H
#define ANIMATE_PARAMETER_CODEC_H
#include "animate_parameters.h"
#include <stdint.h>
#define AP_LOCK_BYTES 19
typedef struct {uint8_t bytes[AP_LOCK_BYTES];} ap_lock;
/* Fixed factory catalogue: four real baseline tables. No imported source IDs
 * are silently truncated; extending the catalogue requires a schema revision. */
void ap_lock_clear(ap_lock *lock);
void ap_lock_unset(ap_lock *lock,unsigned parameter);
int ap_lock_set(ap_lock *lock,unsigned parameter,float value);
int ap_lock_get(const ap_lock *lock,unsigned parameter,float *value);
int ap_lock_valid(const ap_lock *lock);
float ap_parameter_resolution(unsigned parameter);
#endif
