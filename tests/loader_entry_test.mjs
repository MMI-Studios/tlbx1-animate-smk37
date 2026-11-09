import assert from 'node:assert/strict';
import {enterSelectedLoader} from '../web/loader-entry.js';
let sends=[],opened=0,closed=0,reply=true;
const port={name:'Melodee',state:'connected',async open(){opened++;},async close(){closed++;},send(x){sends.push(x);}};
const access={sysexEnabled:true,outputs:new Map([['smk',port],['ssl',{...port,name:'SSL 12 MIDI'}]])};
class Probe{async waitForAny(filter,ms){assert.equal(ms,30000);assert(!filter({model:'SMK-37',version:982}));assert(!filter({model:'ota-FM-1',version:982}));assert(!filter({model:'ota-SMK-37',version:921}));const id={model:'ota-SMK-37',version:982,text:'ota-SMK-37_982'};return reply&&filter(id)?{id}:null;}}
const run=(id='smk',confirmed=true)=>enterSelectedLoader(access,id,Probe,()=>{},{confirmed});
await assert.rejects(run('smk',false),/Confirm/);
await assert.rejects(run('missing'),/no longer/);
await assert.rejects(run('ssl'),/Select/);
assert.equal(sends.length,0);
assert.equal((await run()).id.model,'ota-SMK-37');
assert.deepEqual(sends,[[0xf0,0x22,0x24,0x35,0x7f,0xf7]]);
assert.equal(opened,1);assert.equal(closed,1);
reply=false;await assert.rejects(run(),/No programming started/);
assert.equal(sends.length,2);assert.equal(closed,2);
console.log('PASS explicit selected-port entry, confirmation, wrong-port rejection, loader identity gate and no-reply failure');
