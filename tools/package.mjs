// SPDX-License-Identifier: GPL-3.0-only
// Offline packaging only. Exact reviewed application and official Pro016 input.
import fs from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';
import {buildAnimatePackage,STOCK_SHA,STOCK_FILE,STOCK_HELP} from './smk-package.mjs';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const build=path.join(root,'build/smk');
const sha=b=>createHash('sha256').update(b).digest('hex');
if(process.argv.length!==2)throw Error('No candidate overrides accepted');
const readJSON=async p=>JSON.parse(await fs.readFile(p,'utf8'));
const [manifest,review,loaderManifest]=await Promise.all([
 readJSON(path.join(build,'manifest.json')),readJSON(path.join(root,'tools/package-review.json')),
 readJSON(path.join(build,'smk37-loader-manifest.json'))]);
try{await fs.access(path.join(root,STOCK_FILE));}catch{console.error(`Missing ${STOCK_FILE}.\n${STOCK_HELP}`);process.exit(2);}
const [app,stock,loader]=await Promise.all([
 fs.readFile(path.join(build,'animate-candidate.bin')),
 fs.readFile(path.join(root,STOCK_FILE)),
 fs.readFile(path.join(build,'smk37-loader.bin'))]);
assert.equal(manifest.product,'SMK-37_982');assert.equal(manifest.version,'Animate 0.1');
assert.equal(manifest.installable,false);assert.equal(manifest.hardware_tested,false);
assert.equal(review.product,manifest.product);assert.equal(review.version,manifest.version);
assert.equal(review.sha256,manifest.sha256);assert.equal(review.bytes,manifest.bytes);
assert.equal(sha(app),review.sha256);assert.equal(app.length,review.bytes);
if(sha(stock)!==STOCK_SHA){console.error(`${STOCK_FILE} is not the expected file (SHA-256 ${sha(stock)}).\n${STOCK_HELP}`);process.exit(2);}assert.equal(sha(loader),loaderManifest.sha256);
assert.equal(loader.length,loaderManifest.bytes);assert.equal(loaderManifest.entry,'0x1c0a800');
assert.equal(review.loader_sha256,loaderManifest.sha256);
assert.notEqual(app.indexOf(loader),-1,'RAM updater absent from linked application');
assert.ok(parseInt(manifest.bss_end,16)<=0x1c20000,'Static SRAM boundary exceeded');
assert.ok(manifest.resource_bytes>=152*1024,'Original audio arena reserve exhausted');
const {factory,custom,packet}=await buildAnimatePackage(new Uint8Array(stock),new Uint8Array(app),manifest);
const name='Animate-SMK-37-Pro_982.fwsc';
await fs.writeFile(path.join(build,name),factory);
await fs.writeFile(path.join(build,'Animate-SMK-37_982.fwsc'),custom);
await fs.writeFile(path.join(build,'animate-update.bin'),packet);
const report={product:manifest.product,version:manifest.version,packaged:true,hardware_tested:false,
 application_sha256:sha(app),loader_sha256:sha(loader),stock_sha256:sha(stock),
 factory:{file:name,bytes:factory.length,sha256:sha(factory)},
 custom:{file:'Animate-SMK-37_982.fwsc',bytes:custom.length,sha256:sha(custom)},
 portable:{file:'animate-update.bin',bytes:packet.length,sha256:sha(packet)},
 limitations:['New Animate application has not been tested on physical hardware.',
 'Sixteen on-device sound-patch slots are implemented; sequence/project storage remains browser-only.',
 'Nonblank unowned storage requires a saved and verified backup plus explicit initialization; flash writes pause audio.',
 'M-UPGRADE can time out after reboot because runtime identity is generic SMK-37_982.',
 'Interrupted application programming can require a hardware recovery adapter.']};
await fs.writeFile(path.join(build,'package-report.json'),JSON.stringify(report,null,2)+'\n');
const donor=path.join(root,'vendor/melodee/web');
for(const file of ['installer/smk37-update.js','installer/fm1ota.js','installer/smk-diagnostics.js','update-progress.js']){
 const target=path.join(root,'web/update-support',file);await fs.mkdir(path.dirname(target),{recursive:true});await fs.copyFile(path.join(donor,file),target);
}
await fs.writeFile(path.join(root,'web/update-release.json'),JSON.stringify({...report.portable,product:manifest.product,version:manifest.version},null,2)+'\n');
console.log(JSON.stringify(report,null,2));
