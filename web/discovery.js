// SPDX-License-Identifier: GPL-3.0-only
export async function discoverSmk(access,Updater,log){
 const ports=kind=>[...access[kind].values()].filter(p=>p.state!=='disconnected');
 const inputs=ports('inputs'),outputs=ports('outputs');
 log(`MIDI discovery: ${inputs.length} input(s), ${outputs.length} output(s); SysEx ${access.sysexEnabled?'enabled':'not enabled'}.`);
 for(const [kind,list] of [['Input',inputs],['Output',outputs]])
  for(const p of list)log(`${kind}: ${p.name||'(unnamed)'} · ${p.state} · ${p.connection}`);
 if(!access.sysexEnabled)throw Error('Browser MIDI access does not include SysEx. Firmware identity queries require SysEx permission. No programming started.');
 if(!inputs.length||!outputs.length)throw Error('The browser cannot see both a MIDI input and output. This is a port-enumeration problem, not a rejected firmware package. No programming started.');
 const rejected=new Set();
 const accepts=id=>{
  const ok=['SMK-37','ota-SMK-37'].includes(id.model)&&Number.isInteger(id.version)&&id.version>=922;
  if(!ok)rejected.add(id.text||`${id.model}_${id.version}`);
  return ok;
 };
 const updater=new Updater(access,log);
 for(let attempt=1;attempt<=2;attempt++){
  log(`Identity-only probe ${attempt}/2; allowing 2 seconds per output. No programming started.`);
  const dev=await updater.findAny(accepts,{probeMs:2000,openMs:2000});
  if(dev)return dev;
 }
 if(rejected.size)throw Error(`Firmware replied, but no supported custom loader identity matched: ${[...rejected].join(', ')}. No programming started.`);
 throw Error('MIDI ports are visible, but no supported identity reply was received after two probes. Port-open failures, if any, are logged above. This does not establish that another app owns the port. No programming started.');
}
