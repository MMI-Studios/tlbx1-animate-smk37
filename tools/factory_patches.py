#!/usr/bin/env python3
"""Sixteen factory patches with sequences, compiled into the firmware.

Writes src/generated/factory_patches.h and web/factory-names.js.

Each patch starts from the VECTOR factory sound (preset 0), loads four bank
waves into the wave registers, applies the listed control changes, and
replaces the whole ten-lane sequence (steps and parameter locks). Notes are
MIDI numbers (C4 = 60); the scale stays chromatic so they play as written.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Slot parameters (animate_parameter_id)
SRC, COARSE, FINE, LEVEL, PAN, WTAMT, WTSHAPE, WTRATE, WTSYNC, WTDIV, START, LSTART, LEND, FTYPE, CUT, RES, A, D, S, R = range(20)
LP24, LP12, HP, BP, MOOG = range(5)
SINE, TRI, SQUARE, SAW, SH = range(5)
DIV = {'1/16': 0, '1/8': 1, '1/4': 2, '1/3': 3, '1/2': 4, '1': 5, '2': 6, '4': 7}
FX = {'dist': 0, 'chorus': 1, 'delay': 2, 'reverb': 3, 'level': 4, 'pan': 5, 'glide': 6}

NOTE = {'C': 0, 'C#': 1, 'DB': 1, 'D': 2, 'D#': 3, 'EB': 3, 'E': 4, 'F': 5, 'F#': 6, 'GB': 6, 'G': 7,
        'G#': 8, 'AB': 8, 'A': 9, 'A#': 10, 'BB': 10, 'B': 11}


def n(name):
    m = re.fullmatch(r'([A-G][#B]?)(-?\d)', name.upper())
    return 12 * (int(m.group(2)) + 1) + NOTE[m.group(1)]


class Patch:
    def __init__(self, name, waves, tempo, division, length):
        assert len(name) <= 12 and name == name.upper()
        self.name, self.waves = name, waves
        self.ops, self.steps, self.locks = [], {}, []
        self.glob(8, tempo).glob(9, length).glob(10, division)
        self.length = length

    def slot(self, slots, **params):
        for s in ([slots] if isinstance(slots, int) else slots):
            for key, value in params.items():
                self.ops.append(('AC_D_PARAM', 0, s, globals()[key], value))
        return self

    def all(self, **params):
        return self.slot(range(4), **params)

    def vector(self, axis, position=None, shape=None, rate=None, amount=None, sync=None, division=None):
        for i, v in enumerate([position, shape, rate, amount, sync, division]):
            if v is not None:
                self.ops.append(('AC_D_MOD', 0, 0, axis * 8 + i, DIV[v] if i == 5 else v))
        return self

    def fx(self, **values):
        for key, v in values.items():
            self.ops.append(('AC_D_FX', 0, 0, FX[key], v))
        return self

    def glob(self, index, value):
        self.ops.append(('AC_D_GLOBAL', 0, 0, index, value))
        return self

    def level(self, lane, value):
        self.ops.append(('AC_D_LEVEL', lane, 0, 0, value))
        return self

    def note(self, lane, step, note, vel=100, gate=75, prob=100):
        assert 0 <= lane < 10 and 0 <= step < self.length, (self.name, lane, step)
        assert 1 <= vel <= 127 and 1 <= gate <= 100 and 0 <= prob <= 100
        self.steps[(lane, step)] = (n(note) if isinstance(note, str) else note, vel, gate, prob)
        return self

    def pattern(self, lane, notes, start=0, every=1, **kw):
        """notes: list of note names / None (rest), one per `every` steps."""
        for i, x in enumerate(notes):
            if x is not None:
                self.note(lane, start + i * every, x, **kw)
        return self

    def chord(self, step, notes, first_lane=0, **kw):
        for i, x in enumerate(notes):
            self.note(first_lane + i, step, x, **kw)
        return self

    def lock(self, lane, step, slot, param, value):
        self.locks.append((lane, step, slot, param, value))
        return self


def bank_index(names):
    def find(name):
        name = name.upper()
        if name not in names:
            raise SystemExit(f'wave not in bank: {name}')
        return names.index(name)
    return find


def patches():
    P = []

    # 1. Wavestation-style wave sequence: slot A steps through all four waves.
    p = Patch('WAVE STATION', ['BELL1', 'DIGI3', 'VOICE2', 'SYNWAVE4'], 120, 4, 64)
    p.all(FTYPE=LP12, CUT=.62, RES=.2, A=.002, D=.3, S=.45, R=.6)
    p.vector(0, position=.5, shape=SINE, rate=.11, amount=.6).vector(1, position=.5, shape=TRI, rate=.07, amount=.5)
    p.fx(chorus=35, delay=45, reverb=60)
    arp = [0, 1, 2, 3, 2, 1, 2, 3]
    for bar, chord in enumerate([['A3', 'C4', 'E4', 'A4'], ['F3', 'A3', 'C4', 'F4'], ['C4', 'E4', 'G4', 'C5'], ['G3', 'B3', 'D4', 'G4']]):
        for i in range(16):
            step = bar * 16 + i
            p.note(0, step, chord[arp[i % 8]], vel=110 if i % 4 == 0 else 85, gate=60)
            p.lock(0, step, 0, SRC, i % 4)
        p.note(1, bar * 16, chord[0][0] + str(int(chord[0][-1]) - 1), vel=100, gate=100)
        p.note(1, bar * 16 + 8, chord[0][0] + str(int(chord[0][-1]) - 1), vel=80, gate=100)
    P.append(p)

    # 2. Slow glassy pad, chord per bar with a softer repeat on beat three.
    p = Patch('GLASS PAD', ['E.PIANO2', 'BELL3', 'SYNSINE3', 'VOICE1'], 80, 1, 16)
    p.all(FTYPE=LP24, CUT=.55, RES=.1, A=.8, D=2, S=.8, R=3)
    p.slot(1, LEVEL=.7, COARSE=12).slot(3, LEVEL=.8)
    p.vector(0, position=.5, shape=SINE, rate=.15, amount=.6).vector(1, position=.5, shape=TRI, rate=.09, amount=.5)
    p.fx(chorus=55, delay=20, reverb=95)
    for bar, chord in enumerate([['A3', 'E4', 'G4', 'B4', 'A2'], ['F3', 'C4', 'E4', 'A4', 'F2'],
                                 ['C4', 'E4', 'G4', 'B4', 'C3'], ['G3', 'D4', 'E4', 'B4', 'G2']]):
        p.chord(bar * 4, chord, vel=85, gate=100)
        p.chord(bar * 4 + 2, chord[:4], vel=55, gate=100, prob=70)
    P.append(p)

    # 3. Driving synth bass with accents and ghost octaves.
    p = Patch('DWGS BASS', ['SYNBASS2', 'BASS3', 'SAW C3', 'SQUARE C5'], 124, 4, 16)
    p.all(FTYPE=MOOG, CUT=.52, RES=.4, A=.001, D=.22, S=.35, R=.12)
    p.slot(3, COARSE=-12, LEVEL=.6)
    p.vector(0, position=.3).vector(1, position=.5)
    p.fx(dist=35, reverb=15, level=127)
    line = ['E2', None, 'E2', 'E3', None, 'E2', 'G2', None, 'E2', None, 'A2', 'E2', None, 'B2', 'D3', None]
    for i, x in enumerate(line):
        if x:
            p.note(0, i, x, vel=122 if i in (0, 6, 10) else 88, gate=45 if i % 2 else 65)
    p.pattern(1, ['E3', None, None, None] * 4, start=2, vel=60, gate=30, prob=50)
    P.append(p)

    # 4. Organ offbeat stabs over a walking bass.
    p = Patch('ORGAN GROOVE', ['ORGAN2', 'ORGAN5', 'CLAV1', 'SINE C5'], 100, 4, 32)
    p.all(FTYPE=LP12, CUT=.8, RES=.05, A=.002, D=.3, S=.6, R=.15)
    p.slot(2, LEVEL=.6).slot(3, COARSE=12, LEVEL=.5)
    p.vector(0, position=.45, shape=SINE, rate=5.5, amount=.15)
    p.fx(chorus=70, reverb=30)
    for bar, chord in enumerate([['D4', 'F4', 'A4', 'C5'], ['B3', 'D4', 'F4', 'G4']]):
        for s in (2, 6, 10, 14):
            p.chord(bar * 16 + s, chord, vel=95 if s in (6, 14) else 80, gate=35)
    p.pattern(4, ['D2', 'F2', 'A2', 'C3', 'G2', 'B2', 'D3', 'F3'], every=4, vel=100, gate=80)
    P.append(p)

    # 5. Choir from the voice waves, eight-bar minor progression.
    p = Patch('VOX CHOIR', ['VOICE1', 'VOICE3', 'VOX C5', 'VOICE2 C3'], 72, 1, 32)
    p.all(FTYPE=LP12, CUT=.5, RES=.1, A=1.2, D=2, S=.9, R=3.5)
    p.vector(1, position=.5, shape=SINE, rate=.07, amount=.7)
    p.fx(chorus=40, reverb=110)
    prog = [['D4', 'F4', 'A4', 'D3'], ['D4', 'F4', 'BB4', 'BB2'], ['C4', 'F4', 'A4', 'F2'], ['C4', 'E4', 'G4', 'C3'],
            ['D4', 'F4', 'A4', 'D3'], ['D4', 'G4', 'BB4', 'G2'], ['C#4', 'E4', 'A4', 'A2'], ['C#4', 'E4', 'A4', 'A2']]
    for bar, chord in enumerate(prog):
        p.chord(bar * 4, chord, vel=90, gate=100)
        p.chord(bar * 4 + 2, chord[:3], vel=60, gate=100, prob=60)
    P.append(p)

    # 6. Half-time growl bass; the X LFO wobbles across the growl waves in time.
    p = Patch('GROWL BASS', ['GROWL 008', 'GROWL 010', 'GROWL 005', 'SAW C5'], 140, 4, 32)
    p.all(FTYPE=LP24, CUT=.6, RES=.35, A=.001, D=.3, S=.75, R=.1)
    p.slot(3, COARSE=-12, LEVEL=.5)
    p.vector(0, position=.5, shape=SQUARE, amount=.8, sync=1, division='1/8').vector(1, position=.5, shape=SINE, amount=.4, sync=1, division='1/2')
    p.fx(dist=50, glide=20, reverb=20, level=120)
    for i, x in enumerate(['F1', None, None, 'F2', None, None, 'F1', None, 'AB1', None, 'F1', None, 'EB2', None, 'C2', None] * 2):
        if x:
            p.note(0, i, x, vel=115 if i % 16 == 0 else 95, gate=90 if i % 16 in (0, 6) else 55)
    P.append(p)

    # 7. Gliding lead over a pulsing bass.
    p = Patch('VIRUS LEAD', ['VIRUS 012', 'VIRUS 045', 'VIRUS 101', 'SAW C5'], 116, 4, 32)
    p.all(FTYPE=LP12, CUT=.7, RES=.3, A=.005, D=.4, S=.7, R=.3)
    p.vector(0, position=.5, shape=TRI, rate=.25, amount=.5)
    p.fx(glide=40, delay=60, reverb=40, chorus=20)
    mel = ['A4', None, 'C5', 'D5', None, 'E5', None, 'G5', 'E5', None, 'D5', None, 'C5', 'D5', None, None,
           'A4', None, 'C5', 'D5', None, 'E5', 'G5', 'A5', None, 'G5', 'E5', None, 'D5', None, 'C5', None]
    for i, x in enumerate(mel):
        if x:
            p.note(0, i, x, vel=105, gate=95 if mel[(i + 1) % 32] is None else 100)
    p.pattern(1, ['A2', 'A2', 'A3', 'A2'] * 4 + ['F2', 'F2', 'F3', 'F2'] * 2 + ['G2', 'G2', 'G3', 'G2'] * 2, every=1, vel=75, gate=40)
    p.level(1, .7)
    P.append(p)

    # 8. Random bell arpeggio in C major pentatonic.
    p = Patch('BELL TREE', ['BELL1', 'BELL2', 'BELL4', 'SYNSINE1'], 96, 4, 32)
    p.all(FTYPE=LP12, CUT=1, RES=0, A=.001, D=1.2, S=0, R=1.6)
    p.slot(3, COARSE=-12, LEVEL=.6)
    p.vector(0, position=.5, shape=SH, rate=1.5, amount=.7)
    p.fx(delay=70, reverb=90)
    pent = ['C5', 'D5', 'E5', 'G5', 'A5', 'C6', 'A5', 'G5']
    for i in range(32):
        p.note(0, i, pent[(i * 3) % 8], vel=60 + (i * 37) % 60, gate=40, prob=80 if i % 4 == 0 else 45)
    for i in range(0, 32, 2):
        p.note(1, i, ['C4', 'G4', 'E4', 'A4'][(i // 2) % 4], vel=55, gate=40, prob=30)
    P.append(p)

    # 9. Syncopated clav chords through a band-pass "wah".
    p = Patch('CLAV FUNK', ['CLAV2', 'GUITAR2', 'CLAV1', 'SQUARE C5'], 104, 4, 16)
    p.all(FTYPE=BP, CUT=.62, RES=.3, A=.001, D=.18, S=.4, R=.08)
    p.slot([1, 3], FTYPE=LP12, CUT=.7, RES=.1)
    p.vector(0, position=.5, shape=SINE, amount=.6, sync=1, division='1')
    p.fx(chorus=20, reverb=20, level=127)
    for s in (0, 3, 6, 8, 10, 13):
        p.chord(s, ['E3', 'G#3', 'D4', 'F#4'], vel=110 if s in (0, 8) else 85, gate=25)
    p.pattern(4, ['E2', None, None, 'E2', None, None, 'G2', None, 'A2', None, 'B2', None, None, 'D3', 'B2', None], vel=100, gate=50)
    P.append(p)

    # 10. Electric-piano comping, eighth-note grid.
    p = Patch('DIGI KEYS', ['DIGI2', 'E.PIANO1', 'DIGI5', 'E.PIANO3'], 88, 2, 32)
    p.all(FTYPE=LP12, CUT=.65, RES=.05, A=.002, D=1.5, S=.3, R=.8)
    p.vector(0, position=.5, shape=SINE, rate=4.5, amount=.12)
    p.fx(chorus=60, reverb=50)
    for bar, chord in enumerate([['E4', 'G4', 'B4', 'D5', 'C3'], ['C4', 'E4', 'G4', 'B4', 'A2'],
                                 ['C4', 'E4', 'F4', 'A4', 'D3'], ['B3', 'E4', 'F4', 'A4', 'G2']]):
        p.chord(bar * 8, chord, vel=95, gate=100)
        p.chord(bar * 8 + 3, chord[:4], vel=70, gate=60)
        p.note(4, bar * 8 + 6, chord[4], vel=70, gate=50, prob=60)
    P.append(p)

    # 11. Sparse resonant noise and sine texture.
    p = Patch('NOISE SWEEP', ['NOISE', 'SYNSINE6', 'NOISE', 'ENDLESS'], 70, 2, 32)
    p.all(FTYPE=BP, CUT=.5, RES=.6, A=1.5, D=2, S=.7, R=4)
    p.slot(1, FTYPE=LP12, CUT=.6, RES=.1).slot(2, COARSE=12)
    p.vector(0, position=.5, shape=SH, rate=.3, amount=.8).vector(1, position=.5, shape=SINE, rate=.04, amount=.6)
    p.fx(delay=60, reverb=120, level=127)
    scale = ['C4', 'EB4', 'G4', 'BB4', 'D5']
    for i in range(32):
        p.note(i % 3, i, scale[(i * 2) % 5], vel=90, gate=100, prob=50)
    P.append(p)

    # 12. Power riff from the fifth waves.
    p = Patch('5TH STACK', ['5THWAVE1', '5THWAVE3', 'SYNWAVE6', 'SAW C3'], 118, 4, 16)
    p.all(FTYPE=MOOG, CUT=.64, RES=.3, A=.001, D=.4, S=.7, R=.2)
    p.vector(0, position=.4)
    p.fx(dist=60, reverb=25, level=120)
    riff = ['E2', None, 'E2', None, 'G2', None, 'E2', None, 'A2', None, 'E2', 'E2', 'B2', None, 'A2', None]
    for i, x in enumerate(riff):
        if x:
            p.note(0, i, x, vel=115 if i in (0, 8) else 95, gate=70)
    P.append(p)

    # 13. Chip-style arpeggios; the X LFO flips waves every sixteenth.
    p = Patch('CHIP ARP', ['TRIANGLE C5', 'SQUARE C5', 'SINE C5', 'SAW C5'], 150, 4, 64)
    p.all(FTYPE=LP12, CUT=1, RES=0, A=.001, D=.2, S=.5, R=.1)
    p.vector(0, position=.5, shape=SQUARE, amount=1, sync=1, division='1/16')
    p.fx(delay=20)
    for bar, chord in enumerate([['C4', 'E4', 'G4'], ['A3', 'C4', 'E4'], ['F3', 'A3', 'C4'], ['G3', 'B3', 'D4']]):
        tones = chord + [c[:-1] + str(int(c[-1]) + 1) for c in chord]
        for i in range(16):
            p.note(0, bar * 16 + i, tones[[0, 1, 2, 3, 4, 5, 4, 3][i % 8]], vel=100, gate=50)
        p.note(1, bar * 16, chord[0][:-1] + str(int(chord[0][-1]) - 1), vel=90, gate=100)
        p.note(1, bar * 16 + 8, chord[0][:-1] + str(int(chord[0][-1]) - 1), vel=80, gate=100)
    P.append(p)

    # 14. Evolving pad from the Virus waves.
    p = Patch('VIRUS PAD', ['VIRUS 150', 'VIRUS 177', 'SYNWAVE2', 'VIRUS 060'], 76, 1, 16)
    p.all(FTYPE=LP24, CUT=.6, RES=.15, A=1.5, D=2.5, S=.85, R=4)
    p.vector(0, position=.5, shape=SINE, rate=.05, amount=.9).vector(1, position=.5, shape=TRI, rate=.08, amount=.6)
    p.fx(chorus=60, reverb=100)
    for bar, chord in enumerate([['E3', 'B3', 'D4', 'F#4', 'E2'], ['C3', 'G3', 'B3', 'E4', 'C2'],
                                 ['G3', 'D4', 'A4', 'B4', 'G2'], ['D3', 'A3', 'B3', 'E4', 'D2']]):
        p.chord(bar * 4, chord, vel=85, gate=100)
    P.append(p)

    # 15. Berlin-school sequence; per-step filter locks open the line.
    p = Patch('BERLIN SEQ', ['GUITAR1', 'BASS1', 'SYNBASS5', 'SAW C3'], 112, 4, 16)
    p.all(FTYPE=LP24, CUT=.35, RES=.5, A=.001, D=.25, S=.1, R=.2)
    p.fx(delay=50, reverb=40)
    line = ['C2', 'C3', 'G2', 'BB2', 'C3', 'EB3', 'G2', 'C3', 'C2', 'C3', 'G2', 'BB2', 'F3', 'EB3', 'G2', 'BB2']
    for i, x in enumerate(line):
        p.note(0, i, x, vel=110 if i % 4 == 0 else 85, gate=50)
        for slot in range(4):
            p.lock(0, i, slot, CUT, round(.3 + .5 * (i / 15), 3))
        p.note(1, i, n(x) + 12, vel=60, gate=30, prob=40)
    P.append(p)

    # 16. Slow rising notes over a pedal, long tails.
    p = Patch('ENDLESS', ['ENDLESS', 'SYNSINE4', 'SYNWAVE1', 'VOICE4'], 60, 1, 16)
    p.all(FTYPE=LP12, CUT=.6, RES=.15, A=.5, D=2, S=.7, R=5)
    p.vector(0, position=.5, shape=SINE, rate=.03, amount=.8).vector(1, position=.5, shape=SINE, rate=.045, amount=.8)
    p.fx(delay=70, reverb=127)
    for i in range(16):
        p.note(0, i, n('C4') + 2 * (i % 8), vel=75, gate=100)
    for bar in range(4):
        p.note(1, bar * 4, 'C3', vel=70, gate=100)
    P.append(p)
    assert len(P) == 16
    return P


def c_float(v):
    return f'{float(v)!r}f'


def main():
    bank = json.loads((ROOT / 'assets/wave-bank-manifest.json').read_text())
    names = [x.upper() for x in bank['originals']] + [e['name'] for e in bank['entries']]
    find = bank_index(names)
    P = patches()
    out = ['/* Generated by tools/factory_patches.py. Do not edit. */', '#pragma once', '#include <stdint.h>',
           '#define AF_FACTORY_SEQ_COUNT 16u',
           'typedef struct {uint8_t field, lane, slot, index; float value;} afp_op;',
           'typedef struct {uint8_t lane, step, note, velocity, gate, probability;} afp_step;',
           'typedef struct {uint8_t lane, step, slot, parameter; float value;} afp_lock;',
           'typedef struct {const char *name; uint16_t wave[4]; const afp_op *ops; uint16_t nops; const afp_step *steps; uint16_t nsteps; const afp_lock *locks; uint16_t nlocks;} afp_patch;']
    for i, p in enumerate(P):
        out.append(f'static const afp_op afp_ops_{i}[] = {{')
        out += [f'    {{{f}, {l}, {s}, {x}, {c_float(v)}}},' for f, l, s, x, v in p.ops]
        out.append('};')
        out.append(f'static const afp_step afp_steps_{i}[] = {{')
        out += [f'    {{{l}, {s}, {nt}, {vel}, {g}, {pr}}},' for (l, s), (nt, vel, g, pr) in sorted(p.steps.items())]
        out.append('};')
        if p.locks:
            out.append(f'static const afp_lock afp_locks_{i}[] = {{')
            out += [f'    {{{l}, {s}, {sl}, {pa}, {c_float(v)}}},' for l, s, sl, pa, v in p.locks]
            out.append('};')
    out.append('static const afp_patch afp_patches[AF_FACTORY_SEQ_COUNT] = {')
    for i, p in enumerate(P):
        waves = ', '.join(str(find(w)) for w in p.waves)
        locks = f'afp_locks_{i}, {len(p.locks)}' if p.locks else '0, 0'
        out.append(f'    {{"{p.name}", {{{waves}}}, afp_ops_{i}, {len(p.ops)}, afp_steps_{i}, {len(p.steps)}, {locks}}},')
    out.append('};')
    (ROOT / 'src/generated/factory_patches.h').write_text('\n'.join(out) + '\n')
    js = ['// Generated by tools/factory_patches.py. Do not edit.',
          'export const factoryNames=' + json.dumps(['VECTOR', 'SINE', 'SAW', 'BELL', 'VOICE', 'USER VECTOR'] + [p.name for p in P]) + ';']
    (ROOT / 'web/factory-names.js').write_text('\n'.join(js) + '\n')
    for p in P:
        print(f'{p.name:13s} {len(p.steps):4d} steps {len(p.locks):3d} locks waves {p.waves}')


if __name__ == '__main__':
    main()
