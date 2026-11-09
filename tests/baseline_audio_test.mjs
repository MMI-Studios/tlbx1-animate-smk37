// Exact baseline from the owner's earlier Melodee-based Animate adaptation.
// Never regenerate expected hashes automatically. Optional argv[2] is a future
// compatible adapter WASM; the default reads the preserved reference unchanged.
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import { fileURLToPath } from 'node:url';

const here=path.dirname(fileURLToPath(import.meta.url));
const reference=path.resolve(here,'../vendor/melodee');
const wasmPath=process.argv[2]?path.resolve(process.argv[2]):path.join(reference,'web/audio/smk-engine.wasm');
const bytes=fs.readFileSync(wasmPath);
console.log('Audio reference candidate:',wasmPath);
console.log('SHA256:',crypto.createHash('sha256').update(bytes).digest('hex'));
const protocolSource=fs.readFileSync(path.join(reference,'web/reference-protocol.js'),'utf8');
const context={console,setTimeout,clearTimeout};vm.createContext(context);
vm.runInContext(protocolSource+
 ';globalThis.api={req,parse,frame,unframe};',context);
const protocol=context.api;
const {instance:{exports:e}}=await WebAssembly.instantiate(bytes);
e.synth_init();const memory=new Uint8Array(e.memory.buffer);let metadata;
function request(r){
 const data=protocol.frame(...r).slice(1,-1);memory.set(data,e.fw_request());
 const count=e.fw_command(data.length),reply=memory.slice(e.fw_reply(),e.fw_reply()+count);let answer;
 for(let at=0;at<reply.length;){const end=reply.indexOf(247,at);assert.ok(end>=0,'complete protocol frame');
  const f=protocol.unframe(reply.slice(at,end+1));if(f&&f.cmd===r[0])answer=protocol.parse[f.cmd](f.a,metadata);at=end+1;}
 assert.ok(answer,'expected native reply');return answer;
}
function render(blocks){
 let energy=0,peak=0,hash=2166136261;
 for(let i=0;i<blocks;i++){
  const pointer=e.synth_render();
  for(const sample of new Int32Array(e.memory.buffer,pointer,64)){
   energy+=sample*sample;peak=Math.max(peak,Math.abs(sample));hash=Math.imul(hash^sample,16777619)>>>0;
  }
 }
 return {energy,peak,hash};
}
metadata=request(protocol.req.info());
assert.ok(metadata.engines.some(name=>name.startsWith('ANIMATE')));
for(let i=0;i<metadata.pcount;i++)request(protocol.req.desc(0,i));
const cornerHashes=[];
for(let preset=1;preset<=4;preset++){
 e.fw_release();render(4000);request(protocol.req.preset(21,preset));render(30);
 e.fw_event(2,12,110);const sound=render(400);e.fw_event(2,12,0);
 assert.ok(sound.energy>0&&sound.peak<32768,'corner audible and PCM16 bounded');
 cornerHashes.push(sound.hash);console.log('Corner',preset,sound);
}
assert.deepEqual(cornerHashes,[1782912505,2011708317,1877176961,3477105355],
 'preserved full mixer/voice_amp/DC-filter baseline changed; do not regenerate golden values');

// Compare the saved USER VECTOR preset with the owner's actual captured session,
// including source-window and position-LFO settings. This is parameter equality
// plus bounded audio, not an invented historical USER VECTOR audio hash.
function configuration(){const p=e.fw_snapshot(),n=e.fw_snapshot_size();assert.ok(p);return new Int16Array(e.memory.buffer,p+n-4-160-4*92,46).slice();}
function motion(){const p=e.fw_snapshot(),n=e.fw_snapshot_size();return new Int16Array(e.memory.buffer,p+n-4-160,80).slice();}
const owner=JSON.parse(fs.readFileSync(path.join(here,'fixtures/user-vector-session.json'),'utf8'));
memory.set(Buffer.from(owner.data,'base64'),e.fw_snapshot());assert.equal(e.fw_restore(),0,'owner session accepted');
render(40);const ownerParams=Array.from(request(protocol.req.trackDump(0)).p);
const ownerConfiguration=configuration(),ownerMotion=motion().slice(0,20);
const otherParams=Array.from(request(protocol.req.trackDump(1)).p);
request(protocol.req.track(0));request(protocol.req.preset(21,1));render(40);
request(protocol.req.preset(21,5));render(40);
assert.deepEqual(Array.from(request(protocol.req.trackDump(0)).p),ownerParams,'USER VECTOR track settings');
assert.deepEqual(configuration(),ownerConfiguration,'USER VECTOR slot/filter/envelope/vector settings');
assert.deepEqual(motion().slice(0,20),ownerMotion,'USER VECTOR source windows and position LFOs');
assert.deepEqual(Array.from(request(protocol.req.trackDump(1)).p),otherParams,'other part unchanged');
e.fw_event(2,12,100);const userAudio=render(400);assert.ok(userAudio.energy>0&&userAudio.peak<32768);e.fw_release();
render(20000);assert.ok(render(200).peak<=1,'release reaches inherited PCM noise floor');
console.log('USER VECTOR bounded audio:',userAudio);
console.log('PASS: exact four-corner audio baseline; USER VECTOR settings/windows; bounded output and release.');
