/* Animate-only host extraction from vendor/melodee/firmware/src/params.c.
 * General integer arithmetic retained; native-engine dispatch and old UI excluded. */
/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Parameter descriptors, formatting and the page table. */
static const char *const N_LSYNC[] = {"OFF", "4BAR", "2BAR", "1/1", "1/2", "1/4", "1/8", "8T", "1/16", "16T", "1/32"};
static const uint8_t LSYNC_DIV[11] = {0, 9, 8, 7, 6, 0, 1, 4, 2, 5, 3};
static const char *const N_LWAVE[] = {"SIN", "TRI", "SAW", "SQR", "S&H"};
static const char *const N_AMODE[] = {"OFF", "UP", "DN", "UPDN", "RND", "ORD", "REPEAT"};
static const char *const N_DIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR"};
/* the delay's TIME (delay.c dly_samples): N_DIV's ten (their stored ids), then x0x's dotted and 1/4 triplet */
static const char *const N_DDIV[] = {"1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR",
                                     "1/16.", "1/8.", "1/4.", "4T"};
static const char *const N_DTYPE[] = {"DIGI", "TAPE", "DG-PP", "TP-PP"};   /* G_DTYPE (delay.c): PP ping-pong */
static const char *const N_RECQ[] = {"OFF", "1/4", "1/8", "1/16", "1/32", "8T", "16T", "1/2", "1/1", "2BAR", "4BAR"};
static const char *const N_LTRIG[] = {"NOTE", "FREE"};
static const char *const N_LPOL[] = {"BI", "UNI"};
static const char *const N_ONOFF[] = {"OFF", "ON"};
static const char *const N_QUANT[] = {"OFF", "SNAP", "WHITE", "ALL", "MPC"};   /* Q_OFF .. Q_MPC (seq.c kb_map, midi_map) */
/* chord keys (chord.c): OFF, the diatonic triad / seventh of the track's ROOT and SCALE on the key, fixed shapes */
static const char *const N_CHRD[] = {"OFF", "DIA3", "DIA7", "MAJ", "MIN", "DOM7", "MAJ7", "MIN7", "SUS4", "POW"};
static const char *const N_VOIC[] = {"CLOSE", "OPEN", "INV1", "INV2", "+OCT"};   /* VC_CLOSE .. VC_BASS */
static const char *const N_VOICE[] = {"POLY", "MONO", "LEG", "UNI"};   /* V_POLY .. V_UNISON */
static const char *const N_GLMODE[] = {"RATE", "TIME"};
static const char *const N_PRIO[] = {"LAST", "LOW", "HIGH"};
static const char *const N_ALLOC[] = {"ROT", "REUSE"};
static const char *const N_ORDER[] = {"NOTE", "PLAY"};
static const char *const N_CLOCK[] = {"INT", "USB", "TRS"};
static const char *const N_MIDI_INPUT[] = {"USB", "TRS"};
static const char *const N_ROUTE[] = {"CH1-4", "SEL"};   /* MIDI IN (seq.c midi_track): channels 1..4 -> parts 1..4 / all -> the selected */
static const char *const N_NOTE[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const N_DASH[] = {"--"};
static const char *const N_RTYPE[] = {"ROOM", "SPRING", "HALL"};   /* G_RTYPE: the reverb bus's model (fx.c) */
static const char *const N_GO[] = {"--", "GO"};
static const char *const N_BOOT[] = {"OFF", "A", "B", "C", "D"};
static const char *const N_DRUMCH[] = {"OFF", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14",
                                       "15", "16"};
/* GLO > SYSTEM KNOB 2, DRUM: the MIDI channel that plays the first DRUM track whatever ROUT says (none: the selected
 * track), 0 OFF, 1..16; 10 as General MIDI's. A device setting kept with the settings (ext.drumch), not the
 * project's: the column shows drumch_cell (seq.c midi_track) */
static uint8_t settings_drumch = 10;
static int16_t drumch_cell;
#define PROJ_TMPL 5                                      /* SLOT 5: the template (project.c tmpl) */
/* SAVE > PROJECT KNOB 2, BOOT: the project power-on loads (OFF, A..D; OFF or an empty slot: the template, if one is
 * saved). A device setting kept with the settings (ext.boot), not the project's: the column shows boot_cell */
static uint8_t settings_boot;
/* Device recording preferences: defaults preserve existing behavior. */
#include "recording_preferences.h"
static int16_t boot_cell;
static const char *const N_SLCR[] = {"OFF", "GATE", "STUT"};             /* SL_OFF .. SL_STUT (slicer.c) */
static const char *const N_SLDIV[] = {"1/8", "1/16", "1/32", "8T", "16T", "32T"};   /* SL_DEN */
/* modulation matrix (mod.c): sources, destinations (E1..E8 = P_E0..P_E7: shown with the engine's labels) */
static const char *const N_MSRC[] = {"OFF", "LFO", "ENV", "VEL", "KEY", "RAND", "MODW", "AT", "EXPR", "S&H", "SLEW"};
static const char *const N_MDST[] = {"OFF", "PITCH", "CUT", "SHP", "AMP", "PAN", "DIST", "CHO", "DLY", "REV", "RATE",
                                     "VIB", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "DEPTH"};
static const char *const N_ENGNAME[] = {"ANALOG", MELODEE_FM4 ? "DIGITAL" : "-", MELODEE_LEGACY_EXTRAS ? "PHASE" : "-", "SID", "-", MELODEE_LEGACY_EXTRAS ? "VOICE" : "-", MELODEE_LEGACY_EXTRAS ? "TRIO" : "-", MELODEE_LEGACY_EXTRAS ? "WHEEL" : "-", "-", MELODEE_LEGACY_EXTRAS ? "PHYS" : "-",
                                             "DRUM", MELODEE_LEGACY_EXTRAS ? "NOISE" : "-", "FM6", MELODEE_SLICE ? "SLICE" : "-", "-", "CZ-1", "-", "-", "-", "PROPHET"};

#define PD(l, f, mn, mx, df) {l, f, mn, mx, df, 0, 0}
#define PE(l, n, df) {l, F_ENUM, 0, (int16_t)(sizeof(n) / sizeof(n[0]) - 1), df, n, 0}

/* FM6's pages (an FM6 track only): the selected track's patch (fm6_patch), its operator switches and the FM6 function
 * settings, in the DX7's ranges. The operator pages show operator fm6_opsel (the PRESETS knob picks it); the STORE
 * page saves the whole sound into user preset slot fm6_bslot (fm6_store.c). The values are not track parameters: page_desc gives a copy
 * (fm6_cell), fm6_page_put writes it back */
static const char *const N_FM6ALG[32] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
                                        "16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28",
                                        "29", "30", "31", "32"};
static const char *const N_FMCRV[] = {"-LIN", "-EXP", "+EXP", "+LIN"};
static const char *const N_FMMODE[] = {"RATIO", "FIXED"};
static const char *const N_FMLFW[] = {"TRI", "SAWDN", "SAWUP", "SQR", "SIN", "S&H"};
static const char *const N_FMPM[] = {"PEDAL", "ON"};   /* portamento: while CC 65 is down (Dexed), always */
static const char *const N_FMDEST[] = {"-", "P", "A", "PA", "E", "PE", "AE", "PAE"};   /* pitch, amp, EG bias */
static const char *const N_FMENG[] = {"MODRN", "MARK I", "OPL"};                  /* Dexed's engine resolutions */
static const char *const N_FMONOFF[] = {"OFF", "ON"};
static const param_desc_t TP[P_COUNT] = {
    [P_LN0] = PD("KICK", F_PCT, 0, 127, 127),
    [P_LN1] = PD("SNARE", F_PCT, 0, 127, 127),
    [P_LN2] = PD("CLAP", F_PCT, 0, 127, 127),
    [P_LN3] = PD("HATCL", F_PCT, 0, 127, 127),
    [P_LN4] = PD("HATOP", F_PCT, 0, 127, 127),
    [P_LN5] = PD("TOM", F_PCT, 0, 127, 127),
    [P_LN6] = PD("RIM", F_PCT, 0, 127, 127),
    [P_LN7] = PD("BELL", F_PCT, 0, 127, 127),
    [P_LEVEL] = PD("LVL", F_DB, 0, 127, 104),
    [P_ATK] = PD("ATK", F_TIME, 0, 127, 10),
    [P_DEC] = PD("DEC", F_TIME, 0, 127, 70),
    [P_SUS] = PD("SUS", F_PCT, 0, 127, 90),
    [P_REL] = PD("REL", F_TIME, 0, 127, 60),
    [P_ED_FLT] = PD("FLT", F_BIPCT, -64, 63, 0),
    [P_ED_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_ED_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_ED_FX] = PE("QNT", N_RECQ, 0),
    [P_LSYNC] = PE("SYNC", N_LSYNC, 0),
    [P_LTRIG] = PE("TRIG", N_LTRIG, 0),
    [P_LPOL] = PE("POL", N_LPOL, 0),
    [P_SPRD] = PD("SPRD", F_PCT, 0, 127, 0),
    [P_LRATE] = PD("RATE", F_LFOHZ, 0, 127, 60),
    [P_LWAVE] = PE("WAVE", N_LWAVE, 0),
    [P_LPHASE] = PD("PHS", F_INT, 0, 127, 0),
    [P_LFADE] = PD("FADE", F_TIME, 0, 127, 0),
    [P_LD_PIT] = PD("PIT", F_BIPCT, -64, 63, 0),
    [P_LD_FLT] = PD("FLT", F_BIPCT, -64, 63, 0),
    [P_LD_SHP] = PD("SHP", F_BIPCT, -64, 63, 0),
    [P_LD_AMP] = PD("AMP", F_PCT, 0, 127, 0),
    [P_AMODE] = PE("MODE", N_AMODE, 0),
    [P_ARATE] = PE("RATE", N_DIV, 2),
    [P_AOCT] = PD("OCT", F_INT, 1, 4, 1),
    [P_AGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_ASWING] = PD("SWG", F_PCT, 0, 100, 0),
    [P_APROB] = PD("PROB", F_PCT, 0, 127, 127),
    [P_AHOLD] = PE("HOLD", N_ONOFF, 0),
    [P_AORDER] = PE("ORD", N_ORDER, 0),
    [P_ROOT] = PD("ROOT", F_NOTE, 0, 11, 0),
    [P_SCALE] = PE("SCL", N_SCALE, 0),
    [P_QUANT] = PE("QNT", N_QUANT, 0),
    [P_TRANS] = PD("TRN", F_SEMI, -24, 24, 0),
    [P_SLEN] = PD("LEN", F_STEPS, 1, NSTEP, 16),
    [P_SDIV] = PE("DIV", N_DIV, 2),
    [P_SSWING] = PD("SWG", F_PCT, 0, 100, 0),
    [P_SGATE] = PD("GATE", F_PCT, 1, 127, 64),
    [P_DIST] = PD("DST", F_PCT, 0, 127, 0),
    [P_CHOR] = PD("CHO", F_PCT, 0, 127, 0),
    [P_DLY] = PD("DLY", F_PCT, 0, 127, 0),
    [P_REV] = PD("REV", F_PCT, 0, 127, 0),
    [P_VOICE] = PE("VCE", N_VOICE, 0),
    [P_GLIDE] = PD("GLD", F_TIME, 0, 127, 0),
    [P_GLMODE] = PE("GLMOD", N_GLMODE, 0),
    [P_PRIO] = PE("PRIO", N_PRIO, 0),
    [P_ALLOC] = PE("ALLOC", N_ALLOC, 0),
    [P_DETUNE] = PD("DTUNE", F_INT, 0, 127, 40),
    [P_PAN] = PD("PAN", F_BIPCT, -64, 63, 0),
    [P_MUTE] = PE("MUTE", N_ONOFF, 0),
    [P_SLCR] = PE("SLCR", N_SLCR, 0),
    [P_SLPAT] = PD("PAT", F_INT, 1, 16, 1),        /* SL_PAT[] */
    [P_SLRATE] = PE("RATE", N_SLDIV, 1),
    [P_SLDEPTH] = PD("DEPTH", F_PCT, 0, 127, 127),
#define MSLOT(k) [P_M##k##SRC] = PE("SRC" #k, N_MSRC, 0), [P_M##k##DST] = PE("DST" #k, N_MDST, 0), \
                 [P_M##k##AMT] = PD("AMT" #k, F_BIPCT, -64, 63, 0)
    MSLOT(1), MSLOT(2), MSLOT(3), MSLOT(4),
#undef MSLOT
/* the OP ENV / OP LEVEL values of DIGITAL (ids 61..80): with MELODEE_FM4 its operator envelopes; without it
 * inert, on no page and in no editor layout, read only when a DIGITAL sound converts (fm4_convert.c). Their labels
 * stay: the editor's library files key parameters by label ("ATK#2" ..), so an old file's values still find them */
#define FMOP(k) [P_FM##k##_ATK] = PD("ATK", F_TIME, 0, 127, 0), \
                [P_FM##k##_DEC] = PD("DEC", F_TIME, 0, 127, 0), \
                [P_FM##k##_SUS] = PD("SUS", F_PCT, 0, 127, 127), \
                [P_FM##k##_REL] = PD("REL", F_TIME, 0, 127, 0), \
                [P_FM##k##_LEVEL] = PD("LVL", F_PCT, 0, 127, 127)
    FMOP(1), FMOP(2), FMOP(3), FMOP(4),
#undef FMOP
    [P_CHRD] = PE("CHRD", N_CHRD, 0),
    [P_VOIC] = PE("VOIC", N_VOIC, 0),
    [P_MPCDEG] = PD("DEG", F_INT, 1, 12, 1),         /* its max: the scale's notes (track_desc) */
};

static const param_desc_t GP[G_COUNT] = {
    [G_BPM] = PD("BPM", F_BPM, 40, 240, 120),
    [G_SWING] = PD("SWG", F_PCT, 0, 100, 0),
    [G_CLOCK] = PE("CLK", N_CLOCK, 0),
    [G_TUNE] = PD("TUNE", F_INT, -50, 50, 0),
    /* the delay (delay.c): the defaults x0x's (dotted eighths, TAPE) */
    [G_DTIME] = PE("TIME", N_DDIV, 11),
    [G_DFDBK] = PD("FDBK", F_PCT, 0, 120, 50),
    [G_DCOLOR] = PD("TONE", F_PCT, 0, 127, 51),
    [G_DMIX] = PD("MIX", F_PCT, 0, 127, 85),
    [G_RSIZE] = PD("SIZE", F_PCT, 0, 127, 90),
    [G_RDAMP] = PD("DAMP", F_PCT, 0, 127, 60),
    [G_CRATE] = PD("CRT", F_LFOHZ, 0, 127, 40),
    [G_CDEPTH] = PD("CDP", F_PCT, 0, 127, 60),
    [G_MIDI] = PE("MIDI", N_MIDI_INPUT, 0),
    [G_DRUMCH] = PE("DRUM", N_DRUMCH, 10),
    [G_ROUTE] = PE("ROUT", N_ROUTE, 0),          /* (was "--": stored 0 = CH1-4, as MIDI IN always was) */
    [G_INFO] = PD("CPU", F_INT, 0, 0, 0),
    [G_SLOT] = PD("SLOT", F_INT, 1, PROJ_TMPL, 1),     /* PROJ_TMPL: the template (param_format: TMPL) */
    [G_BOOT] = PE("BOOT", N_BOOT, 0),
    [G_LOAD] = PE("LOAD", N_GO, 0),
    [G_SAVE] = PE("SAVE", N_GO, 0),
    [G_ENGSEL] = PE("ENG", N_ENGNAME, 0),
    [G_A4] = {"A4", F_INT, A4_MIN, A4_MAX, A4_DEFAULT, 0, "Hz"},
    [G_CLRSEQ] = PE("CLRSQ", N_GO, 0),
    [G_INITSND] = PE("INIT", N_GO, 0),
    /* the reverb's model on the REVERB page: the id of the old GM drum channel (G_DRCH, inert since 1.0) */
    [G_RTYPE] = PE("TYPE", N_RTYPE, 0),
    /* the delay's TYPE and WEAR on the DLY 2 page: the ids of the old GM drum part's level and reverb send */
    [G_DTYPE] = PE("TYPE", N_DTYPE, 1),
    [G_DWEAR] = PD("WEAR", F_PCT, 0, 127, 64),
};


static const param_desc_t *track_desc(const track_t*t,uint32_t id){(void)t;return id>=P_E0?&ENG_ANIMATE.edit[id-P_E0]:&TP[id];}


