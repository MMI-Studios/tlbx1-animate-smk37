"""Windows WinMM identity-only diagnostic. Never enters a loader or writes flash."""
import ctypes as c
from ctypes import wintypes as w
import sys
import time

QUERY = bytes.fromhex('f0 00 32 45 00 00 00 40 7f f7')
STATUS = bytes.fromhex('f0 7d 41 46 01 01 01 00 f7')

class InCaps(c.Structure):
    _fields_ = [('mid',w.WORD),('pid',w.WORD),('version',w.DWORD),
                ('name',w.WCHAR*32),('support',w.DWORD)]

class OutCaps(c.Structure):
    _fields_ = [('mid',w.WORD),('pid',w.WORD),('version',w.DWORD),
                ('name',w.WCHAR*32),('technology',w.WORD),('voices',w.WORD),
                ('notes',w.WORD),('channel_mask',w.WORD),('support',w.DWORD)]

class Header(c.Structure):
    _fields_ = [('data',c.c_void_p),('length',w.DWORD),('recorded',w.DWORD),
                ('user',c.c_size_t),('flags',w.DWORD),('next',c.c_void_p),
                ('reserved',c.c_size_t),('offset',w.DWORD),('reserved2',c.c_size_t*8)]

def main():
    if sys.platform != 'win32':
        raise SystemExit('Windows only')
    if sys.argv[1:] not in ([],['--status']):
        raise SystemExit('Usage: probe_midi_identity.py [--status] (read-only requests)')
    query=STATUS if sys.argv[1:] else QUERY
    mm=c.WinDLL('winmm')
    for prefix,Caps in [('In',InCaps),('Out',OutCaps)]:
        getattr(mm,f'midi{prefix}GetDevCapsW').argtypes=[c.c_size_t,c.POINTER(Caps),w.UINT]
        getattr(mm,f'midi{prefix}Open').argtypes=[c.POINTER(c.c_void_p),w.UINT,c.c_size_t,c.c_size_t,w.DWORD]
        for action in ['PrepareHeader','UnprepareHeader']:
            getattr(mm,f'midi{prefix}{action}').argtypes=[c.c_void_p,c.POINTER(Header),w.UINT]
        for action in ['Reset','Close']:
            getattr(mm,f'midi{prefix}{action}').argtypes=[c.c_void_p]
    mm.midiInStart.argtypes=[c.c_void_p]
    mm.midiInStop.argtypes=[c.c_void_p]
    mm.midiInAddBuffer.argtypes=[c.c_void_p,c.POINTER(Header),w.UINT]
    mm.midiOutLongMsg.argtypes=[c.c_void_p,c.POINTER(Header),w.UINT]
    def check(rc,operation):
        if rc: raise RuntimeError(f'{operation}: WinMM error {rc}')
    selected=[]
    for prefix,Caps in [('In',InCaps),('Out',OutCaps)]:
        matches=[]
        for index in range(getattr(mm,f'midi{prefix}GetNumDevs')()):
            caps=Caps()
            check(getattr(mm,f'midi{prefix}GetDevCapsW')(index,c.byref(caps),c.sizeof(caps)),'device capabilities')
            if 'melodee' in caps.name.lower() or 'smk' in caps.name.lower():
                print(f'{prefix} {index}: {caps.name}',flush=True)
                matches.append(index)
        if len(matches)!=1: raise RuntimeError(f'Expected exactly one Melodee/SMK {prefix} port, found {len(matches)}; no query sent')
        selected.append(matches[0])
    inp,out=c.c_void_p(),c.c_void_p()
    rx=c.create_string_buffer(4096);tx=c.create_string_buffer(query)
    ih=Header(data=c.addressof(rx),length=4096)
    oh=Header(data=c.addressof(tx),length=len(query))
    ip=op=False
    try:
        check(mm.midiInOpen(c.byref(inp),selected[0],0,0,0),'open input')
        check(mm.midiOutOpen(c.byref(out),selected[1],0,0,0),'open output')
        check(mm.midiInPrepareHeader(inp,c.byref(ih),c.sizeof(ih)),'prepare input');ip=True
        check(mm.midiInAddBuffer(inp,c.byref(ih),c.sizeof(ih)),'queue input')
        check(mm.midiInStart(inp),'start input')
        check(mm.midiOutPrepareHeader(out,c.byref(oh),c.sizeof(oh)),'prepare output');op=True
        check(mm.midiOutLongMsg(out,c.byref(oh),c.sizeof(oh)),'send identity query')
        print('Sent read-only '+('storage status' if query==STATUS else 'identity')+' query; listening for four seconds.',flush=True)
        end=time.monotonic()+4
        while time.monotonic()<end and not ih.flags&1: time.sleep(.01)
        mm.midiInStop(inp);mm.midiInReset(inp)
        print(f'Output completed: {bool(oh.flags&1)}; received {ih.recorded} SysEx bytes')
        if ih.recorded: print(bytes(rx[:ih.recorded]).hex(' '))
        else: print('No SysEx reply received through native Windows MIDI.')
    finally:
        if inp.value:
            mm.midiInReset(inp)
            if ip: mm.midiInUnprepareHeader(inp,c.byref(ih),c.sizeof(ih))
            mm.midiInClose(inp)
        if out.value:
            mm.midiOutReset(out)
            if op: mm.midiOutUnprepareHeader(out,c.byref(oh),c.sizeof(oh))
            mm.midiOutClose(out)

if __name__=='__main__': main()
