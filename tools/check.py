#!/usr/bin/env python3
"""Host-module checks only; this does not produce an installable firmware."""
import argparse
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
SUITES = {
    'parameters': ['animate_parameters'],
    'support': ['support/output', 'support/music'],
    'math': ['animate_math'],
    'control': ['control'],
    'display': ['control', 'display', 'animate_parameters'],
    'splash': ['splash', 'control', 'display', 'animate_parameters'],
    'state': ['project_state', 'parameter_codec', 'sequence', 'animate_parameters'],
    'random': ['project_state', 'parameter_codec', 'sequence', 'animate_parameters'],
}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('suite', nargs='*', choices=list(SUITES))
    p.add_argument('--sanitize', action='store_true')
    args = p.parse_args()
    output = ROOT / 'build' / 'host'
    output.mkdir(parents=True, exist_ok=True)
    names = args.suite or [n for n in SUITES if (ROOT / 'tests' / f'{n}_test.c').exists()]
    for name in names:
        exe = output / (name + '-test')
        flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if args.sanitize else ['-O2']
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', *flags,
                        '-I' + str(ROOT / 'src'),
                        *[str(ROOT / 'src' / (s + '.c')) for s in SUITES[name]],
                        str(ROOT / 'tests' / (name + '_test.c')), '-lm', '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    print(f'{len(names)} host module suites passed. No firmware image was built.')

if __name__ == '__main__':
    main()
