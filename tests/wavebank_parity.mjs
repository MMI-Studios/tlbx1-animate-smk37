// Bank frames must equal what the wavetable browser's "256 samples / frame, 16-bit"
// preview (normalized) produced, scaled to the original-wave peak (18700).
// WAV decoding is taken from tools/wavetable-browser.html itself.
import fs from 'node:fs';import path from 'node:path';import {fileURLToPath} from 'node:url';import assert from 'node:assert/strict';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const html=fs.readFileSync(path.join(root,'tools/wavetable-browser.html'),'utf8');
const js=html.slice(html.indexOf('function parseChunks'),html.indexOf('/* ---------- Audio engine'));
const {parseChunks,decodeSamples}=new Function(js+'\nreturn {parseChunks,decodeSamples};')();
const cur=JSON.parse(fs.readFileSync(path.join(root,'assets/wavetable-curation.json')));
const src=path.join(root,'assets/factory');
const H=fs.readFileSync(path.join(root,'src/generated/wave_bank.h'),'utf8');
const body=H.slice(H.indexOf('ak_bank_data['));const data=Int16Array.from(body.slice(body.indexOf('{')+1,body.indexOf('};')).split(',').filter(x=>x.trim()).map(Number));
let first=0,checked=0;
for(const it of cur.items){const b=fs.readFileSync(path.join(src,it.path));const ab=b.buffer.slice(b.byteOffset,b.byteOffset+b.length);const h=parseChunks(ab);const s=decodeSamples(ab,h);
 const fsz=it.frameSize,fc=it.frameCount,d=new Float32Array(256*fc);
 for(let k=0;k<fc;k++){const base=k*fsz;for(let j=0;j<256;j++){const x=j*fsz/256,i0=Math.floor(x),q=x-i0,a=s[base+i0],c=s[base+((i0+1)%fsz)];d[k*256+j]=a+(c-a)*q;}}
 let pk=0;for(const v of d)pk=Math.max(pk,Math.abs(v));if(pk>1e-6)for(let i=0;i<d.length;i++)d[i]/=pk;
 for(let i=0;i<d.length;i++){const code=Math.round(Math.max(-1,Math.min(1,d[i]))*32767);assert.equal(data[first*256+i],Math.round(code*18700/32767)+0,`${it.path} sample ${i}`);checked++;}
 first+=fc;}
assert.equal(first*256,data.length);
console.log(`PASS wave bank matches the browser preview exactly: ${cur.items.length} tables, ${checked} samples`);
