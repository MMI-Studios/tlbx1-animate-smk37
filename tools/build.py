#!/usr/bin/env python3
"""Build the Animate 982 board services or complete application. Never flash."""
import argparse, hashlib, importlib.util, json, os, pathlib, re, subprocess, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]
OLD = ROOT / 'vendor'
DONOR = OLD / 'melodee'
OUT = ROOT / 'build' / 'smk'
# CPU1 renders complete control blocks while CPU0 services scanner/USB IRQs.
# The inherited worker joins before CPU0 accesses runtime state again.
DUAL_CORE = os.environ.get('AF_DUAL_CORE', '1')
assert DUAL_CORE in ('0', '1'), 'AF_DUAL_CORE must be 0 or 1'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('target', choices=['board', 'bridge', 'objects', 'smk'])
    args = p.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault('JIELI_TOOLCHAIN', str(ROOT / 'tools-local/jieli/toolchain'))
    os.environ['JIELI_DOCKER'] = '0'
    sys.path.insert(0, str(DONOR / 'tools'))
    spec = importlib.util.spec_from_file_location('build', DONOR / 'tools/build.py')
    b = importlib.util.module_from_spec(spec)
    sys.modules['build'] = b
    spec.loader.exec_module(b)
    from build_smk37_loader import build_loader
    b.SRC = ROOT
    b.CFLAGS += ['-I' + str(b.FW / 'hal')]
    original_tc = b.tc
    b.tc = lambda tool, *a: original_tc(tool, *[str(x) if isinstance(x, pathlib.Path) else x for x in a])
    gen = OUT / 'gen'
    gen.mkdir(exist_ok=True)
    (gen / 'smk37_unit.h').write_text('#define SMK37_PUBLIC_BUILD 1\n#define SMK37_UNIT_TRIM_CRC 0u\n#define SMK37_UNIT_HEAD_CRC 0x3a04ad23u\n#define SMK37_UNIT_SAMPLE_CRCS {0,0,0}\n#define SMK37_UPDATE_PRODUCT "SMK-37_982"\n#define SMK37_UPDATE_VERSION "Animate 0.1"\n#define SMK37_RELEASE_VERSION "Animate 0.1"\n')
    build_loader(OUT, gen, 'SMK-37_982')
    flags = [*b.CFLAGS, '-fdata-sections', '-DMELODEE_DUAL_CORE='+DUAL_CORE, '-DMELODEE_CACHE_RAM=0',
             '-DSMK37_ENABLE_USB_AUDIO=1', '-DFM1_IRQ_TARGET=1', '-DAF_HARDWARE=1',
             *['-I' + str(x) for x in [ROOT/'src', b.FW/'src', gen, DONOR/'build/gen', DONOR/'web/audio/include']]]
    # Scanner/USB handlers preempt audio. Optimize the board unit for throughput,
    # retaining the same arithmetic, scan cadence and hardware register accesses.
    board_flags = [('-O2' if f == '-Os' else f) for f in flags]
    b.tc('cc', *board_flags, '-c', ROOT/'src/hardware_entry.c', '-o', OUT/'board.o')
    if args.target == 'board':
        print('Board services compiled; no application image linked.')
        return
    if args.target == 'bridge':
        b.tc('cc', *flags, '-DAB_FREESTANDING=1', '-DAB_EXTERNAL_LIBC=1',
             '-c', ROOT/'src/baseline/bridge.c', '-o', OUT/'baseline_bridge.o')
        print('Board and Animate bridge compiled separately; no application image linked.')
        return
    # Execute the preserved public loader's synthetic-flash boundary checks.
    # These tests never enumerate or write a device.
    for name, extra in [('smk37_flash_profile', []), ('smk37_update', ['-DTEST_PUBLIC=1'])]:
        exe = OUT/(name+'-test')
        subprocess.run(['cc', '-std=c11', '-O2', *extra,
                        '-I'+str(b.FW/'hal'), '-I'+str(b.FW/'loader'),
                        str(DONOR/'tests'/(name+'_test.c')), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    sources = ['runtime', 'session', 'patch', 'storage', 'control', 'display', 'sequence', 'project_state',
               'parameter_codec', 'animate_parameters', 'animate_math', 'support/music', 'visual', 'splash']
    missing = [x for x in sources if not (ROOT/'src'/(x+'.c')).exists()]
    if missing and args.target != 'objects':
        raise RuntimeError('Application sources not ready: ' + ', '.join(missing))
    objects = [OUT/'board.o']
    for source in [*sources, 'baseline/bridge']:
        if source in missing:
            continue
        obj = OUT/(source.replace('/', '_')+'.o')
        extra = ['-DAB_FREESTANDING=1', '-DAB_EXTERNAL_LIBC=1'] if source == 'baseline/bridge' else []
        # The dedicated audio unit has ample flash headroom: optimize hot loops
        # for throughput, without fast-math or changing arithmetic contraction.
        source_flags = [('-O2' if f == '-Os' else f) for f in flags] if source == 'baseline/bridge' else flags
        b.tc('cc', *source_flags, *extra, '-c', ROOT/'src'/(source+'.c'), '-o', obj)
        objects.append(obj)
    if args.target == 'objects':
        print('Available application objects compiled; not linked. Missing: '+', '.join(missing))
        return
    for name, source in [('crt0', b.FW/'smk37_crt0.S'), ('vec', b.FW/'hal/fm1_vec.S')]:
        obj = OUT/(name+'.o')
        b.tc('cc', *flags, '-c', source, '-o', obj)
        objects.insert(0, obj)
    # Preserve donor flash/stack bounds and the 152 KiB audio reserve; the local
    # map reserves upper-pool SRAM for direct-call-reachable DSP code.
    elf = OUT/'animate.elf'
    b.tc('pi32v2/bin/ld', '--gc-sections', '-T', ROOT/'app.ld', *objects, '-o', elf)
    symbols = b.tc('common/bin/objdump', '-t', elf)
    (OUT/'symbols.txt').write_text(symbols)
    def sym(name):
        found = re.search(r'^([0-9a-f]+) .*\s'+re.escape(name)+r'$', symbols, re.M)
        if not found:
            raise RuntimeError('Missing linker symbol '+name)
        return int(found[1], 16)
    from smk_storage_audit import audit_ram
    # Every per-block audio routine and interrupt body must be SRAM-resident.
    hot_names = ['ak_render', 'ak_prepare', 'ak_run', 'ak_slot_run', 'ak_filter', 'ak_worker', 'ak_finish', 'smk37_audio_irq', 'smk37_scanner_irq', 'smk37_usb_body',
                 'af_render_q15', 'ab_render', 'track_render_audio', 'perf_block', 'dly_bus', 'rev_hall',
                 'mix_spread', 'master_out', 'mod_begin', 'as_advance']
    for hot in hot_names:
        address = sym(hot)
        if not sym('_dsp_start') <= address < sym('_dsp_end'):
            raise RuntimeError('Audio renderer not resident in DSP SRAM: '+hot)
    from smk_multicore_audit import audit_multicore
    multicore = audit_multicore(symbols, b.tc('common/bin/objdump', '-d', '-j', '.dsp_text', elf), DUAL_CORE == '1')
    disassembly = b.tc('common/bin/objdump', '-d', '-j', '.ram_text', elf)
    audit = audit_ram(symbols, disassembly)
    audit['write_ranges'] = [['0x9d000','0xbd000'],['0xfe000','0xff000']]
    (OUT/'storage-audit.json').write_text(json.dumps(audit,indent=2))
    (OUT/'ram-disassembly.txt').write_text(disassembly)
    exe=OUT/'animate-storage-test'
    subprocess.run(['cc','-std=c11','-O2','-I'+str(ROOT/'src'),str(ROOT/'src/storage.c'),str(ROOT/'tests/storage_test.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    exe=OUT/'animate-storage-protocol-test'
    subprocess.run(['cc','-std=c11','-O2','-I'+str(ROOT/'src'),str(ROOT/'src/storage.c'),str(ROOT/'tests/storage_protocol_test.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    image = bytearray()
    sections = sorted([('.boot', b.APP_XIP), ('.text', sym('_text_load')), ('.ram_text', sym('_rt_load')), ('.dsp_text', sym('_dsp_load')), ('.dsp_tables', sym('_dt_load')), ('.data', sym('_data_load'))], key=lambda x: x[1])
    for section, address in sections:
        f = OUT/(section[1:]+'.bin')
        b.tc('common/bin/objcopy', '-O', 'binary', '-j', section, elf, f)
        raw = f.read_bytes() if f.exists() else b''
        if not raw:
            continue
        offset = address-b.APP_XIP
        if offset < len(image):
            raise RuntimeError('Overlapping application sections')
        image.extend(b'\xff'*(offset-len(image)))
        image.extend(raw)
    image.extend(b'\xff'*(-len(image)%4))
    if len(image)>0x8dfbc or sym('_start')!=b.APP_XIP:
        raise RuntimeError('Application outside board flash bounds')
    (OUT/'animate-candidate.bin').write_bytes(image)
    report = dict(product='SMK-37_982', version='Animate 0.1', bytes=len(image), sha256=hashlib.sha256(image).hexdigest(),
                  bss_end=hex(sym('_bss_end')), pool_end=hex(sym('_pool_end')),
                  resource_bytes=sym('_resource_end')-sym('_resource_start'),
                  dual_core=DUAL_CORE == '1', multicore_audit=multicore,
                  installable=False, hardware_tested=False, device_project_persistence=False, device_patch_persistence=True)
    (OUT/'manifest.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
