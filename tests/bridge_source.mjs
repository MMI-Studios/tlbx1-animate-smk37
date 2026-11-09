import fs from 'node:fs';
import assert from 'node:assert/strict';
const bytes=fs.readFileSync(new URL('../build/bridge/animate-bridge.wasm',import.meta.url));
async function run(mode,slot,source){const {instance:{exports:e}}=await WebAssembly.instantiate(bytes);e.ab_init();assert.equal(e.ab_polyphony(16),1);const handles=[];for(let k=0;k<16;k++)handles.push(e.ab_owned_note_on(48+k,90));assert(handles.every(Boolean));const output=[];for(let b=0;b<100;b++){if(b===20){if(mode===0)e.ab_slot(slot,1,source);else if(mode===1)e.ab_slot_parameter(slot,0,source);else for(const h of handles)e.ab_voice_parameter(h,slot,0,source);}e.ab_render(e.ab_output(),32);output.push(...new Int32Array(e.memory.buffer,e.ab_output(),64));}assert.equal(e.ab_active(),16);return output;}
for(let slot=0;slot<4;slot++)for(let source=0;source<4;source++){const native=await run(0,slot,source);assert.deepEqual(await run(1,slot,source),native);assert.deepEqual(await run(2,slot,source),native);}
console.log('PASS all slot/source combinations: live source edits and per-voice source locks match native audio exactly with sixteen held voices');
