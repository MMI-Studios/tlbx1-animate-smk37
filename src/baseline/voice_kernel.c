/* SPDX-License-Identifier: GPL-3.0-only
 * Unified Animate voice kernel (owner-approved 2026-11-07: near-identical
 * instead of bit-identical output, in exchange for polyphony).
 *
 * One render path for every parameter state. Per voice and 32-sample block,
 * ak_prepare() (always CPU0) resolves each slot exactly as before: native
 * value, edited value or per-voice lock, the same filter choice, envelope rates,
 * windows and modulation. It also advances the vector and position LFOs, the
 * vector smoothing and envelope stages at block rate. ak_run() (CPU0 or CPU1)
 * then renders each audible slot in tight loops: wavetable lookup, the slot's
 * filter, then envelope and gain ramps. Slots silent for the whole block
 * are skipped, with their phase and envelope advanced analytically.
 *
 * Differences from the per-sample reference (extended_render.c / eng_animate.c):
 * vector and position LFOs, the vector target and envelope stage changes
 * update once per block (0.73 ms). Gains ramp linearly across the block; the
 * envelope itself still advances every sample. Paired voices render on both
 * cores in parallel, like Melodee's Prophet; their sums are order-independent
 * integer additions. */
enum { AK_FILTER_NONE, AK_FILTER_TSVF, AK_FILTER_SVF, AK_FILTER_LADDER };
#define AK_ENV_MAX ((int32_t)32767 << 16)
typedef struct {
    const int16_t *table;                 /* window base */
    uint32_t len, inc, offset;
    uint32_t max_offset, pos_rate;        /* position LFO, evaluated per sample when on */
    uint8_t pos_shape, pos_depth_native;
    float pos_depth;
    int16_t pos_depth_w;
    uint32_t *noise;
    int32_t env, mag, lim;                /* Q15.16 */
    int32_t gain, dgain;                  /* Q15.16 ramp */
    int32_t pan;                          /* Q15, 0 = centre */
    uint8_t active, filter, svf_type, reloop, edit_check, up;
    uint8_t src;
    float loop_start, loop_end;           /* reloop window (fractions of the table) */
    float g, k, a, pole, res;
    tsvf_t tsvf;
    animate_slot_voice_t *state;
    float *fstate;
    uint8_t *looped;
    uint16_t *frac;
} ak_slot_t;
typedef struct {
    ak_slot_t slot[4];
    vmod_t mod;
    uint32_t n;
    int32_t vel;
    uint8_t pan;
} ak_voice_job_t;
#include "../generated/wave_bank.h"
/* Four SRAM wave registers. A slot's SOURCE (0..3) picks a register; each
 * register holds frame 0 of any bank entry. Entries 0..3 are the original
 * Animate waves at their native length; curated entries are 256 samples.
 * Registers are filled at event time only, so rendering never reads flash. */
#define AK_WAVE_MAX AK_BANK_FRAME
static int16_t ak_wave[4][AK_WAVE_MAX];
static uint32_t ak_len[4];
static uint16_t ak_sel[4];
static uint16_t ak_env_frac[NVOICE][4];
static uint8_t ak_tables_ready;
_Static_assert(AK_BANK_FRAME >= 172u, "register holds the original waves");
static void ak_load(unsigned reg, unsigned entry)
{
    if (entry < AK_BANK_ORIGINALS) {
        /* Q31 assets to Q15, with C's truncating division as before. */
        uint32_t len = animate_poc_lengths[entry];
        for (unsigned i = 0; i < len && i < AK_WAVE_MAX; i++)
            ak_wave[reg][i] = (int16_t)(animate_poc_tables[entry][i] / 65536);
        ak_len[reg] = len < AK_WAVE_MAX ? len : AK_WAVE_MAX;
    } else {
        const int16_t *src = ak_bank_data + (uint32_t)ak_bank_entries[entry].first * AK_BANK_FRAME;
        for (unsigned i = 0; i < AK_BANK_FRAME; i++) ak_wave[reg][i] = src[i];
        ak_len[reg] = AK_BANK_FRAME;
    }
    ak_sel[reg] = (uint16_t)entry;
}
static void ak_tables_init(void)
{
    for (unsigned r = 0; r < 4; r++) ak_load(r, r);
    ak_tables_ready = 1;
}
int ab_wave(unsigned reg, unsigned entry)
{
    if (reg >= 4 || entry >= AK_BANK_COUNT) return 0;
    if (ak_sel[reg] != entry || !ak_tables_ready) ak_load(reg, entry);
    return 1;
}
unsigned ab_wave_get(unsigned reg) { return reg < 4 ? ak_sel[reg] : 0; }
unsigned ab_wave_count(void) { return AK_BANK_COUNT; }
unsigned ab_wave_frames(unsigned entry) { return entry < AK_BANK_ORIGINALS ? 1u : entry < AK_BANK_COUNT ? ak_bank_entries[entry].frames : 0u; }
const char *ab_wave_name(unsigned entry) { return entry < AK_BANK_COUNT ? ak_bank_entries[entry].name : "--"; }
MELODEE_DSP_INLINE int32_t ak_lookup(const int16_t *t, uint32_t len, uint32_t phase, uint32_t off)
{
    uint64_t pos = (uint64_t)phase * len;
    uint32_t i = (uint32_t)(pos >> 32) + off;
    if (i >= len) i -= len;
    uint32_t frac = (uint32_t)pos >> 16;
    int32_t a = t[i], b = t[i + 1u == len ? 0 : i + 1u], d = b - a;
    int32_t ip = (int32_t)(((uint32_t)(d < 0 ? -d : d) * frac) >> 16);
    return a + (d < 0 ? -ip : ip);
}
MELODEE_DSP_INLINE void ak_window(float begin, float end, uint32_t len, uint32_t *s, uint32_t *e)
{
    *s = (uint32_t)(begin * len); if (*s >= len) *s = len - 1;
    *e = (uint32_t)(end * len); if (*e <= *s) *e = *s + 1;
}
MELODEE_DSP_INLINE void ak_weights(int32_t x, int32_t y, int solo, int selected, int32_t w[4])
{
    w[0] = mulq15(32767 - x, 32767 - y); w[1] = mulq15(x, 32767 - y);
    w[2] = mulq15(32767 - x, y);         w[3] = mulq15(x, y);
    if (solo) for (int q = 0; q < 4; q++) w[q] = q == selected ? 32767 : 0;
}
MELODEE_DSP_INLINE int32_t ak_smooth(int32_t s, int32_t target)
{
    /* TLBX's 0.999/0.001 smoothing, with at least one Q15 unit of progress. */
    int32_t d = target - s, q = ANIMATE_DIV1000(d);
    return s + q + (d && q == 0 ? (d > 0 ? 1 : -1) : 0);
}
MELODEE_DSP_INLINE int32_t ak_vector_lfo(uint32_t phase, unsigned shape, uint32_t noise, int ext)
{
    if (ext) return bridge_lfo(phase, shape, noise);
    return shape == 1 ? osc_tri(phase) : shape == 2 ? (int32_t)(phase >> 16) - 32768 :
           shape == 3 ? (phase & 0x80000000u ? -32767 : 32767) : osc_sine(phase);
}
static ANIMATE_HOT void ak_prepare(track_t *t, voice_t *v, uint32_t n, const vmod_t *m, ak_voice_job_t *j)
{
    unsigned vi = (unsigned)(v - t->v);
    animate_part_t *part = animate_part((uint32_t)(t - trk));
    j->n = 0; j->pan = 0;
    if (!part || !n || n > CTL) return;
    animate_config_t *c = animate_config_of(t);
    animate_motion_t *motion = &animate_motion[(uint32_t)(t - trk) % NPART];
    j->n = n; j->vel = v->vel * 258; j->mod = *m;
    /* Vector position: LFO value at the block start, smoothed per sample. */
    uint32_t rate = (uint32_t)t->p[P_E2] * (uint32_t)(4294967296.0 / (127.0 * FS)) * 4u;
    uint32_t rate_y = (uint32_t)c->yrate * (uint32_t)(4294967296.0 / (127.0 * FS)) * 4u;
    int32_t depth_x = t->p[P_E3] * 16383 / 127, depth_y = c->ydepth * 16383 / 127;
    unsigned shape_x = (unsigned)c->xshape, shape_y = (unsigned)c->yshape;
    int ext_x = 0, ext_y = 0;
    if (bridge_vector_lfo_masks[0]) {
        float *p = bridge_vector_lfo[0];
        if (bridge_vector_lfo_masks[0] & 26u) rate = bridge_vector_rate(0);
        if (bridge_vector_lfo_masks[0] & 4u) depth_x = (int32_t)(p[2] * 16383.f);
        if (bridge_vector_lfo_masks[0] & 1u) { ext_x = 1; shape_x = (unsigned)p[0]; }
    }
    if (bridge_vector_lfo_masks[1]) {
        float *p = bridge_vector_lfo[1];
        if (bridge_vector_lfo_masks[1] & 26u) rate_y = bridge_vector_rate(1);
        if (bridge_vector_lfo_masks[1] & 4u) depth_y = (int32_t)(p[2] * 16383.f);
        if (bridge_vector_lfo_masks[1] & 1u) { ext_y = 1; shape_y = (unsigned)p[0]; }
    }
    uint32_t lfo = v->ph[1], lfo_y = v->ph[2];
    int32_t lx = ak_vector_lfo(lfo, shape_x, bridge_vector_noise[vi][0], ext_x);
    int32_t ly = ak_vector_lfo(lfo_y, shape_y, bridge_vector_noise[vi][1], ext_y);
    int32_t bx = bridge_vector_override ? (int32_t)(bridge_vector_x * 32767) : t->p[P_E0] * 32767 / 127;
    int32_t by = bridge_vector_override ? (int32_t)(bridge_vector_y * 32767) : t->p[P_E1] * 32767 / 127;
    int32_t tx = clamp(bx + mulq15(lx, depth_x), 0, 32767), ty = clamp(by + mulq15(ly, depth_y), 0, 32767);
    int32_t sx = ak_smooth(v->s[0], tx), sy = ak_smooth(v->s[1], ty);
    int solo = animate_solo[(uint32_t)(t - trk) % NPART] != 0;
    int32_t w0[4], w1[4];
    ak_weights(sx, sy, solo, c->selected, w0);
    for (uint32_t i = 1; i < n; i++) { sx = ak_smooth(sx, tx); sy = ak_smooth(sy, ty); }
    v->s[0] = sx; v->s[1] = sy;
    ak_weights(sx, sy, solo, c->selected, w1);
    v->ph[1] = lfo + rate * n; v->ph[2] = lfo_y + rate_y * n;
    if (v->ph[1] < lfo) { uint32_t z = bridge_vector_noise[vi][0]; z ^= z << 13; z ^= z >> 17; z ^= z << 5; bridge_vector_noise[vi][0] = z; }
    if (v->ph[2] < lfo_y) { uint32_t z = bridge_vector_noise[vi][1]; z ^= z << 13; z ^= z >> 17; z ^= z << 5; bridge_vector_noise[vi][1] = z; }

    for (unsigned q = 0; q < 4; q++) {
        ak_slot_t *s = &j->slot[q];
        animate_slot_voice_t *st = &part->slot[vi][q];
        const int16_t *p = c->slot[q], *w = motion->slot[q];
        uint32_t masks = bridge_masks[q] | bridge_lock_masks[vi][q];
        float vals[20];
        const bridge_coefficients_t *cf = 0;
        if (masks) {
            for (unsigned k = 0; k < 20; k++)
                vals[k] = (bridge_lock_masks[vi][q] & (1u << k)) ? bridge_locks[vi][q][k] : bridge_values[q][k];
            cf = bridge_coefficients_get(q, vals, masks);
        }
        unsigned src = (masks & 1u) ? (unsigned)vals[0] : (unsigned)p[0];
        src &= 3u;
        uint32_t len = ak_len[src], start, end;
        start = (uint32_t)w[0] * len / 127u; if (start >= len) start = len - 1;
        end = (uint32_t)w[1] * len / 127u; if (end <= start) end = start + 1;
        s->reloop = 0;
        if (masks & (7u << 10)) {
            ak_window(bridge_looped[vi][q] ? vals[11] : vals[10], vals[12], len, &start, &end);
            s->reloop = 1; s->loop_start = vals[11]; s->loop_end = vals[12];
        }
        s->src = (uint8_t)src;
        s->table = ak_wave[src] + start; s->len = end - start;
        s->state = st; s->fstate = bridge_filter[vi][q]; s->looped = &bridge_looped[vi][q]; s->frac = &ak_env_frac[vi][q];
        /* Pitch */
        if (masks & ((1u << 1) | (1u << 2))) {
            float step = cents_inc(m->pitch16, 0, v->scale_fine) * cf->pitch;
            while (step >= 4294967296.f) step -= 4294967296.f;
            s->inc = (uint32_t)step;
        } else s->inc = cents_inc(m->pitch16 + p[AN_TUNE - 1] * 16, 0, v->scale_fine);
        /* Position LFO: offset held for the block. */
        uint32_t span = s->len - 1u;
        uint32_t max_offset = (masks & (1u << 5)) ? (uint32_t)(span * vals[5]) : span * (uint32_t)w[4] / 127u;
        unsigned shape = (masks & (1u << 6)) ? (unsigned)vals[6] : (w[2] == 2 ? 3u : w[2] == 3 ? 2u : (unsigned)w[2]);
        uint32_t pos_rate = (masks & (7u << 7)) ? cf->position_rate : (uint32_t)w[3] * (uint32_t)(4294967296.0 / (127.0 * FS)) * 4u;
        s->offset = max_offset ? ANIMATE_UDIV65535((uint32_t)(bridge_lfo(st->position_phase, shape, bridge_noise[vi][q]) + 32768) * max_offset) : 0;
        if (s->offset >= s->len) s->offset = s->len - 1;
        s->max_offset = max_offset; s->pos_rate = pos_rate; s->pos_shape = (uint8_t)shape;
        s->pos_depth_native = !(masks & (1u << 5)); s->pos_depth = (masks & (1u << 5)) ? vals[5] : 0.f; s->pos_depth_w = w[4];
        s->noise = &bridge_noise[vi][q];
        /* Live window edits: cancel the discontinuity at the next sample. */
        s->edit_check = st->window_ready && (st->last_start != w[0] || st->last_end != w[1] || st->last_shape != w[2] ||
                        st->last_rate != w[3] || st->last_depth != w[4]);
        st->last_start = w[0]; st->last_end = w[1]; st->last_shape = w[2]; st->last_rate = w[3]; st->last_depth = w[4];
        st->window_ready = 1;
        /* Envelope: rate and stage for this block; samples advance it exactly. */
        int32_t a, d, r, sus;
        if (masks & (15u << 16)) {
            a = cf->attack >= 32767.f ? AK_ENV_MAX : (int32_t)(cf->attack * 65536.f);
            d = cf->decay >= 32767.f ? AK_ENV_MAX : (int32_t)(cf->decay * 65536.f);
            r = cf->release >= 32767.f ? AK_ENV_MAX : (int32_t)(cf->release * 65536.f);
            sus = (int32_t)(vals[18] * 32767.f);
        } else {
            const bridge_native_coefficients_t *nc = bridge_native_coefficients_get(q, p);
            a = nc->attack >= 32767 ? AK_ENV_MAX : nc->attack << 16;
            d = nc->decay >= 32767 ? AK_ENV_MAX : nc->decay << 16;
            r = nc->release >= 32767 ? AK_ENV_MAX : nc->release << 16;
            sus = p[AN_SUS - 1] * 32767 / 127;
        }
        sus = clamp(sus, 0, 32767) << 16;
        if (!st->env && !st->stage) ak_env_frac[vi][q] = 0;   /* (re)started voice */
        int32_t e = (int32_t)((uint32_t)clamp(st->env, 0, 32767) << 16 | ak_env_frac[vi][q]);
        if (!v->gate) st->stage = 3;
        if (st->stage == 0 && e >= AK_ENV_MAX) st->stage = 1;
        if (st->stage == 1 && e <= sus) { e = sus; st->stage = 2; }
        switch (st->stage) {
        case 0: s->up = 1; s->mag = a; s->lim = AK_ENV_MAX; break;
        case 1: s->up = 0; s->mag = d; s->lim = sus; break;
        case 2: s->up = 0; s->mag = 0; e = sus; s->lim = sus; break;
        default: s->up = 0; s->mag = r; s->lim = 0; break;
        }
        s->env = e;
        /* Gain: vector weight, slot level and mixer level, ramped over the block. */
        int32_t level = (masks & (1u << 3)) ? clamp((int32_t)(vals[3] * 32767.f), 0, 32767) : p[AN_LEVEL - 1] * 32767 / 127;
        int32_t lm = level * clamp(t->p[P_E4 + q], 0, 127) / 127;
        int32_t g0 = mulq15(w0[q], lm), g1 = mulq15(w1[q], lm);
        s->gain = g0 << 16; s->dgain = (int32_t)(((int64_t)(g1 - g0) << 16) / (int32_t)n);
        s->pan = (masks & (1u << 4)) ? clamp((int32_t)(vals[4] * 32767.f), -32767, 32767) : 0;
        if (s->pan) j->pan = 1;
        s->active = (g0 || g1) && !(st->stage == 3 && e == 0);
        /* Filter: the same choice as before for this slot's parameters. */
        s->filter = AK_FILTER_NONE;
        if (!(bridge_masks[q] & (1u << 20))) {
            if (masks & (7u << 13)) {
                unsigned type = (unsigned)vals[13];
                s->fstate = bridge_filter[vi][q];
                if (type == 4) { s->filter = AK_FILTER_LADDER; s->a = cf->pole; s->res = vals[15]; }
                else { s->filter = AK_FILTER_SVF; s->svf_type = (uint8_t)type; s->g = cf->g; s->k = cf->k; s->a = cf->inverse; }
            } else if (!p[AN_BYPASS - 1]) {
                s->filter = AK_FILTER_TSVF;
                bridge_filter_coefficients(&s->tsvf, q, p[AN_CUT - 1] << 8, p[AN_RES - 1]);
            }
        }
    }
}
static ANIMATE_HOT void ak_filter(ak_slot_t *s, int32_t *x, uint32_t n)
{
    animate_slot_voice_t *st = s->state;
    if (s->filter == AK_FILTER_TSVF) {
        int32_t ic1 = st->ic1, ic2 = st->ic2;
        for (uint32_t i = 0; i < n; i++) x[i] = clamp(tsvf_lp(&s->tsvf, x[i], &ic1, &ic2), -32767, 32767);
        st->ic1 = ic1; st->ic2 = ic2;
    } else if (s->filter == AK_FILTER_SVF) {
        float *z = s->fstate, g = s->g, k = s->k, a = s->a, ga = g * a, gga = g * g * a;
        unsigned type = s->svf_type;
        for (uint32_t i = 0; i < n; i++) {
            float in = x[i] * (1.f / 32768.f), v3 = in - z[1], bp = a * z[0] + ga * v3, lp = z[1] + ga * z[0] + gga * v3;
            z[0] = 2 * bp - z[0]; z[1] = 2 * lp - z[1];
            float y = type == 2 ? in - k * bp - lp : type == 3 ? bp : lp;
            if (type == 0) { v3 = y - z[3]; bp = a * z[2] + ga * v3; lp = z[3] + ga * z[2] + gga * v3; z[2] = 2 * bp - z[2]; z[3] = 2 * lp - z[3]; y = lp; }
            x[i] = clamp((int32_t)(bridge_clampf(y, -1.f, 1.f) * 32767.f), -32767, 32767);
        }
    } else if (s->filter == AK_FILTER_LADDER) {
        float *z = s->fstate, pole = s->a, fb = s->res * 3.8f;
        for (uint32_t i = 0; i < n; i++) {
            float y = av_tanh(x[i] * (1.f / 32768.f) - fb * z[3]);
            for (unsigned k = 0; k < 4; k++) { z[k] += pole * (av_tanh(y) - av_tanh(z[k])); y = z[k]; }
            x[i] = clamp((int32_t)(bridge_clampf(y, -1.f, 1.f) * 32767.f), -32767, 32767);
        }
    }
}
MELODEE_DSP_INLINE int32_t ak_env_step(int32_t e, int32_t mag, int32_t lim, unsigned up)
{
    if (up) return lim - e <= mag ? lim : e + mag;
    return e - lim <= mag ? lim : e - mag;
}
static ANIMATE_HOT void ak_slot_run(ak_slot_t *s, uint32_t n, int32_t *acc, int32_t *pl, int32_t *pr)
{
    animate_slot_voice_t *st = s->state;
    int32_t e = s->env;
    if (!s->active) {
        /* Silent for the whole block: advance phase and envelope only. */
        uint32_t pp = st->position_phase;
        st->position_phase = pp + s->pos_rate * n;
        if (st->position_phase < pp) { uint32_t z = *s->noise; z ^= z << 13; z ^= z >> 17; z ^= z << 5; *s->noise = z; }
        uint64_t next = (uint64_t)st->phase + (uint64_t)s->inc * n;
        if (next >> 32) *s->looped = 1;
        st->phase = (uint32_t)next;
        int64_t span = (int64_t)s->mag * n;
        if (s->up) e = (int64_t)(s->lim - e) <= span ? s->lim : e + (int32_t)span;
        else e = (int64_t)(e - s->lim) <= span ? s->lim : e - (int32_t)span;
        st->env = e >> 16; *s->frac = (uint16_t)e;
        return;
    }
    int32_t x[CTL];
    uint32_t phase = st->phase, inc = s->inc, len = s->len, off = s->offset;
    const int16_t *tb = s->table;
    int32_t eo = st->edit_offset;
    if (s->edit_check) eo = st->last_output - ak_lookup(tb, len, phase, off);
    if (!eo && !s->reloop && !s->max_offset) {
        if (((uint64_t)phase + (uint64_t)inc * n) >> 32) *s->looped = 1;
        for (uint32_t i = 0; i < n; i++) { x[i] = clamp(ak_lookup(tb, len, phase, off), -32767, 32767); phase += inc; }
        uint32_t pp = st->position_phase;
        st->position_phase = pp + s->pos_rate * n;
        if (st->position_phase < pp) { uint32_t z = *s->noise; z ^= z << 13; z ^= z >> 17; z ^= z << 5; *s->noise = z; }
    } else {
        /* Exact per-sample path for position modulation, declick and loops. */
        uint32_t pp = st->position_phase, prate = s->pos_rate, mo = s->max_offset, nz = *s->noise;
        unsigned shape = s->pos_shape;
        for (uint32_t i = 0; i < n; i++) {
            if (mo) off = ANIMATE_UDIV65535((uint32_t)(bridge_lfo(pp, shape, nz) + 32768) * mo);
            int32_t value = clamp(ak_lookup(tb, len, phase, off) + eo, -32767, 32767);
            x[i] = value;
            if (eo) eo -= ANIMATE_SDIV_POW2(eo, 7) + (eo > 0 ? 1 : -1);
            uint32_t prevpos = pp;
            pp += prate;
            if (pp < prevpos) { nz ^= nz << 13; nz ^= nz >> 17; nz ^= nz << 5; }
            uint32_t prev = phase;
            phase += inc;
            if (phase < prev) {
                *s->looped = 1;
                if (s->reloop) {
                    uint32_t full = ak_len[s->src], start, end;
                    ak_window(s->loop_start, s->loop_end, full, &start, &end);
                    tb = ak_wave[s->src] + start; len = end - start;
                    if (mo) mo = s->pos_depth_native ? (len - 1u) * (uint32_t)s->pos_depth_w / 127u : (uint32_t)((len - 1u) * s->pos_depth);
                    if (off >= len) off = len - 1;
                }
            }
        }
        st->position_phase = pp; *s->noise = nz;
    }
    st->phase = phase; st->edit_offset = eo; st->last_output = x[n - 1];
    ak_filter(s, x, n);
    int32_t g = s->gain, dg = s->dgain, lim = s->lim, mag = s->mag;
    unsigned up = s->up;
    if (!s->pan) {
        for (uint32_t i = 0; i < n; i++) {
            e = ak_env_step(e, mag, lim, up);
            acc[i] += mulq15(mulq15(x[i], e >> 16), g >> 16);
            g += dg;
        }
    } else {
        int32_t pan = s->pan;
        for (uint32_t i = 0; i < n; i++) {
            e = ak_env_step(e, mag, lim, up);
            int32_t contribution = mulq15(mulq15(x[i], e >> 16), g >> 16);
            acc[i] += contribution;
            if (pan > 0) pl[i] -= mulq15(contribution, pan); else pr[i] += mulq15(contribution, pan);
            g += dg;
        }
    }
    st->env = e >> 16; *s->frac = (uint16_t)e;
}
/* Render one prepared voice, adding into out and (when it pans) the pan buffers. */
static ANIMATE_HOT void ak_run(ak_voice_job_t *j, int32_t *out, int32_t *pan_l, int32_t *pan_r)
{
    uint32_t n = j->n;
    if (!n) return;
    int32_t acc[CTL], pl[CTL], pr[CTL];
    for (uint32_t i = 0; i < n; i++) acc[i] = pl[i] = pr[i] = 0;
    for (unsigned q = 0; q < 4; q++) ak_slot_run(&j->slot[q], n, acc, pl, pr);
    int32_t vel = j->vel;
    for (uint32_t i = 0; i < n; i++) out[i] += voice_amp(mulq15(acc[i], vel), &j->mod, i) * 2;
    if (j->pan)
        for (uint32_t i = 0; i < n; i++) {
            pan_l[i] += voice_amp(mulq15(pl[i], vel), &j->mod, i) * 2;
            pan_r[i] += voice_amp(mulq15(pr[i], vel), &j->mod, i) * 2;
        }
}
/* Parallel voices: the platform supplies the donor mailbox (audio_worker.h). */
static int (*ak_hook_submit)(void (*)(void *), void *);
static int (*ak_hook_join)(void);
void ab_worker_hooks(int (*submit)(void (*)(void *), void *), int (*join)(void)) { ak_hook_submit = submit; ak_hook_join = join; }
typedef struct { ak_voice_job_t job; int32_t pcm[CTL], pl[CTL], pr[CTL]; int32_t *out; } ak_pending_t;
static ak_pending_t ak_pend;
static uint8_t ak_pending;
static uint32_t ak_pairs, ak_lost;
static ANIMATE_HOT void ak_worker(void *context)
{
    ak_pending_t *p = context;
    for (uint32_t i = 0; i < p->job.n; i++) p->pcm[i] = p->pl[i] = p->pr[i] = 0;
    ak_run(&p->job, p->pcm, p->pl, p->pr);
}
static ANIMATE_HOT void ak_finish(void)
{
    if (!ak_pending) return;
    ak_pending = 0;
    if (!ak_hook_join()) { ak_lost++; return; }  /* CPU1 retired: this voice's block is dropped */
    uint32_t n = ak_pend.job.n;
    for (uint32_t i = 0; i < n; i++) ak_pend.out[i] += ak_pend.pcm[i];
    if (ak_pend.job.pan)
        for (uint32_t i = 0; i < n; i++) { bridge_pan_l[i] += ak_pend.pl[i]; bridge_pan_r[i] += ak_pend.pr[i]; }
}
/* Will voice.c render another voice after vi? (It ends voices whose gate is
 * off and whose four slot envelopes are zero, as animate_done.) */
static ANIMATE_HOT int ak_next_renders(track_t *t, unsigned vi, const animate_part_t *part)
{
    for (unsigned k = vi + 1; k < NVOICE; k++) {
        const voice_t *o = &t->v[k];
        if (!o->active) continue;
        if (o->gate) return 1;
        for (unsigned q = 0; q < 4; q++) if (part->slot[k][q].env) return 1;
    }
    return 0;
}
static ANIMATE_HOT void ak_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    animate_part_t *part = animate_part((uint32_t)(t - trk));
    if (!part) return;
    if (!ak_tables_ready) ak_tables_init();
    if (!ak_pending && ak_hook_submit && ak_next_renders(t, (unsigned)(v - t->v), part)) {
        ak_prepare(t, v, n, m, &ak_pend.job);
        ak_pend.out = out;
        if (ak_hook_submit(ak_worker, &ak_pend)) { ak_pending = 1; return; }
        ak_run(&ak_pend.job, out, bridge_pan_l, bridge_pan_r);   /* worker unavailable */
        return;
    }
    ak_voice_job_t job;
    ak_prepare(t, v, n, m, &job);
    if (ak_pending) {
        int32_t pcm[CTL], pl[CTL], pr[CTL];
        for (uint32_t i = 0; i < n; i++) pcm[i] = pl[i] = pr[i] = 0;
        ak_run(&job, pcm, pl, pr);
        ak_pairs++;
        ak_finish();
        for (uint32_t i = 0; i < n; i++) out[i] += pcm[i];
        if (job.pan) for (uint32_t i = 0; i < n; i++) { bridge_pan_l[i] += pl[i]; bridge_pan_r[i] += pr[i]; }
        return;
    }
    ak_run(&job, out, bridge_pan_l, bridge_pan_r);
}
static ANIMATE_HOT void ak_post(track_t *t, int32_t *out, uint32_t n, uint32_t nr)
{
    (void)t; (void)out; (void)n; (void)nr;
    ak_finish();
}
