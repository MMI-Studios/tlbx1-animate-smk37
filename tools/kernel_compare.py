#!/usr/bin/env python3
"""Compare the unified kernel against the per-sample reference renderer."""
import pathlib, subprocess, sys
import numpy as np
ROOT = pathlib.Path(__file__).resolve().parents[1]
D = ROOT / 'vendor' / 'melodee'
OUT = ROOT / 'build' / 'host'; OUT.mkdir(parents=True, exist_ok=True)
NAMES = ['VECTOR','SINE','SAW','BELL','VOICE','USER VECTOR','native filters','LP24','LP12','HP','BP','MOOG',
         'pan/level/pitch','windows/position LFO','extended envelopes','vector LFO + solo','live window edits','8 voices','(native window)','(native pos LFO sine)','(ext loop window)','(ext S&H position)']
def build(name, extra):
    exe = OUT / name
    subprocess.run(['cc','-std=gnu11','-O2','-fwrapv',*extra,'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/baseline'),
                    '-I'+str(D/'firmware/src'),'-I'+str(D/'build/gen'),'-I'+str(D/'web/audio/include'),
                    str(ROOT/'src/animate_math.c'),str(ROOT/'tests/kernel_compare.c'),'-lm','-o',str(exe)],check=True)
    return exe
def stft_db(x):
    n=1024;h=256;win=np.hanning(n);frames=[x[i:i+n]*win for i in range(0,len(x)-n,h)]
    return 20*np.log10(np.abs(np.fft.rfft(np.array(frames),axis=1))+1.0)
ref, new = build('kernel-ref',['-DAB_REFERENCE_KERNEL=1']), build('kernel-new',[])
worst = []
print(f"{'scenario':24s} {'level dB':>8s} {'wave SNR':>8s} {'spec err dB':>11s} {'tail':>6s}")
for i,name in enumerate(NAMES):
    a=OUT/'k_ref.bin'; b=OUT/'k_new.bin'
    subprocess.run([str(ref),str(i),str(a)],check=True); subprocess.run([str(new),str(i),str(b)],check=True)
    x=np.fromfile(a,dtype=np.int32).astype(np.float64); y=np.fromfile(b,dtype=np.int32).astype(np.float64)
    xm=x.reshape(-1,2).mean(1); ym=y.reshape(-1,2).mean(1)
    rx=np.sqrt(np.mean(x**2))+1e-9; ry=np.sqrt(np.mean(y**2))+1e-9
    snr=10*np.log10(np.sum(x**2)/(np.sum((x-y)**2)+1e-9))
    sx=stft_db(xm); sy=stft_db(ym); loud=sx>np.max(sx)-60
    spec=np.mean(np.abs(sx-sy)[loud])
    tail=np.sqrt(np.mean(y[-8000:]**2))/ (np.sqrt(np.mean(x[-8000:]**2))+1e-9) if np.any(x[-8000:]) else (0.0 if not np.any(y[-8000:]) else 99.0)
    level=20*np.log10(ry/rx)
    worst.append((name,level,snr,spec))
    print(f"{name:24s} {level:8.2f} {snr:8.1f} {spec:11.2f} {tail:6.2f}")
if '--check' in sys.argv:
    bad=[w for w in worst if abs(w[1])>0.5 or w[3]>1.5]
    assert not bad, bad
    print('PASS: unified kernel within 0.5 dB level and 1.5 dB mean spectral difference of the reference in all scenarios')
