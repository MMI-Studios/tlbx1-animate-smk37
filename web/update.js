// SPDX-License-Identifier: GPL-3.0-only
import {parsePackage,installSmk} from './update-support/installer/smk37-update.js';
import {Updater} from './update-support/installer/fm1ota.js?v=progress1';
import {discoverSmk} from './discovery.js?v=1';
import {enterSelectedLoader} from './loader-entry.js?v=1';
const $=id=>document.getElementById(id);let release,pkg,access,checked=false,busy=false;
const log=s=>{$('status').textContent+=`\n${s}`;};
function controls(){ $('file').disabled=busy||!release;$('check').disabled=busy||!pkg;$('confirm').disabled=busy;$('install').disabled=busy||!pkg||!checked||!$('confirm').checked;$('ports').disabled=busy||!release;$('output').disabled=busy||!access;$('loader').disabled=busy||!pkg||!$('output').value||!$('confirm').checked; }
async function discover(){access ||= await navigator.requestMIDIAccess({sysex:true});return discoverSmk(access,Updater,log);}
async function run(fn){if(busy)return;busy=true;controls();try{await fn();}catch(e){checked=false;log(e.message);}finally{busy=false;controls();}}
$('file').onchange=()=>run(async()=>{pkg=null;checked=false;const file=$('file').files[0];if(!file)return;pkg=await parsePackage(new Uint8Array(await file.arrayBuffer()),release.sha256);if(pkg.product!==release.product||pkg.version!==release.version||pkg.format!==2){pkg=null;throw Error('Wrong release identity');}log('Reviewed portable package verified; no device writes.');});
$('check').onclick=()=>run(async()=>{checked=false;let dev;try{dev=await discover();checked=true;log(`Found ${dev.id.text}. No programming started.`);}finally{await dev?.link.close();}});
$('confirm').onchange=controls;
$('output').onchange=()=>{checked=false;controls();};
$('ports').onclick=()=>run(async()=>{
 access ||= await navigator.requestMIDIAccess({sysex:true});
 $('output').replaceChildren(new Option('Select SMK MIDI output',''));
 for(const p of access.outputs.values())if(p.state!=='disconnected'&&/^(Melodee(?: In)?|SMK-37(?: Pro| Elite)?(?: Midi)?)$/i.test(p.name||''))
  $('output').add(new Option(p.name,p.id));
 checked=false;log('Select the output belonging to your SMK. Refreshing ports sent no MIDI commands.');
});
$('loader').onclick=()=>run(async()=>{checked=false;let dev;try{
 dev=await enterSelectedLoader(access,$('output').value,Updater,log,{confirmed:$('confirm').checked});checked=true;
}finally{await dev?.link.close();}});
$('install').onclick=()=>{if($('install').disabled)return;run(async()=>{const dev=await discover();checked=false;await installSmk(access,pkg,log,{device:dev});log('Installation verified; returned as SMK-37_982.');});};
window.addEventListener('beforeunload',e=>{if(busy){e.preventDefault();e.returnValue='';}});
try{const r=await fetch('update-release.json',{cache:'no-store'});if(!r.ok)throw Error('Release metadata missing');release=await r.json();if(release.product!=='SMK-37_982'||release.version!=='Animate 0.1'||release.bytes!==627712||!/^[a-f0-9]{64}$/.test(release.sha256))throw Error('Unexpected release metadata');if(!navigator.requestMIDIAccess)throw Error('Use Chrome or Edge on localhost.');$('status').textContent=`Expected animate-update.bin · SHA256 ${release.sha256}`;}catch(e){release=null;log(e.message);}controls();
