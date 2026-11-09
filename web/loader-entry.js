// SPDX-License-Identifier: GPL-3.0-only
// Explicit user action only. Never broadcast a mode switch or program flash.
export async function enterSelectedLoader(access,outputId,Updater,log,{confirmed=false}={}){
 if(!confirmed)throw Error('Confirm that your sound is saved before starting the device updater.');
 if(!access.sysexEnabled)throw Error('SysEx permission is required.');
 const port=access.outputs.get(outputId);
 if(!port||port.state==='disconnected')throw Error('Selected MIDI output is no longer connected. Refresh MIDI ports.');
 if(!/^(Melodee(?: In)?|SMK-37(?: Pro| Elite)?(?: Midi)?)$/i.test(port.name||''))
  throw Error('Select the Melodee or SMK-37 output belonging to your custom SMK.');
 await port.open();
 try{port.send([0xf0,0x22,0x24,0x35,0x7f,0xf7]);}
 finally{try{await port.close();}catch(e){log('Port disconnected during updater entry: '+e.message);}}
 log(`Requested the built-in updater on ${port.name}. Sound stops; no firmware data has been sent.`);
 const up=new Updater(access,log);
 const dev=await up.waitForAny(id=>id.model==='ota-SMK-37'&&Number.isInteger(id.version)&&id.version>=922,30000);
 if(!dev)throw Error('The device updater did not reply. No programming started. Do not repeat installation blindly; save this log.');
 log(`Verified ${dev.id.text}. Ready for Install / resume; no programming started.`);
 return dev;
}
