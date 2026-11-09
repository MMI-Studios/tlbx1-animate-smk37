// SPDX-License-Identifier: GPL-3.0-only
// Adapted from ChanceTheMaker/melodee-smk web/installer/smk37-package.js.
// Preserve its Pro016 layout, protected-byte and serialization checks. Only the
// application release allowlist differs. The Animate baseline shares the preserved host DSP separately.
import {Package,crc16,sha256hex} from '../vendor/melodee/web/installer/fm1pkg.js';
const LO=0x4000,HI=0x9d000;
export const STOCK_SHA='2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0';
/* M-VAVE's official SMK-37 Pro V016 firmware is not distributed with this repository. */
export const STOCK_FILE='assets/stock/SMK-37 Pro_016.fwsc';
export const STOCK_HELP=`M-VAVE's official SMK-37 Pro firmware V016 is required for packaging and is not
included in this repository. Download it from M-VAVE (https://www.m-vave.com/appdownload,
Download > PC Firmware > SMK-37 PRO), keep the file unchanged, and save it as
  ${STOCK_FILE}
Its SHA-256 must be ${STOCK_SHA}. See assets/stock/README.md.`;
const u16=(b,o)=>b[o]|b[o+1]<<8;
const u32=(b,o)=>(b[o]|b[o+1]<<8|b[o+2]<<16|b[o+3]<<24)>>>0;
function enc(b){let key=65535;for(let i=0;i<b.length;i++){b[i]^=key&255;key=((key*2)^((key&32768)?0x1021:0))&65535;}return b;}
function crc32(data){let c=0xffffffff;for(const b of data){c^=b;for(let i=0;i<8;i++)c=(c>>>1)^((0-(c&1))&0xedb88320);}return(c^0xffffffff)>>>0;}
function logical(bytes){
 if(bytes.length<960||bytes.length>32*1024*1024)throw Error('Invalid SMK firmware file size');
 const probe=new Uint8Array(64);for(let i=0;i<64;i++)probe[i]=bytes[Math.floor(i/47)*48+i%47];enc(probe);
 if(crc16(probe.subarray(2))!==u16(probe,0))throw Error('SMK firmware header checksum failed');
 const blocks=bytes.length-u32(probe,4);if(blocks!==36)throw Error('Expected Pro016 wrapper');
 const raw=new Uint8Array(bytes.length-blocks);
 for(let i=0;i<blocks;i++)raw.set(bytes.subarray(i*48,i*48+47),i*47);
 raw.set(bytes.subarray(blocks*48),blocks*47);return {raw,blocks};
}
class SMKPackage extends Package{
 parseUfw(){
  this.container=this.raw.slice();const{raw,blocks}=logical(this.raw);this.blocks=blocks;this.skew=blocks;
  const h=raw.slice();enc(h.subarray(0,64));this.ufw=h;this.nent=u16(h,8);
  if(this.nent<1||this.nent>128||64+this.nent*80>h.length||crc16(h.subarray(64,64+this.nent*80))!==u16(h,2))throw Error('SMK directory checksum failed');
  this.entries=[];const ranges=[];
  for(let off=64;off<64+this.nent*80;off+=80){
   const e=enc(h.slice(off,off+80));const name=new TextDecoder().decode(e.subarray(64,80)).replace(/\0.*$/s,'');
   const item={off,type:u16(e,0),dcrc:u16(e,4),dataOff:u32(e,8),size:u32(e,12),name,plain:e};
   if(item.dataOff<64+this.nent*80||item.dataOff>raw.length||item.size>raw.length-item.dataOff||u32(e,16)<item.size)throw Error('Invalid SMK payload bounds');
   if(ranges.some(([lo,hi])=>item.dataOff<hi&&item.dataOff+item.size>lo))throw Error('Overlapping SMK payloads');
   ranges.push([item.dataOff,item.dataOff+item.size]);
   if([0,2,100,255].includes(item.type)&&crc16(raw.subarray(item.dataOff,item.dataOff+item.size))!==item.dcrc)throw Error('SMK payload checksum failed');
   this.entries.push(item);
  }
  const matches=this.entries.filter(e=>e.type===0&&e.name==='flash.bin');if(matches.length!==1)throw Error('Expected one flash payload');
  this.raw=raw;this.flashEntry=matches[0];this.flashFileOff=this.flashEntry.dataOff;this.flashSize=this.flashEntry.size;
 }
 setProduct(name){
  if(name.length>this.blocks)throw Error('SMK identity too long');
  for(let i=0;i<this.blocks;i++){const v=i<name.length?(name.charCodeAt(i)+i+1)&255:125;if(i<name.length&&v===125)throw Error('Invalid identity');this.container[i*48+47]=v;}
 }
 hdrFileOff(i){return i;}
 serialize(){const raw=super.serialize(),out=this.container.slice();for(let i=0;i<this.blocks;i++)out.set(raw.subarray(i*47,i*47+47),i*48);out.set(raw.subarray(this.blocks*47),this.blocks*48);return out;}
}
export async function buildAnimatePackage(stock,app,release){
 if(await sha256hex(stock)!==STOCK_SHA)throw Error('Requires unmodified official SMK-37 Pro016 package');
 if(release?.product!=='SMK-37_982'||release.version!=='Animate 0.1'||app.length!==release.bytes||await sha256hex(app)!==release.sha256||app.length<4||app.length>0x8dfbc||app.length%4||u32(app,0)!==0x00808104)throw Error('Animate candidate identity/hash/size failed');
 const pkg=new SMKPackage(stock);
 if(pkg.blocks!==36||pkg.flashSize!==HI||pkg.appBase!==LO||pkg.appOff!==0x4120||pkg.app.size!==619704||pkg.chipKey!==0x980f||crc32(pkg.flash.subarray(0,LO))!==0x3a04ad23)throw Error('Unsupported SMK Pro016 flash layout');
 const before=pkg.flash.slice();pkg.setApp(app);
 const allowed=[[LO,LO+4],[pkg.app.hdrOff,pkg.app.hdrOff+4],[pkg.appOff,pkg.appOff+pkg.app.size]];
 for(let i=0;i<before.length;i++)if(before[i]!==pkg.flash[i]&&!allowed.some(([lo,hi])=>i>=lo&&i<hi))throw Error('Protected firmware bytes changed');
 pkg.setProduct('SMK-37 Pro_982');const factory=pkg.serialize(),checked=new SMKPackage(factory);
 if(checked.flash.some((b,i)=>b!==pkg.flash[i]))throw Error('SMK serialized package round-trip mismatch');
 pkg.setProduct('SMK-37_982');const custom=pkg.serialize(),customChecked=new SMKPackage(custom);
 if(customChecked.flash.some((b,i)=>b!==checked.flash[i]))throw Error('Custom identity changed flash payload');
 const HEADER=1024,packet=new Uint8Array(HEADER+HI-LO),h=new DataView(packet.buffer);
 packet.set(new TextEncoder().encode('SMK37-SALT-USB1'));
 [2,LO,HI,HI-LO,crc32(before.subarray(0,LO)),0,crc32(pkg.flash.subarray(LO,HI))].forEach((v,i)=>h.setUint32(16+i*4,v,true));
 packet.set(new TextEncoder().encode(release.product),48);packet.set(new TextEncoder().encode(release.version),80);
 packet.set(pkg.flash.subarray(LO,HI),HEADER);
 for(let i=0;i<(HI-LO)/4096;i++)h.setUint32(128+i*4,crc32(packet.subarray(HEADER+i*4096,HEADER+(i+1)*4096)),true);
 h.setUint32(44,crc32(packet.subarray(0,HEADER)),true);
 return {factory,custom,packet};
}
export async function inspectStockPatchRegion(stock){
 if(await sha256hex(stock)!==STOCK_SHA)throw Error('Requires unmodified official Pro016');
 const pkg=new SMKPackage(stock);
 return [[0x9d000,0xbd000],[0xfe000,0xff000]].map(([start,end])=>({start,end,available:pkg.flash.length>=end,nonErased:pkg.flash.length>=end?pkg.flash.subarray(start,end).reduce((n,b)=>n+(b!==255),0):null}));
}
