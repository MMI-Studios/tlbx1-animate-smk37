#!/usr/bin/env python3
"""Build the shared Animate runtime for the physical browser simulator."""
import os,pathlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
OLD=ROOT/'vendor';DONOR=OLD/'melodee'
EXPORTS='af_patch_buffer af_patch_bytes af_patch_pending af_patch_slot af_patch_request_id af_patch_complete af_patch_select af_patch_save af_patch_load af_patch_confirm af_patch_cancel af_patch_dirty af_patch_status af_patch_selection af_patch_confirm_kind af_patch_slots af_patch_notice af_patch_occupied af_init af_render af_output af_button af_pad af_key af_encoder af_fader af_wheel af_midi af_release af_draw af_pad_lights af_button_lights af_visual_data af_visual_bins af_get af_set af_label af_value af_control_get af_control_set af_project_size af_project_buffer af_project_read af_project_begin af_project_write af_project_commit ac_display_data ac_display_count ac_display_stride ac_display_font af_splash_render af_splash_pads'.split()
def main():
 out=ROOT/'build/wasm';out.mkdir(parents=True,exist_ok=True);env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(out/'zig-cache')
 names=['runtime','session','patch','control','display','sequence','project_state','parameter_codec','animate_parameters','animate_math','splash']
 sources=[ROOT/'src'/f'{name}.c' for name in names]+[ROOT/'src/baseline/bridge.c',ROOT/'src/support/music.c',ROOT/'src/visual.c']
 subprocess.run([str(x) for x in [os.environ.get('ZIG', 'zig'),'cc','-target','wasm32-freestanding','-O2','-DAB_FREESTANDING=1','-fno-builtin','-nostdlib','-Wl,--no-entry','-Wl,-z,stack-size=0x10000','-Wl,--initial-memory=0x400000','-Wl,--max-memory=0x400000',*[f'-Wl,--export={x}' for x in EXPORTS],'-I'+str(ROOT/'src'),'-I'+str(DONOR/'web/audio/include'),'-I'+str(DONOR/'firmware/src'),'-I'+str(DONOR/'build/gen'),*sources,'-o',ROOT/'web/animate.wasm']],check=True,env=env)
if __name__=='__main__':main()
