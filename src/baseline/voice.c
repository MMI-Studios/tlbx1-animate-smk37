/* SPDX-License-Identifier: GPL-3.0-only
 * Use the preserved Melodee voice lifecycle directly. Only note ownership and
 * extension state are supplied by Animate; allocation, ramps and mixing stay
 * in the shared host. Engine capacity uses its existing cap callback. */
#define MELODEE_ANIMATE_ONLY 1
#define MELODEE_NOTE_REUSE(t, v, pitch) (!bridge_owned_note && (v)->note == (pitch))
#define MELODEE_VOICE_STARTED(v) bridge_voice_started(v)
#define MELODEE_RENDER_BEGIN(t) bridge_pan_clear()
#include "../../vendor/melodee/firmware/src/voice.c"
#undef MELODEE_RENDER_BEGIN
#undef MELODEE_VOICE_STARTED
#undef MELODEE_NOTE_REUSE
#undef MELODEE_ANIMATE_ONLY

