#!/usr/bin/env python3
"""Native full-runtime assertions and exact preserved-host audio comparisons."""
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
DONOR = ROOT / 'vendor' / 'melodee'
OUT = ROOT / 'build' / 'host'
OUT.mkdir(parents=True, exist_ok=True)
exe = OUT / 'runtime-test'
host = OUT / 'host-voice-test'
subprocess.run(['cc', '-std=gnu11', '-O2', '-fwrapv', '-I'+str(ROOT/'src'),
                '-I'+str(DONOR/'firmware/src'), '-I'+str(DONOR/'build/gen'),
                str(ROOT/'tests/host_voice_test.c'), str(ROOT/'src/animate_math.c'),
                '-o', str(host)], check=True)
subprocess.run([str(host)], check=True)
for hardware in [False, True]:
    visual = OUT / ('visual-hardware-test' if hardware else 'visual-simulator-test')
    subprocess.run(['cc', '-std=gnu11', '-O2', *(['-DAF_HARDWARE=1'] if hardware else []),
                    str(ROOT/'src/visual.c'), str(ROOT/'tests/visual_test.c'),
                    '-o', str(visual)], check=True)
    subprocess.run([str(visual)], check=True)
sources = ['runtime', 'session', 'patch', 'control', 'display', 'sequence', 'project_state',
           'parameter_codec', 'animate_parameters', 'animate_math', 'baseline/bridge',
           'support/music', 'visual']
worker = OUT / 'render-worker-test'
subprocess.run(['cc', '-std=gnu11', '-O2', '-fwrapv', '-pthread',
                '-I'+str(ROOT/'src'), '-I'+str(DONOR/'firmware/src'), '-I'+str(DONOR/'build/gen'),
                *[str(ROOT/'src'/(s+'.c')) for s in sources],
                str(ROOT/'tests/render_worker_test.c'), '-o', str(worker)], check=True)
assert subprocess.check_output([str(worker),'0']) == subprocess.check_output([str(worker),'1'])
print('PASS serial/parallel-voice runtime PCM identical: eight notes, source edits, filters, pan and transport')
for suite in ['runtime', 'patch']:
    target = OUT / (suite+'-test')
    subprocess.run(['cc', '-std=gnu11', '-O2', '-fwrapv', '-I'+str(ROOT/'src'),
                    '-I'+str(DONOR/'firmware/src'), '-I'+str(DONOR/'build/gen'),
                    *[str(ROOT/'src'/(s+'.c')) for s in sources],
                    str(ROOT/'tests'/(suite+'_test.c')), '-o', str(target)], check=True)
    subprocess.run([target], check=True)
for preset in range(6):
    reference = subprocess.check_output([exe, str(preset), 'r'])
    actual = subprocess.check_output([exe, str(preset)])
    restored = subprocess.check_output([exe, str(preset), 's'])
    patch = subprocess.check_output([exe, str(preset), 'p'])
    assert reference == actual == restored == patch, (preset, reference, actual, restored, patch)
    print('Exact runtime / preserved bridge / restored project / patch:', actual.decode().strip())
baseline = subprocess.check_output([exe, '0'])
for knob in range(8):
    assert subprocess.check_output([exe, '0', 'k'+str(knob)]) != baseline
print('All eight slot volume/pan knobs change rendered audio.')
subprocess.run([sys.executable, str(ROOT/'tools/kernel_compare.py'), '--check'], check=True)
