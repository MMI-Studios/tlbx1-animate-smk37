import fs from 'node:fs';import assert from 'node:assert/strict';
const bytes=fs.readFileSync(new URL('../web/animate.wasm',import.meta.url));
async function fresh(){const{instance:{exports:e}}=await WebAssembly.instantiate(bytes);e.af_init();return e;}
function save(e){const n=e.af_project_size();assert.ok(n);const bytes=new Uint8Array(n);for(let at=0;at<n;at+=1024){const count=Math.min(1024,n-at);assert.equal(e.af_project_read(at,count),count);bytes.set(new Uint8Array(e.memory.buffer,e.af_project_buffer(),count),at);}return bytes;}
function load(e,bytes){assert.equal(e.af_project_begin(bytes.length),1);for(let at=0;at<bytes.length;at+=1024){const count=Math.min(1024,bytes.length-at);new Uint8Array(e.memory.buffer,e.af_project_buffer(),count).set(bytes.subarray(at,at+count));assert.equal(e.af_project_write(at,count),1);}return e.af_project_commit();}
function fixture(e){let hash=2166136261;function render(n){for(let i=0;i<n;i++){e.af_render(32);for(const f of new Float32Array(e.memory.buffer,e.af_output(),64))hash=Math.imul(hash^(f*32768),16777619)>>>0;}}render(30);e.af_key(60,110);render(400);e.af_key(64,100);e.af_key(67,90);render(200);e.af_key(60,0);e.af_key(64,0);e.af_key(67,0);render(4000);return hash;}
for(let preset=0;preset<6;preset++){const before=await fresh(),after=await fresh();assert.equal(before.af_set(0,0,12,20,preset),1);const project=save(before);assert.equal(load(after,project),1);assert.equal(fixture(after),fixture(before),`cold preset ${preset} audio after session import`);console.log('Cold preset audio roundtrip exact',preset);}
const a=await fresh();const vals=[1,.125,.5,.73,-.25,.4,4,.731,1,3,.15,.23,.82,4,.332,.7,.123,.456,.678,.901];for(let slot=0;slot<4;slot++)for(let id=0;id<20;id++)assert.equal(a.af_set(0,slot,0,id,vals[id]),1);
for(let axis=0;axis<2;axis++)for(const[k,value]of [[0,.33],[1,4],[2,3.125],[3,.72],[4,1],[5,3]])assert.equal(a.af_set(0,0,13,axis*8+k,value),1);
assert.equal(a.af_set(0,0,12,17,1),1);assert.equal(a.af_set(0,0,12,24,3),1);assert.equal(a.af_set(0,0,12,8,137),1);assert.equal(a.af_control_set(2,2),1);assert.equal(a.af_control_set(0,8),1);
const project=save(a),b=await fresh();assert.equal(load(b,project),1);
for(let slot=0;slot<4;slot++)for(let id=0;id<20;id++)assert.equal(b.af_get(0,slot,0,id),a.af_get(0,slot,0,id),`slot${slot} param${id}`);
for(let axis=0;axis<2;axis++)for(let i=0;i<6;i++)assert.equal(b.af_get(0,0,13,axis*8+i),a.af_get(0,0,13,axis*8+i));
assert.equal(b.af_get(0,0,12,17),1,'ARP restored');assert.equal(b.af_get(0,0,12,18),0);assert.equal(b.af_get(0,0,12,8),137);
assert.equal(fixture(b),fixture(a),'edited parameter masks/LFO state audio roundtrip');
const live=await fresh();live.af_key(60,100);live.af_render(32);const voices=live.af_get(0,0,12,5),snapshot=save(live),bad=snapshot.slice();bad[40]^=1;assert.equal(load(live,bad),0);assert.equal(live.af_get(0,0,12,5),voices,'corrupt import retains held voices');assert.deepEqual(save(live),snapshot,'corrupt import retains full session');
console.log('PASS: six exact cold preset imports; all80 float parameters; vector masks; ARP; corruption preserves session and live voices.');
