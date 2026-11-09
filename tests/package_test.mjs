// Offline packaging checks. No device access or output package writes.
import fs from 'node:fs/promises';
import assert from 'node:assert/strict';
import {buildAnimatePackage,STOCK_FILE,STOCK_HELP} from '../tools/smk-package.mjs';
const root=new URL('../',import.meta.url);
try{await fs.access(new URL(STOCK_FILE,root));}catch{console.log(`SKIP packaging checks: ${STOCK_FILE} is not present.\n${STOCK_HELP}`);process.exit(0);}
const app=new Uint8Array(await fs.readFile(new URL('build/smk/animate-candidate.bin',root)));
const stock=new Uint8Array(await fs.readFile(new URL(STOCK_FILE,root)));
const manifest=JSON.parse(await fs.readFile(new URL('build/smk/manifest.json',root),'utf8'));
const packaged=await buildAnimatePackage(stock,app,manifest);
assert.ok(packaged.factory.length>app.length);
assert.ok(packaged.packet.length>app.length);
assert.equal(packaged.custom.length,packaged.factory.length);
const identity=b=>Array.from({length:36},(_,i)=>b[i*48+47]).map((v,i)=>v===125?'':String.fromCharCode((v-i-1)&255)).join('');
assert.equal(identity(packaged.factory),'SMK-37 Pro_982');
assert.equal(identity(packaged.custom),'SMK-37_982');
for(let i=0;i<packaged.factory.length;i++)if(i>=36*48||i%48!==47)assert.equal(packaged.custom[i],packaged.factory[i],`Non-identity byte changed at ${i}`);
const corruptApp=app.slice();corruptApp[corruptApp.length-1]^=1;
await assert.rejects(buildAnimatePackage(stock,corruptApp,manifest),/identity\/hash\/size/);
const corruptStock=stock.slice();corruptStock[corruptStock.length-1]^=1;
await assert.rejects(buildAnimatePackage(corruptStock,app,manifest),/unmodified official/);
await assert.rejects(buildAnimatePackage(stock,app,{...manifest,product:'SMK-37_981'}),/identity\/hash\/size/);
await assert.rejects(buildAnimatePackage(stock,app,{...manifest,version:'other'}),/identity\/hash\/size/);
await assert.rejects(buildAnimatePackage(stock,app.subarray(0,app.length-1),manifest),/identity\/hash\/size/);
console.log('Animate packaging: protected-byte reconstruction and corrupt source/candidate/identity rejection passed. No device access.');
