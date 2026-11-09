import assert from 'node:assert/strict';
import {discoverSmk} from '../web/discovery.js';
const port={name:'SMK',state:'connected',connection:'closed'};
const access={sysexEnabled:true,inputs:new Map([['i',port]]),outputs:new Map([['o',port]])};
let calls=0,reply=null,after=1;
class Probe{async findAny(filter,options){calls++;assert.equal(options.probeMs,2000);return calls>=after&&reply&&filter(reply)?{id:reply}:null;}}
const run=a=>discoverSmk(a,Probe,()=>{});
await assert.rejects(run({...access,inputs:new Map()}),/port-enumeration/);assert.equal(calls,0);
await assert.rejects(run({...access,sysexEnabled:false}),/SysEx permission/);assert.equal(calls,0);
await assert.rejects(run(access),/no supported identity reply/);assert.equal(calls,2);
calls=0;reply={model:'SMK-37 Pro',version:16,text:'SMK-37 Pro_016'};
await assert.rejects(run(access),/SMK-37 Pro_016/);
calls=0;after=2;reply={model:'SMK-37',version:982,text:'SMK-37_982'};
assert.equal((await run(access)).id,reply);assert.equal(calls,2);
calls=0;after=1;reply={model:'ota-SMK-37',version:982,text:'ota-SMK-37_982'};
assert.equal((await run(access)).id,reply);assert.equal(calls,1);
console.log('PASS discovery: missing ports, SysEx, no reply, rejected identity, retry and loader');
