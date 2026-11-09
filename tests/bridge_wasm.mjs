import fs from 'node:fs';
import assert from 'node:assert/strict';
const bytes=fs.readFileSync(new URL('../build/bridge/animate-bridge.wasm',import.meta.url));
// Native unified-kernel hashes (check_runtime.py / build_bridge.py candidate).
// The donor comparison is tolerance-based since the 2026-11-07 kernel rewrite.
const golden=[3052755883,1240670629,3663639197,2970021757,549255579,3031936733];
for(let preset=0;preset<6;preset++){
 const {instance:{exports:e}}=await WebAssembly.instantiate(bytes);e.ab_init();e.ab_preset(preset);
 for(let slot=0;slot<4;slot++){const mask=e.ab_slot_parameter_mask(slot);for(let id=0;id<20;id++)assert.equal(e.ab_slot_parameter(slot,id,e.ab_slot_parameter_get(slot,id)),1);assert.equal(e.ab_slot_parameter_mask_set(slot,mask),1);}
 for(let axis=0;axis<2;axis++){const mask=e.ab_vector_lfo_mask(axis);for(let field=0;field<5;field++)assert.equal(e.ab_vector_lfo(axis,field,e.ab_vector_lfo_get(axis,field)),1);assert.equal(e.ab_vector_lfo_mask_set(axis,mask),1);}
 let hash=2166136261;
 function render(n){for(let b=0;b<n;b++){assert.equal(e.ab_render(e.ab_output(),32),1);for(const s of new Int32Array(e.memory.buffer,e.ab_output(),64))hash=Math.imul(hash^s,16777619)>>>0;}}
 render(30);const a=e.ab_note_on(60,110);assert.ok(a);render(400);
 const b=e.ab_note_on(64,100),c=e.ab_note_on(67,90);render(200);
 e.ab_note_off(a);e.ab_note_off(b);e.ab_note_off(c);render(4000);
 assert.equal(hash,golden[preset],`WASM preset ${preset}`);
 console.log('Exact native/WASM unified-kernel match',preset,hash);
}
