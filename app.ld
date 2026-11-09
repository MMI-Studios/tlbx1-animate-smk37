/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Animate memory map, adapted from MELODEE.
 * Reserve upper-pool SRAM (48 KiB) for DSP and interrupt code within the
 * direct-call range of XIP.
 * Preserve the 152KiB minimum working arena and all original stack/flash bounds.
 * Code + constants execute in place from XIP (app.bin offset 0 = 0x02000120).
 * 0x01C00000..0x01C06000 holds .ram_text (code that must run while the flash
 * is busy: the flash driver). Unused RAMTEXT/debug space ends at 0x01C07F00:
 * crt0 still writes STATUS in the last 256 bytes. Linker-derived SRAM tails
 * join the audio resource banks without changing retained state or stacks.
 * RAM  : .data/.bss (96 KiB)
 * POOL : big zero-initialised buffers (FX lines, canvases), section .pool
 * STACKS with 256-byte guard bands; NOINIT survives resets and UBOOT entry.
 * 0x01C7FD50.. is reserved: boot_info, boot-state mailbox, vector table. */
MEMORY
{
    XIP    (rx)  : ORIGIN = 0x02000120, LENGTH = 0x8DFBC   /* the app area to 0x93000 (fm1pkg_make.py APP_SLOT) */
    RAMTEXT (rwx): ORIGIN = 0x01C00000, LENGTH = 0x6000
    RAM    (rwx) : ORIGIN = 0x01C08000, LENGTH = 96K
    POOL   (rw)  : ORIGIN = 0x01C20000, LENGTH = 0x48000
    DSPHOT (rx)  : ORIGIN = 0x01C68000, LENGTH = 0xC000
    NOINIT (rw)  : ORIGIN = 0x01C7C000, LENGTH = 0x3D50
    CACHE  (rw)  : ORIGIN = 0x01F28000, LENGTH = 28K
}
SECTIONS
{
    /* crt0 stays first at APP_XIP. The SRAM DSP image is claimed next so its
     * named input sections win before the generic .text wildcard: every routine
     * executed per audio block or in the scanner/USB interrupts runs from SRAM
     * instead of competing with UI/display code for XIP instruction cache. */
    .boot : { KEEP(*(.text.crt0)) . = ALIGN(4); } > XIP
    .dsp_text : { _dsp_start = .; *(.dsp_text*)
        *(.text.smk37_audio_irq)
        *(.text.smk37_isr_audio)
        *(.text.smk37_scanner_irq)
        *(.text.smk37_scan_next)
        *(.text.smk37_isr_scanner)
        *(.text.smk37_usb_irq)
        *(.text.smk37_isr_usb)
        *(.text.smk37_usb_body)
        *(.text.sie_rd)
        *(.text.sie_wr)
        *(.text.ua_transmit)
        *(.text.ua_tx_fill)
        *(.text.ua_tx_prepare)
        *(.text.af_render_q15)
        *(.text.as_advance)
        *(.text.af_visual_capture)
        *(.text.track_render_audio)
        *(.text.ab_render)
        *(.text.ab_vector)
        *(.text.voices_busy)
        *(.text.voice_units)
        *(.text.resource_get)
        *(.text.perf_block)
        *(.text.perf_begin)
        *(.text.perf_master)
        *(.text.perf_pre)
        *(.text.perf_mute)
        *(.text.perf_mute_sd)
        *(.text.perf_flanger)
        *(.text.perf_phaser)
        *(.text.perf_harm)
        *(.text.dly_bus)
        *(.text.dly_side_mix)
        *(.text.rev_hall)
        *(.text.rev_room)
        *(.text.rev_spring)
        *(.text.rev_side_mix)
        *(.text.hall_gains)
        *(.text.reverb_prepare)
        *(.text.reverb_idle_clock)
        *(.text.mix_spread)
        *(.text.mix_cache_update)
        *(.text.spread_clear)
        *(.text.master_out)
        *(.text.mod_begin)
        *(.text.mod_voice)
        *(.text.mod_end)
        *(.text.mod_tsrc)
        *(.text.mod_depth)
        *(.text.lfo_sync_inc)
        *(.text.sl_seg)
        *(.text.slicer_track)
        *(.text.track_dist)
        /* CPU1 block handoff and small per-block helpers: no flash fetches from
         * either core while a block renders. */
        *(.text.af_render_job_run)
        *(.text.audio_worker_join)
        *(.text.audio_worker_submit)
        *(.text.bridge_filter_coefficients)
        *(.text.spread_sum)
        *(.text.tune_cache_update)
        *(.text.af_voice_count)
        . = ALIGN(4); _dsp_end = .; } > DSPHOT AT > XIP
    _dsp_load = LOADADDR(.dsp_text);
    /* RAM routines and boot-copied tables precede the generic .text rule so the
     * named read-only tables below are claimed here. Their VMA order (RAMTEXT
     * from 0x01C00000) is unchanged. These are every constant table read by the
     * audio, scanner and USB interrupt paths: flash data otherwise goes through
     * the shared XIP data cache, which both cores contend for while voices
     * render. Contents are identical copies; nothing writes them. */
    .ram_text : { _rt_start = .; KEEP(*(.ram_text*)) . = ALIGN(4); _rt_end = .; } > RAMTEXT AT > XIP
    _rt_load = LOADADDR(.ram_text);
    .dsp_tables : { _dt_start = .; *(.dsp_tables*)
        *(.rodata.smk37_key_contacts)
        *(.rodata.smk37_scan_select)
        *(.rodata.smk37_indicator_order)
        *(.rodata.smk37_button_contacts)
        *(.rodata.af_board_event.buttons)
        *(.rodata.DEV_DESC)
        *(.rodata.STR0)
        *(.rodata.ep0_service.zero2)
        *(.rodata.TP)
        *(.rodata.SINE)
        *(.rodata.PITCH_OCT)
        *(.rodata.TANH_Q15)
        *(.rodata.LFO_INC)
        *(.rodata.PF_SVF)
        *(.rodata.SVF_G)
        *(.rodata.LEVEL_Q12)
        *(.rodata.ENGINES)
        *(.rodata.PH_A)
        *(.rodata.SL_PAT)
        *(.rodata.HL_LEN)
        *(.rodata.LSYNC_DIV)
        *(.rodata.N_LSYNC)
        *(.rodata.mod_param.ID)
        *(.rodata.animate_poc_tables)
        *(.rodata.animate_poc_lengths)
        *(.rodata.animate_poc_a)
        *(.rodata.animate_poc_b)
        *(.rodata.animate_poc_c)
        *(.rodata.animate_poc_d)
        *(.rodata.av_sine_quarter)
        *(.rodata.av_tanh_positive)
        *(.rodata.DIV_DEN)
        *board.o(.rodata.cst*)
        *baseline_bridge.o(.rodata.cst*)
        . = ALIGN(4); _dt_end = .; } > RAMTEXT AT > XIP
    _dt_load = LOADADDR(.dsp_tables);
    .text : { *(.text*) *(.rodata*) . = ALIGN(4); _etext = .; } > XIP
    _text_load = LOADADDR(.text);
    _resource_low_start = ALIGN(_dt_end, 4);
    _resource_low_end = 0x01C07F00;
    .data : { . = ALIGN(4); _data_start = .; *(.data*) *(.sdata*) . = ALIGN(4); _data_end = .; } > RAM AT > XIP
    _data_load = LOADADDR(.data);
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > RAM
    _resource_data_start = _bss_end;
    _resource_data_end = ORIGIN(RAM) + LENGTH(RAM);
    .pool (NOLOAD) : { _pool_start = .; *(.pool*) . = ALIGN(4); _pool_end = .; } > POOL
    .cpu1_stacks (NOLOAD) : {
        . = ALIGN(32); _cpu1_stacks_lo = .;
        KEEP(*(.cpu1_stacks))
        _cpu1_stacks_hi = .; _resource_start = .;
    } > POOL
    _cpu1_ustack_top = _cpu1_stacks_lo + 4096;
    _cpu1_sstack_top = _cpu1_stacks_lo + 8192;
    _resource_end = ORIGIN(POOL) + LENGTH(POOL);
    ASSERT(_resource_end - _resource_start >= 152K, "audio working arena below worst-case budget")
    .noinit (NOLOAD) : { *(.noinit*) . = ALIGN(4); _noinit_end = .; } > NOINIT
    _resource_retained_start = _noinit_end;
    _resource_retained_end = ORIGIN(NOINIT) + LENGTH(NOINIT);
    /* Never part of .bss or startup zeroing: use only after HAL self-test. */
    _resource_cache_start = ORIGIN(CACHE);
    _resource_cache_end = ORIGIN(CACHE) + LENGTH(CACHE);
    ASSERT(_resource_low_start <= _resource_low_end, "RAM code overlaps startup STATUS")
    ASSERT(_resource_data_start <= _resource_data_end, "data overlaps resource pool")
    ASSERT(_resource_retained_start <= _resource_retained_end, "retained state overlaps boot records")
    _guard0 = 0x01C74000; /* DSPHOT ends exactly here */ _ustack_lo = 0x01C74100; _ustack_top = 0x01C7A000;
    _guard1 = 0x01C7A000; _sstack_lo = 0x01C7A100; _sstack_top = 0x01C7C000;
    /DISCARD/ : { *(.comment) *(.note*) }
}
