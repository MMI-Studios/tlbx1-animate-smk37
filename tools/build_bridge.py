#!/usr/bin/env python3
"""Portable preserved-host comparison and WASM build; no board/flash action."""
import os,pathlib,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
OLD=ROOT/'vendor'
DONOR=OLD/'melodee'
OUT=ROOT/'build/bridge'
def run(args,**kw):return subprocess.run([str(x) for x in args],check=True,**kw)
def main():
 OUT.mkdir(parents=True,exist_ok=True)
 inc=['-I'+str(DONOR/'firmware/src'),'-I'+str(DONOR/'build/gen')]
 if len(sys.argv)>1 and sys.argv[1]=='wasm':
  names='ab_init ab_preset ab_param ab_slot ab_param_get ab_slot_get ab_slot_parameter ab_slot_parameter_get ab_slot_parameter_mask ab_slot_parameter_mask_set ab_voice_parameter ab_voice_parameters_clear ab_note_on ab_owned_note_on ab_note_off ab_polyphony ab_render ab_output ab_active ab_tempo ab_master ab_release_all ab_vector ab_vector_get ab_vector_lfo ab_vector_lfo_get ab_vector_lfo_mask ab_vector_lfo_mask_set ab_voice_pitch ab_pitch_bend ab_modwheel ab_panic ab_shed_oldest'.split()
  env=os.environ.copy();env['ZIG_GLOBAL_CACHE_DIR']=str(OUT/'zig-cache')
  run([os.environ.get('ZIG', 'zig'),'cc','-target','wasm32-freestanding','-O2','-DAB_FREESTANDING=1','-fno-builtin','-nostdlib','-Wl,--no-entry','-Wl,-z,stack-size=0x10000','-Wl,--initial-memory=0x200000','-Wl,--max-memory=0x200000',*[f'-Wl,--export={x}' for x in names],*inc,ROOT/'src/baseline/bridge.c',ROOT/'src/animate_math.c','-o',OUT/'animate-bridge.wasm'],env=env)
  return
 for name in ['candidate','reference','ownership']:
  run(['cc','-std=gnu11','-O2','-fwrapv',*inc,ROOT/f'tests/bridge_{name}.c',ROOT/'src/animate_math.c','-lm','-o',OUT/name])
 for preset in range(6):
  a=run([OUT/'reference',preset],capture_output=True).stdout
  b=run([OUT/'candidate',preset],capture_output=True).stdout
  # The unified block-rate kernel is near-identical, not bit-identical, to the
  # preserved donor (owner decision 2026-11-07); require matching signal energy.
  ra=int(a.split()[2]);rb=int(b.split()[2])
  if abs(rb-ra)>ra*0.01:raise RuntimeError(f'Preset {preset} energy differs >1%: {a!r} vs {b!r}')
  print(f'Preset {preset} donor/unified energy within {abs(rb-ra)/ra*100:.3f}%: candidate hash {b.split()[1].decode()}')
 run([OUT/'ownership'])
if __name__=='__main__':main()
