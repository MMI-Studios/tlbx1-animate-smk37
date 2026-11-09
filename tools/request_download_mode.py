"""Explicitly request ROM download mode; resets the synth, never writes flash.

Requires separate owner approval: unsaved runtime state is lost if it succeeds.
The bytes match usb.c UBOOT_KEY and M-Flasher's documented download command.
"""
import ctypes as c
from ctypes import wintypes as w
import sys
import time
from probe_midi_identity import OutCaps, Header

def main():
    if sys.platform != 'win32' or sys.argv[1:] != ['--confirm-reset']:
        raise SystemExit('Requires Windows and --confirm-reset; obtain owner approval first.')
    mm=c.WinDLL('winmm')
    mm.midiOutGetDevCapsW.argtypes=[c.c_size_t,c.POINTER(OutCaps),w.UINT]
    mm.midiOutOpen.argtypes=[c.POINTER(c.c_void_p),w.UINT,c.c_size_t,c.c_size_t,w.DWORD]
    for name in ['PrepareHeader','UnprepareHeader','LongMsg']:
        getattr(mm,'midiOut'+name).argtypes=[c.c_void_p,c.POINTER(Header),w.UINT]
    mm.midiOutReset.argtypes=[c.c_void_p];mm.midiOutClose.argtypes=[c.c_void_p]
    def check(rc):
        if rc: raise RuntimeError(f'WinMM error {rc}')
    ports=[]
    for index in range(mm.midiOutGetNumDevs()):
        caps=OutCaps();check(mm.midiOutGetDevCapsW(index,c.byref(caps),c.sizeof(caps)))
        if caps.name in ('Melodee','Melodee In'):
            ports.append((index,caps.name))
    if len(ports)!=1: raise RuntimeError(f'Expected exactly one previously identified Melodee output: {ports}; nothing sent')
    handle=c.c_void_p();prepared=False
    data=c.create_string_buffer(bytes.fromhex('f0 22 24 35 7d f7'))
    header=Header(data=c.addressof(data),length=6)
    try:
        check(mm.midiOutOpen(c.byref(handle),ports[0][0],0,0,0))
        check(mm.midiOutPrepareHeader(handle,c.byref(header),c.sizeof(header)));prepared=True
        check(mm.midiOutLongMsg(handle,c.byref(header),c.sizeof(header)))
        end=time.monotonic()+4
        while not header.flags&1 and time.monotonic()<end: time.sleep(.01)
        print(f'Download-mode request sent to {ports[0][1]}; Windows completed={bool(header.flags&1)}. No flash commands sent. Device mode must be checked independently.')
    finally:
        if handle.value:
            mm.midiOutReset(handle)
            if prepared:mm.midiOutUnprepareHeader(handle,c.byref(header),c.sizeof(header))
            mm.midiOutClose(handle)

if __name__=='__main__':main()
