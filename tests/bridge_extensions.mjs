import fs from 'node:fs';
import assert from 'node:assert/strict';
const bytes=fs.readFileSync(new URL('../build/bridge/animate-bridge.wasm',import.meta.url));
async function render(edits=[],voiceEdits=[],release=false,bypass=null){
 const {instance:{exports:e}}=await WebAssembly.instantiate(bytes);e.ab_init();e.ab_preset(1);
 for(const[id,value]of edits){assert.equal(e.ab_slot_parameter(0,id,value),1);assert.ok(Math.abs(e.ab_slot_parameter_get(0,id)-value)<1e-5);}
 if(bypass!==null){assert.equal(e.ab_slot(0,6,bypass),1);assert.equal(e.ab_slot_get(0,6),bypass);}
 const h=e.ab_owned_note_on(60,100);assert.ok(h);
 for(const[id,value]of voiceEdits)assert.equal(e.ab_voice_parameter(h,0,id,value),1);
 let hash=2166136261,left=0,right=0,peak=0;
 for(let b=0;b<1800;b++){
  if(release&&b===300)e.ab_note_off(h);
  e.ab_render(e.ab_output(),32);const samples=new Int32Array(e.memory.buffer,e.ab_output(),64);
  for(let i=0;i<64;i++){const s=samples[i];assert.ok(Number.isFinite(s));peak=Math.max(peak,Math.abs(s));hash=Math.imul(hash^s,16777619)>>>0;if(i&1)right+=s*s;else left+=s*s;}
 }
 assert.ok(peak<=32768&&peak>0);return{hash,left,right,e,h};
}
const base=await render();
const cases=[[[0,1]],[[1,.1]],[[2,.5]],[[3,.8]],[[4,.5]],[[5,.7],[7,1]],[[5,.7],[7,1],[6,4]],[[5,.7],[7,.8]],[[5,.7],[8,1]],[[5,.7],[8,1],[9,3]],[[10,.2]],[[11,.3]],[[12,.7]],[[13,0]],[[14,.3]],[[15,.7]],[[16,.2]],[[17,.8]],[[18,.3]],[[19,.8]]];
for(let id=0;id<20;id++){const r=await render(cases[id],[],id===19);const context=cases[id].filter(([parameter])=>parameter!==id);const comparison=id===19?(await render([],[],true)).hash:context.length?(await render(context)).hash:base.hash;assert.notEqual(r.hash,comparison,`logical parameter ${id} affects sound independently`);}
const filters=new Set(),shapes=new Set();
for(let filter=0;filter<5;filter++)filters.add((await render([[13,filter],[14,.6],[15,.8]])).hash);
for(let shape=0;shape<5;shape++)shapes.add((await render([[5,.8],[7,1],[6,shape]])).hash);
assert.equal(filters.size,5);assert.equal(shapes.size,5);
assert.equal((await render([[13,4]],[],false,1)).hash,base.hash,'explicit bypass returns original dry sound');
assert.notEqual((await render([[13,4]],[],false,0)).hash,base.hash,'re-enabled chosen filter affects sound');
const pan=await render([[4,1]]);assert.ok(pan.left<pan.right*.001,'hard right dry slot balance');
const locked=await render([],[[1,.125]]);assert.equal(locked.e.ab_slot_parameter_get(0,1),0,'voice lock leaves base unchanged');assert.notEqual(locked.hash,base.hash);
assert.equal(locked.e.ab_slot_parameter(0,2,NaN),0);assert.equal(locked.e.ab_slot_parameter(0,0,4),0);assert.equal(locked.e.ab_slot_parameter(0,6,1.5),0);
console.log('PASS: all20 controls affect audio; five filter modes/five LFO shapes bounded; fractional edits; voice/base isolation; pan; invalid inputs.');
