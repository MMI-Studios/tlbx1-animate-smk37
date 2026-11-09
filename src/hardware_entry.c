/* SPDX-License-Identifier: GPL-3.0-only */
/* Animate 982 board services adapted from preserved Melodee SMK HAL entry.
 * No native engine, sequencer, UI or project-store core is included here.
 * Audio runtime and preserved Animate bridge are separate translation units. */
#include <stdint.h>
#define MELODEE_SMK_HARDWARE 1
#include "melodee_smk_options.h"
#define FELUCCA_FLASH 0
#define SMK37_ENABLE_TRS 0
#define FELUCCA_UAC 1 /* dedicated SMK timer for MIDI and audio service */
#include "smk37_unit.h"
#include "smk37_loader_image.h"
#define SMK37_ENABLE_USB_UPDATE 1
#include "fm1_time.h"
#include "fm1_sys.h"
#include "fm1_irq.h"
#if MELODEE_DUAL_CORE
#include "fm1_multicore.h"
#endif
#include "melodee_smk_multicore.h"
#include "fm1_timer.h"
#include "smk37_audio.h"
#include "smk37_lcd_hw.h"
#include "smk37_lights_hw.h"
#include "smk37_panel.h"
#define SMK37_SCAN_CACHED_INDICATORS 1
#include "smk37_scanner.h"
#include "smk37_contacts.h"
#include "smk37_keyscan.h"
#include "smk37_events.h"
#include "smk37_controls.h"
#include "smk37_adc.h"
#include "smk37_boot.h"
#include "smk37_bootguard.h"
#include "smk37_time.h"
#include "smk37_memory.h"
#include "smk37_velocity.h"
#include "smk37_pedal.h"
#include "smk37_analog_controls.h"
static void lcd_sync(void);
static void lcd_blit(uint32_t,uint32_t,uint32_t,uint32_t,const uint16_t*);
static void lcd_fill(uint32_t,uint32_t,uint32_t,uint32_t,uint16_t);
#define FS 44100u
#define CTL 32u
#define HALF_FRAMES 128u
#define FM1_TICKS_PER_US 24u
#define MELODEE_USB_AUDIO 1
#define MELODEE_OTA 1
#define MELODEE_CDC 0
#define MELODEE_VERSION "Animate 982"
#define MELODEE_ID "SMK-37_982"
#include "libc.c"
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#include "usb.c"
#include "control.h"
#include "display.h"
#include "splash.h"
#include "runtime.h"
/* Parallel voices: the Animate kernel pairs voices like Melodee's Prophet.
 * CPU0 renders the audio block and submits every other voice to CPU1 through
 * the donor mailbox, joining before it touches that voice's output. */
void ab_worker_hooks(int (*submit)(void (*)(void *), void *), int (*join)(void));
uint32_t ab_active(void);
typedef ac_draw_command tlbx_draw_command_t;
#define TLBX_DISPLAY_H /* shared 56-byte command ABI; do not import old runtime */
#include "../vendor/tlbx-display/hardware_display.h"
static uint8_t smk_sustain;
static uint32_t af_cpu_q8;
static volatile uint32_t af_scan_ticks,af_usb_ticks,af_scan_irqs;
static uint32_t af_render_ticks_avg,af_irq_ticks_avg,af_other_ticks_avg;
#include "overload_capture.h"
static af_overload_capture af_overload;
/* Snapshot timer/counters together: a nested IRQ between reads would otherwise
 * be charged to the wrong stage. Called only from the nesting-enabled audio ISR. */
static void af_audio_clock(uint32_t*t,uint32_t*irq){fm1_irq_off();*t=fm1_ticks();*irq=af_scan_ticks+af_usb_ticks;fm1_irq_on();}
static struct {uint32_t late,max_us;} melodee_dbg;
static void af_board_release(void){smk_sustain=0;af_release();}
static void af_board_event(unsigned kind,unsigned id,int value){
 static const unsigned buttons[18]={AC_PARA,AC_FX,AC_LEFT,AC_RIGHT,AC_OCT_DOWN,AC_OCT_UP,AC_ARP,AC_REPEAT,AC_SCALE,AC_CHORD,AC_GLOBE,AC_SHIFT,AC_PATCH,AC_PARA,AC_FX,AC_SEQ,AC_PLAY,AC_REC};
 if(kind==0&&id<18)af_button(buttons[id],value,fm1_ms);
 else if(kind==1&&id<8)af_encoder(id,value);
 else if(kind==2&&id<37)af_key(48+id,value);
 else if(kind==3&&id<16)af_pad(id,value);
 else if(kind==4&&id<4)af_fader(id,(unsigned)value*127/1023);
 else if(kind==5&&id<2)af_wheel(id,value);
 else if(kind==6){smk_sustain=!!value;af_midi(0xb0,64,value?127:0);}
}
static void af_board_midi(void){
 if(midi_in_overflow){fm1_irq_off();mi_r=mi_w;midi_in_overflow=0;fm1_irq_on();af_board_release();}
 for(unsigned n=0;n<MQ&&mi_r!=mi_w;n++){RING_PUBLISH();uint32_t p=midi_in_q[mi_r%MQ];RING_PUBLISH();mi_r++;af_midi((p>>8)&255,(p>>16)&127,(p>>24)&127);}
}
static void af_board_draw(void){
#if MELODEE_DUAL_CORE
 fm1_irq_off();unsigned online=audio_worker_online,jobs=audio_worker.jobs,timeouts=audio_worker.timeouts;fm1_irq_on();
 af_hardware_worker(online,jobs,timeouts);
#else
 af_hardware_worker(0,0,0);
#endif
 uint32_t capture[8];fm1_irq_off();memcpy(capture,af_overload.value,sizeof capture);fm1_irq_on();af_hardware_overload(capture);
 static unsigned visual_at,was_visual;
 unsigned visual=af_control_get(0)==AC_VIEW_VIS;
 if(visual&&was_visual&&fm1_ms-visual_at<100u)return;
 if(visual)visual_at=fm1_ms;
 was_visual=visual;
 af_hardware_metrics(af_cpu_q8*100u/256u,melodee_dbg.late);
 af_hardware_profile(af_render_ticks_avg/FM1_TICKS_PER_US,af_irq_ticks_avg/FM1_TICKS_PER_US,af_other_ticks_avg/FM1_TICKS_PER_US);af_draw();
 tlbx_hardware_display(ac_display_data(),ac_display_count(),ac_display_font(),af_visual_data(),af_visual_bins(),lcd_blit);
}
#include "audio_budget_monitor.h"


/* TIMER5 owns USB polling; never enter the SIE again from the editor loop. */
static uint32_t ota_now_ms(void){return fm1_ticks()/24000u;}
static void ota_idle(void){fm1_wdt_feed();}
static int af_store_sysex(const uint8_t*,unsigned);
static void af_usb_service(void){
 static const uint8_t q[]={0,0x32,0x45,0,0,0,0x40,0x7f};const uint8_t*p;uint32_t n;
 if(!ota_frame_get(&p,&n))return;
 if(af_store_sysex(p,n))return;
 if(n==sizeof q&&!memcmp(p,q,n)){ota_frame_done();ota_wire_send(smk37_update_identity_wire,sizeof smk37_update_identity_wire);return;}ota_frame_done();
}

volatile smk37_bootguard_t smk37_boot_state __attribute__((section(".noinit")));

static int32_t smk37_dma[HALF_FRAMES * 4u] __attribute__((aligned(4),section(".pool")));
#include "storage_hw.h"
#if FELUCCA_FLASH
static void audio_silence(void){memset(smk37_dma,0,sizeof smk37_dma);}
#endif
static volatile uint32_t smk37_rendered, smk37_late, smk37_scan_frames;
static volatile uint32_t smk37_audio_irqs;
static volatile uint32_t smk37_frame_ticks;
static uint32_t smk37_frame_last;
static smk37_keyscan_t smk37_keys[37];
static smk37_controls_t smk37_controls;
/* Aligned single-word diagnostic snapshots; no synth mutation in scan IRQ. */
static volatile uint32_t smk37_last_button, smk37_last_encoder;
static smk37_adc_t smk37_adc;
/* Keep contact duration for measured-time generic velocity curves. Never call
 * synthesis from the scanner ISR or relabel generic response as factory trim. */
static smk37_events_t smk37_key_events;
/* Separate queue: losing an encoder update cannot lose a key release. */
static smk37_events_t smk37_encoder_events;
static smk37_events_t smk37_button_events;
static smk37_events_t smk37_pad_events;
static smk37_pad_state_t smk37_pad_state[16];
static smk37_pedal_t smk37_pedal_input;
static volatile uint32_t smk37_pedal_latest; /* stable level, applied only by audio consumer */
static volatile uint32_t smk37_analog_latest[6]; /* bit 16 valid, low bits raw */
static volatile uint8_t smk37_analog_failed;
static smk37_fader_filter_t smk37_fader_filters[4];
static void smk37_scanner_body(void)
{
    unsigned key;
    if (!(SMK37_SCAN_CON & 0x8000u)) return;
    unsigned analog_index = smk37_adc.scan.index;
    int analog_result = smk37_adc_spi_done(&smk37_adc);
    if (analog_result < 0) smk37_analog_failed = 1;
    if (analog_result > 0) {
        uint16_t raw = smk37_adc.scan.raw[analog_index];
        if(analog_index==22){
            int pedal=smk37_pedal_sample(&smk37_pedal_input,raw);
            if(pedal>=0)smk37_pedal_latest=(uint32_t)pedal;
        }
        if (analog_index < 16) {
            static const uint8_t pad_for_adc[16]={1,2,3,0,4,7,5,6,9,10,11,8,12,15,13,14};
            unsigned i=pad_for_adc[analog_index];
            smk37_pad_change_t change = smk37_pad_sample(&smk37_pad_state[i], raw);
            if (change.velocity) smk37_events_push(&smk37_pad_events, i, change.velocity);
            if (change.release) smk37_events_push(&smk37_pad_events, i, 0);
        }
        if(analog_index>=18 && analog_index<=21){
            unsigned i=(analog_index-17u)&3u;
            smk37_analog_latest[i] = 0x10000u | smk37_fader_filter(&smk37_fader_filters[i],raw);
        }
        if (analog_index==16 || analog_index==17)
            smk37_analog_latest[analog_index-12] = 0x10000u | raw;
    }
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    if (!smk37_scan_tick()) return;
    uint32_t now=fm1_ticks(), elapsed=now-smk37_frame_last;
    smk37_frame_last=now;
    if(smk37_scan_frames && elapsed && elapsed<240000u)
        smk37_frame_ticks=smk37_frame_ticks ? (smk37_frame_ticks*7u+elapsed)/8u : elapsed;
    ++smk37_scan_frames;

    smk37_control_changes_t controls = smk37_controls_tick(&smk37_controls, smk37_scan_frame);
    for (unsigned i = 0; i < SMK37_BUTTON_COUNT; ++i) {
        if (controls.pressed & (1u << i)) {
            smk37_last_button = ((i+1u) << 8) | 1u;
            smk37_events_push(&smk37_button_events,i,1);
        }
        if (controls.released & (1u << i)) {smk37_last_button = (i+1u) << 8;smk37_events_push(&smk37_button_events,i,0);}
    }
    for (unsigned i = 0; i < SMK37_ENCODER_COUNT; ++i)
        if (controls.encoder[i]) {
            smk37_last_encoder = ((i+1u) << 8) | (controls.encoder[i] > 0 ? 1u : 2u);
            smk37_events_push(&smk37_encoder_events, i, controls.encoder[i] > 0 ? 1u : 2u);
        }
    for (key = 0; key < 37; ++key) {
        uint32_t duration;
        unsigned raw = smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][0]) |
                       smk37_contact_closed(smk37_scan_frame, smk37_key_contacts[key][1]) << 1;
        if (!smk37_keyscan_tick(&smk37_keys[key], raw, &duration)) continue;
        (void)smk37_events_push(&smk37_key_events, key, duration);
    }
}

void smk37_scanner_irq(void){uint32_t begin=fm1_ticks();smk37_scanner_body();af_scan_ticks+=fm1_ticks()-begin;++af_scan_irqs;}
static void smk37_consume_keys(void){
    smk37_event_t e;
    if(smk37_key_events.overflow||smk37_pad_events.overflow||smk37_button_events.overflow){
        fm1_irq_off();smk37_events_discard(&smk37_key_events);smk37_events_discard(&smk37_pad_events);smk37_events_discard(&smk37_button_events);fm1_irq_on();af_board_release();}
    for(unsigned n=0;n<18&&smk37_events_pop(&smk37_button_events,&e);n++)af_board_event(0,e.key,e.duration);
    for(unsigned n=0;n<SMK37_EVENTS&&smk37_events_pop(&smk37_pad_events,&e);n++)af_board_event(3,e.key,e.duration);
    for(unsigned n=0;n<SMK37_EVENTS&&smk37_events_pop(&smk37_key_events,&e);n++)af_board_event(2,e.key,smk37_key_velocity(e.duration,smk37_frame_ticks,0));
    static int8_t phases[8];
    if(smk37_encoder_events.overflow){fm1_irq_off();smk37_events_discard(&smk37_encoder_events);fm1_irq_on();memset(phases,0,sizeof phases);}
    if(smk37_events_pop(&smk37_encoder_events,&e)&&e.key<8){phases[e.key]+=e.duration==1?1:-1;if(phases[e.key]>=4||phases[e.key]<=-4){af_board_event(1,e.key,phases[e.key]>0?1:-1);phases[e.key]=0;}}
    static uint16_t previous[6];static uint8_t valid;
    if(!smk37_analog_failed)for(unsigned i=0;i<6;i++){uint32_t v=smk37_analog_latest[i];if(!(v&0x10000))continue;unsigned raw=v&65535;if(!(valid&(1u<<i))){previous[i]=raw;valid|=1u<<i;continue;}if(!smk37_analog_moved(i,raw,previous[i]))continue;previous[i]=raw;if(i<4)af_board_event(4,i,raw);else af_board_event(5,i-4,i==4?smk37_pitch_value(raw):smk37_mod_value(raw));}
    if(smk_sustain!=(smk37_analog_failed?0:smk37_pedal_latest))af_board_event(6,0,smk37_analog_failed?0:smk37_pedal_latest);
}
static int smk37_panel_command(uint8_t c) { return smk37_lcd_command(c, 4000000u); }
static int smk37_panel_data(const void *p, uint32_t n) { return smk37_lcd_data(p, n, 4000000u); }
static const smk37_panel_io_t smk37_panel_io = {
    smk37_panel_command, smk37_panel_data, fm1_delay_ms
};
static const smk37_audio_hooks_t smk37_audio_hooks = {
    fm1_delay_ms, fm1_irq_attach
};
static uint8_t smk37_display_ok;
static uint16_t swap16(uint32_t c){return(uint16_t)((c>>8)|((c&255)<<8));}
static void lcd_sync(void) {} /* SMK transport completes DMA synchronously. */
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *pixels)
{
    if (!smk37_display_ok) return;
    if (!smk37_panel_window(&smk37_panel_io, x, y, w, h) ||
        !smk37_panel_data(pixels, w*h*2u)) smk37_display_ok = 0;
}

#undef lcd_fill
static void lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){uint16_t row[240];if(w>240||x+w>240||y+h>240)return;for(unsigned i=0;i<w;i++)row[i]=swap16(c);for(unsigned i=0;i<h;i++)lcd_blit(x,y+i,w,1,row);}
/* Boot splash (main loop only). Frame 0 is drawn during boot before the
 * scanner/audio start; later frames redraw only changed regions every 40 ms
 * while audio, USB and the keyboard already run. Any button or encoder ends
 * it early. The display cache is then invalidated so the UI repaints fully. */
static uint8_t af_splash_on=1;
static unsigned af_splash_frame;
static uint16_t af_splash_pixels[240*4];
static void af_splash_paint(unsigned frame){
    unsigned rects[AF_SPLASH_RECTS][4],n=af_splash_regions(frame,rects);
    for(unsigned k=0;k<n;k++){unsigned x=rects[k][0],y=rects[k][1],w=rects[k][2],h=rects[k][3];
        for(unsigned row=0;row<h;row+=4){unsigned rows=h-row<4?h-row:4;
            for(unsigned r=0;r<rows;r++){uint16_t*line=af_splash_pixels+r*w;af_splash_row(frame,y+row+r,x,w,line);for(unsigned i=0;i<w;i++)line[i]=swap16(line[i]);}
            lcd_blit(x,y+row,w,rows,af_splash_pixels);}}
}
static void af_splash_step(void){
    if(smk37_last_button||smk37_last_encoder||++af_splash_frame>=AF_SPLASH_FRAMES){af_splash_on=0;tlbx_hardware_display_reset();return;}
    af_splash_paint(af_splash_frame);
}
static uint8_t smk37_light_buffer[SMK37_LIGHT_BYTES] __attribute__((aligned(4)));
static void smk_lcd_power(unsigned state){if(smk37_display_ok)smk37_lcd_command(state==1?0x29:0x28,4000000u);}
static void smk37_light_service(void){
    static uint32_t previous=UINT32_MAX;
    static uint8_t failed;
    if(failed)return;
    
    fm1_irq_off();
    smk37_scan_indicators_set(af_button_lights());
    uint16_t pads=af_splash_on?af_splash_pads(af_splash_frame):(uint16_t)af_pad_lights(fm1_ms);
    unsigned level=4,color=2;

    fm1_irq_on();
    /* Always show a physical hit when lights are enabled. Do not translate
     * FM-1 piano-key or sequencer lighting into unrelated physical pads. */
    uint32_t state=pads|(level<<16)|(color<<24);
    if(state==previous)return;
    smk37_light_encode_mask(smk37_light_buffer,pads,color,level);
    if(smk37_lights_send(smk37_light_buffer,4000000u))previous=state;
    else {failed=1;fm1_irq_off();smk37_scan_indicators_set(0);fm1_irq_on();}
}

void smk37_audio_irq(void)
{
    ++smk37_audio_irqs;
    uint8_t pending = smk37_audio_pending();
    smk37_audio_ack_aux(pending);
    if (pending & SMK37_AUDIO_HALF) {
        uint32_t begin,irq_begin,render_ticks=0,scan_begin,scan_irqs_begin;
        fm1_irq_off();begin=fm1_ticks();scan_begin=af_scan_ticks;irq_begin=scan_begin+af_usb_ticks;scan_irqs_begin=af_scan_irqs;fm1_irq_on();
        unsigned half = smk37_audio_free_half(), b, i;
        int32_t *out = smk37_dma + half * HALF_FRAMES * 2u;
        if(af_storage_busy){memset(out,0,HALF_FRAMES*2u*sizeof *out);smk37_audio_ack_half();++smk37_rendered;return;}
        smk37_multicore_checkpoint();
        smk37_audio_budget_begin();
        smk37_consume_keys();af_board_midi();
        unsigned voices_before=af_voice_count();

#if FELUCCA_UAC

#endif
        for (b = 0; b < HALF_FRAMES; b += CTL) {
            uint32_t start,stop,irq_start,irq_stop;
            af_audio_clock(&start,&irq_start);
            af_render_q15(out + b * 2u, CTL);
            af_audio_clock(&stop,&irq_stop);
            uint32_t span=stop-start,interrupts=irq_stop-irq_start;
            render_ticks+=span>interrupts?span-interrupts:0;
            /* The complete runtime block has joined before USB or more events. */
            smk37_multicore_checkpoint();
#if MELODEE_USB_AUDIO
            if(usb.up&&usb.config&&!usb.suspended){
                int32_t capture[CTL*4];
                for(unsigned j=0;j<CTL;j++){capture[j*4]=out[(b+j)*2];capture[j*4+1]=out[(b+j)*2+1];capture[j*4+2]=capture[j*4+3]=0;}
                /* Prepare samples with scanning/USB interrupts enabled. Only
                 * the shared queue operation needs the critical section.
                 * Recheck connection state after a possible USB interrupt. */
                fm1_irq_off();
                if(usb.up&&usb.config&&!usb.suspended)ua_audio(out+b*2u,capture,CTL,4096);
                fm1_irq_on();
            }
#endif


#if FELUCCA_UAC

#endif



        }
        for (i = 0; i < HALF_FRAMES * 2u; ++i) out[i] *= 128;
        smk37_audio_ack_half();
        ++smk37_rendered;
        if (smk37_audio_free_half() != half) {++smk37_late;++melodee_dbg.late;}
        uint32_t end,irq_end,scan_end,scan_irqs_end;
        fm1_irq_off();end=fm1_ticks();scan_end=af_scan_ticks;irq_end=scan_end+af_usb_ticks;scan_irqs_end=af_scan_irqs;fm1_irq_on();
        uint32_t elapsed=end-begin,interrupts=irq_end-irq_begin;
        uint32_t other=elapsed>render_ticks+interrupts?elapsed-render_ticks-interrupts:0;
        af_render_ticks_avg=(af_render_ticks_avg*31u+render_ticks)/32u;
        af_irq_ticks_avg=(af_irq_ticks_avg*31u+interrupts)/32u;
        af_other_ticks_avg=(af_other_ticks_avg*31u+other)/32u;
        smk37_audio_budget_end(elapsed);
        uint32_t scan_ticks=scan_end-scan_begin;
        af_overload_record(&af_overload,smk37_shed_req,elapsed,render_ticks,
          scan_ticks,interrupts>scan_ticks?interrupts-scan_ticks:0,voices_before,
          (HALF_FRAMES*1000000u/FS)*85u/100u*FM1_TICKS_PER_US,FM1_TICKS_PER_US,scan_irqs_end-scan_irqs_begin);
        unsigned us=elapsed/FM1_TICKS_PER_US;
        if(us>melodee_dbg.max_us)melodee_dbg.max_us=us;
        af_cpu_q8=(af_cpu_q8*31u+us*256u/(HALF_FRAMES*1000000u/FS))/32u;
    }
}
/* UAC needs one packet each millisecond even while the LCD transfers.
 * A single 2 kHz TIMER5 consumer owns every SIE access. Priority 4 allows it
 * to preempt rendering (3); SMK contact scanning keeps priority 5. The main
 * loop never polls USB concurrently with this consumer. Retry stays in main:
 * usb.up is false throughout its 25 ms reset, so usb_poll touches no SIE. */
#if FELUCCA_UAC
#define smk37_usb_irq smk37_usb_body
#include "melodee_smk_usb_irq.h"
#undef smk37_usb_irq
void smk37_usb_irq(void){uint32_t start,stop,scan,scanned;af_audio_clock(&start,&scan);smk37_usb_body();af_audio_clock(&stop,&scanned);uint32_t span=stop-start;scanned-=scan;af_usb_ticks+=span>scanned?span-scanned:0;}
extern void smk37_isr_usb(void);
static void smk37_usb_timer_start(void){
    fm1_timer5_start(smk37_isr_usb,4);
    FM1_T5_PRD=3000u; /* OSC/4 = 6 MHz, 2 kHz */
}
static void smk37_usb_timer_stop(void){FM1_T5_CON=0x4000u;
#if SMK37_ENABLE_TRS
    if(smk37_trs_running)smk37_uart_tx_stop();
#endif
}
#endif
static void smk37_enter_usb_update(void){
    smk37_bootguard_update(&smk37_boot_state);
    fm1_irq_off();
    smk37_multicore_stop(0); /* stop before the RAM loader overwrites application state */
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();smk37_scan_stop();smk37_adc_stop(&smk37_adc);
    smk37_audio_pc_out(6,0);
    usb_detach();fm1_delay_ms(100);
    volatile uint8_t *dst=(volatile uint8_t *)0x01c0a800u;
    for(unsigned i=0;i<sizeof smk37_loader_image;++i)dst[i]=smk37_loader_image[i];
    __asm__ volatile("csync" ::: "memory");
    void (*volatile enter)(void)=(void (*)(void))0x01c0a800u;
    enter();for(;;);
}
extern void smk37_isr_audio(void);
extern void smk37_isr_scanner(void);

static void fm1_fault(const fm1_crash_t *record)
{
    (void)record;
    smk37_multicore_stop(1);
#if FELUCCA_UAC
    smk37_usb_timer_stop();
#endif
    smk37_audio_stop();
    smk37_scan_stop();
    smk37_adc_stop(&smk37_adc);
    /* Watchdog reset, without attempting an unverified flash repair. */
    for (;;) ;
}

extern uint32_t _bss_start[], _bss_end[], _pool_start[], _pool_end[];
extern uint32_t _data_start[], _data_end[], _data_load[];
extern uint32_t _rt_start[], _rt_end[], _rt_load[];
void smk37_cstart(const uint32_t *boot_parameters)
{
    uint32_t last, remainder = 0, status_at = 0, lights_at = 0;
    uint32_t health_scan = 0, health_audio = 0, health_progress_at = 0;
    uint32_t saved_reset, saved_p33_timeouts;
    uint8_t saved_p3_reset, saved_watchdog;
    /* Shared P33 reset flags: bit 0 is power-on. Snapshot before replacing BSS.
     * Retention and fallback entry are still hardware verification gates. */
    fm1_core1_stop(); /* hold a prior worker before even the reset snapshot touches RAM */
    fm1_p33_timeouts = 0; /* P33 helpers run before BSS initialization. */
    fm1_reset_reason();
    saved_reset = fm1_boot.rst_src;
    saved_p3_reset = fm1_boot.p3_rst;
    saved_watchdog = fm1_boot.wdt_con;
    fm1_wdt_arm(0x0d);
    smk37_multicore_prepare(saved_p3_reset & 1u);
    if (smk37_bootguard_begin(&smk37_boot_state, fm1_boot.p3_rst & 1u))
        fm1_enter_uboot();
    smk37_boot_capture(boot_parameters);
    smk37_boot_state.stage = SMK37_BOOT_MEMORY;
    saved_p33_timeouts = fm1_p33_timeouts;
    smk37_memory_zero(_bss_start, _bss_end);
    smk37_memory_zero(_pool_start, _pool_end);
    smk37_memory_copy(_data_start, _data_end, _data_load);
    smk37_memory_copy(_rt_start, _rt_end, _rt_load);
    extern uint32_t _dsp_start[],_dsp_end[],_dsp_load[],_dt_start[],_dt_end[],_dt_load[];
    smk37_memory_copy(_dsp_start,_dsp_end,_dsp_load);smk37_memory_copy(_dt_start,_dt_end,_dt_load);
    __asm__ volatile("csync" ::: "memory");
    /* Reset diagnostics live in BSS; preserve the pre-watchdog snapshot. */
    fm1_boot.rst_src = saved_reset;
    fm1_boot.p3_rst = saved_p3_reset;
    fm1_boot.wdt_con = saved_watchdog;
    fm1_p33_timeouts = saved_p33_timeouts;
    smk37_time_init();
    fm1_irq_init();
    fm1_mailbox_clear();
    af_init();
#if MELODEE_DUAL_CORE
    ab_worker_hooks(audio_worker_submit, audio_worker_join);
#endif
    /* Playable startup. Diagnostics remain reachable with the page encoder. */

    smk37_boot_state.stage = SMK37_BOOT_USB;
    
    usb_start();
    smk37_boot_state.stage = SMK37_BOOT_DISPLAY;
    smk37_lcd_hw_init(4);
    /* Failure disables the display transport; USB/audio can still run. */
    smk37_display_ok = (uint8_t)smk37_panel_init(&smk37_panel_io);
    /* Branded first frame; boot continues immediately (no added delay). */
    af_splash_paint(0);
    smk37_boot_state.stage = SMK37_BOOT_SCANNER;
    smk37_adc_init(&smk37_adc);
    /* Stock PC10 idle is low/output. Keep it defined between RGB frames. */
    SMK37_LIGHT_REG(0x50080u) &= ~(1u<<10);
    SMK37_LIGHT_REG(0x50088u) &= ~(1u<<10);
    smk37_scan_set_analog(smk37_analog_mask(&smk37_adc.scan));
    smk37_scan_init(fm1_irq_attach, smk37_isr_scanner);
    smk37_boot_state.stage = SMK37_BOOT_AUDIO;
    fm1_irq_off();
    smk37_multicore_start(smk37_boot_state.failures);
    (void)smk37_audio_init(smk37_dma, HALF_FRAMES * 2u,
                          smk37_isr_audio, 3, &smk37_audio_hooks);
    last = fm1_ticks();
#if FELUCCA_UAC
    smk37_usb_timer_start();
#endif
    fm1_irq_enable_all();
    af_store_probe(); /* Reads only; never claims or erases storage on boot. */
    smk37_boot_state.stage = SMK37_BOOT_RUNNING;
#if FELUCCA_FLASH
    /* Probe/read only after interrupts are initialized. No erase or format on boot. */
    
#endif
    for (;;) {
        uint32_t now = fm1_ticks();
        remainder += now - last;
        last = now;
        while (remainder >= 24000u) { ++fm1_ms; remainder -= 24000u; }
        fm1_wdt_feed();
#if !FELUCCA_UAC
        usb_poll();
#endif
        usb_retry(fm1_ms);
#if MELODEE_USB_AUDIO
        (void)ua_off_apply(fm1_ms);
#endif
        af_usb_service();
        af_store_service();
        af_setting_service(fm1_ms);
#if SMK37_ENABLE_USB_UPDATE
        if(usb.ota_req){usb.ota_req=0;smk37_enter_usb_update();}
#endif

#if SMK37_ENABLE_TRS
        smk37_trs_configure();
#endif

#if SMK37_ENABLE_SAMPLES
        sloop_samples_service();
#endif
        if (smk37_scan_frames != health_scan && smk37_rendered != health_audio) {
            health_scan = smk37_scan_frames;
            health_audio = smk37_rendered;
            health_progress_at = fm1_ms;
        }
        if (smk37_boot_state.pending && fm1_ms >= 30000u &&
            health_scan >= 100u && health_audio >= 100u && fm1_ms - health_progress_at < 1000u)
            smk37_bootguard_healthy(&smk37_boot_state);
        if (fm1_ms - lights_at >= 25u) { lights_at=fm1_ms; smk37_light_service(); }
        if (fm1_ms - status_at >= 40u) {
            status_at = fm1_ms;
            if (af_splash_on) af_splash_step();
            if (!af_splash_on) af_board_draw();
        }
        /* Host-requested return uses the mailbox/reset sequence also found
         * in Pro016. It is not a verified forced recovery route. */
        if (usb.uboot_req) {
            smk37_bootguard_update(&smk37_boot_state);
            fm1_irq_off();
            smk37_multicore_stop(0);
#if FELUCCA_UAC
            smk37_usb_timer_stop();
#endif
            smk37_audio_stop();
            smk37_scan_stop();
            smk37_adc_stop(&smk37_adc);
            usb_detach();
            fm1_delay_ms(30);
            fm1_enter_uboot();
        }
    }
}


