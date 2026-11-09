// SPDX-License-Identifier: GPL-3.0-only
// SMK physical panel geometry adapted from Chance Roth's Melodee SMK simulator.
// Animate-only panel projection; snapshots come from its own control/runtime.
// Physical geometry and event ownership retained from the TLBX panel.
export function createHardwarePanel(host,send){
 const el=(tag,cls,text)=>{const n=document.createElement(tag);n.className=cls||'';if(text)n.textContent=text;return n;};
 const now=()=>Math.floor(performance.now());
 const shell=el('div','smk-simulator-shell'),top=el('div','sim-top'),wheels=el('div','sim-wheels'),left=el('div','sim-left'),center=el('div','sim-center'),right=el('div','sim-right');
 const bezel=el('div','sim-bezel'),lcd=el('canvas','sim-lcd');lcd.width=lcd.height=240;lcd.setAttribute('aria-label','Shared TLBX firmware display');lcd.setAttribute('role','img');bezel.append(lcd);left.append(bezel);
 const brand=el('div','sim-brand','SMK-37 PRO');brand.append(el('small','','TLBX ANIMATE'));left.append(brand);
 const ctx=lcd.getContext('2d');ctx.fillStyle='#0a1013';ctx.fillRect(0,0,240,240);ctx.fillStyle='#e3c1ff';ctx.font='14px monospace';ctx.fillText('ENABLE AUDIO TO START',25,120);
 let enabled=false,visible=true,track=0,state,font,splashing=false,splashEnd=null;const owners=new Map(),counts=new Map(),buttons=[],pads=[],keys=[],faders=[],knobs=[],wheelInputs=[];
 function command(kind,id,value,physicalName){if(!enabled)return;if(splashing&&(kind==='button'||kind==='pad')&&value)splashEnd?.();if(kind==='button'){
  send(['button',id,value,now()]);
 }if(kind==='pad')send(['pad',id,value,now()]);if(kind==='key')send(['key',id,value]);if(kind==='sustain')send(['sustain',value?1:0]);}
 function release(owner){const v=owners.get(owner);if(!v)return;owners.delete(owner);let n=counts.get(v.key)-1;if(n)counts.set(v.key,n);else{counts.delete(v.key);command(v.kind,v.id,0);}v.button.setAttribute('aria-pressed',String(n>0));}
 function press(owner,kind,id,velocity,b){if(!enabled||owners.has(owner))return;const key=kind+':'+id,n=counts.get(key)||0;counts.set(key,n+1);owners.set(owner,{key,kind,id,button:b});if(!n)command(kind,id,velocity,b.textContent);b.setAttribute('aria-pressed','true');}
 function wire(b,kind,id,velocity=1){
  b.type='button';b.setAttribute('aria-pressed','false');b.dataset.kind=kind;b.dataset.id=id;
  b.onpointerdown=e=>{if(e.button)return;e.preventDefault();b.focus();b.setPointerCapture(e.pointerId);press('p'+e.pointerId,kind,id,velocity,b);};
  b.onpointerup=b.onpointercancel=b.onlostpointercapture=e=>release('p'+e.pointerId);
  b.onpointermove=e=>{const owner='p'+e.pointerId,current=owners.get(owner);if(!current||!['key','pad'].includes(kind))return;const next=document.elementFromPoint(e.clientX,e.clientY)?.closest(`[data-kind="${kind}"]`);if(next&&host.contains(next)&&next!==current.button){release(owner);press(owner,kind,+next.dataset.id,velocity,next);}};
  b.onkeydown=e=>{if(['Space','Enter'].includes(e.code)){e.preventDefault();e.stopPropagation();if(!e.repeat)press('focus'+e.code,kind,id,velocity,b);}};
  b.onkeyup=e=>{if(['Space','Enter'].includes(e.code)){e.preventDefault();e.stopPropagation();release('focus'+e.code);}};
  b.onblur=()=>{release('focusSpace');release('focusEnter');};
 }
 const encoderRow=el('div','sim-knobs');for(let i=0;i<8;i++){
  const box=el('div','sim-encoder'),dial=el('button','sim-dial'),hint=el('small','','Parameter '+(i+1));dial.setAttribute('aria-label',`Knob ${i+1}`);let angle=0,drag;
  const turn=n=>{if(!enabled)return;send(['encoder',i,n]);angle+=n*8;dial.style.setProperty('--angle',angle+'deg');};
  dial.onpointerdown=e=>{if(e.button)return;e.preventDefault();dial.focus();dial.setPointerCapture(e.pointerId);drag={id:e.pointerId,y:e.clientY};};
  dial.onpointermove=e=>{if(drag?.id!==e.pointerId)return;const n=Math.trunc((drag.y-e.clientY)/5);if(n){turn(n);drag.y-=n*5;}};
  dial.onpointerup=dial.onpointercancel=dial.onlostpointercapture=()=>drag=null;
  dial.onkeydown=e=>{const n={ArrowUp:1,ArrowRight:1,ArrowDown:-1,ArrowLeft:-1}[e.key];if(n){e.preventDefault();e.stopPropagation();turn(n);}};
  dial.addEventListener('wheel',e=>{if(!enabled)return;e.preventDefault();turn(e.deltaY<0?1:-1);},{passive:false});box.append(el('span','',`K${i+1}`),dial,hint);encoderRow.append(box);knobs.push({dial,hint});
 }
 const padGrid=el('div','sim-pads');for(let i=0;i<16;i++){const b=el('button','sim-pad',String(i+1));b.setAttribute('aria-label',`Hardware pad ${i+1}`);wire(b,'pad',i);pads.push(b);padGrid.append(b);}center.append(encoderRow,padGrid);
 const faderRow=el('div','sim-faders');for(let i=0;i<4;i++){const label=el('label','sim-fader',`F${i+1}`),f=el('input');f.type='range';f.min=0;f.max=127;f.value=83;f.setAttribute('aria-label',`Hardware fader ${i+1}`);f.oninput=()=>send(['fader',i,+f.value]);label.append(f);faders.push(f);faderRow.append(label);}right.append(faderRow);
 const banks=el('div','sim-banks'),panel=el('div','sim-buttons');
 const physical=[['KNOB BANK',9],['FADER BANK',10],['◀',14],['▶',15],['OCT−',12],['OCT+',13],['ARP',3],['REPEAT',4],['SCALE',5],['CHORD',6],['GLOBE',7],['BT / SHIFT',0],['PATCH',8],['PARA',9],['FX',10],['SEQ',11],['PLAY',1],['REC',2]];
 physical.forEach(([name,id],i)=>{const b=el('button','sim-button',name);b.setAttribute('aria-label',`Hardware ${name}`);wire(b,'button',id);if(id===4)b.title='Repeat · SHIFT + REPEAT toggles Hold';buttons.push({b,id});(i<6?banks:panel).append(b);});right.append(panel);
 for(const [id,name,min,max]of[[0,'Pitch',-8192,8191],[1,'Mod',0,127]]){
  const label=el('label','sim-wheel',name),housing=el('span','sim-wheel-housing'),tread=el('span','sim-wheel-tread'),input=el('input');input.type='range';input.min=min;input.max=max;input.value=0;input.setAttribute('aria-label',name+' wheel');label.title=id?'Modulation; Animate XY mode controls Y':'Pitch bend; Animate XY mode controls X';
  const paint=()=>housing.style.setProperty('--wheel-position',(12+(1-(+input.value-min)/(max-min))*76)+'%');
  input.oninput=()=>{paint();send(['wheel',id,+input.value]);};
  if(!id){const reset=()=>{input.value=0;input.oninput();};input.onpointerup=input.onpointercancel=input.onblur=reset;}wheelInputs.push({input,paint});paint();housing.append(tread,input);label.append(housing);wheels.append(label);
 }
 const sustain=el('button','sim-button','SUSTAIN');sustain.setAttribute('aria-label','Hardware sustain');wire(sustain,'sustain',0);wheels.append(sustain,banks);
 const piano=el('div','sim-piano');piano.setAttribute('aria-label','37-key SMK keyboard');let white=0;const noteNames=['C','C♯','D','D♯','E','F','F♯','G','G♯','A','A♯','B'];
 for(let i=0;i<37;i++){const n=48+i,black=[1,3,6,8,10].includes(n%12),name=noteNames[n%12]+(Math.floor(n/12)-1),b=el('button','sim-key '+(black?'black':'white'),name);b.setAttribute('aria-label',`Hardware key ${name}`);wire(b,'key',n,100);b.style.setProperty('--key-x',String(black?white-.31:white++));keys.push(b);piano.append(b);}
 top.append(wheels,left,center,right);const bottom=el('div','sim-bottom');bottom.append(piano);shell.append(top,bottom);host.replaceChildren(shell);
 const computer='awsedftgyhujkolp;';
 const down=e=>{if(!enabled||!visible||e.ctrlKey||e.metaKey||e.altKey||e.repeat||e.target.closest('input,select,textarea'))return;if(e.code==='ShiftLeft'||e.code==='ShiftRight'){press('keyboard'+e.code,'button',0,1,buttons.find(x=>x.id===0).b);e.preventDefault();return;}if(e.code==='Space'){if(e.target.closest('button,a'))return;e.preventDefault();press('space','button',1,1,buttons.find(x=>x.id===1).b);return;}if(/^Digit[1-8]$/.test(e.code)){const i=Number(e.code.slice(-1))-1;press('number'+e.code,'pad',i,1,pads[i]);e.preventDefault();return;}const i=computer.indexOf(e.key.toLowerCase());if(i>=0){press('key'+e.code,'key',48+i,100,keys[i]);e.preventDefault();}};
 const up=e=>{release('key'+e.code);release('number'+e.code);if(e.code==='Space')release('space');if(e.code.startsWith('Shift'))release('keyboard'+e.code);};
 document.addEventListener('keydown',down);document.addEventListener('keyup',up);
 function releaseAll(){for(const owner of [...owners.keys()])release(owner);}
 window.addEventListener('blur',releaseAll);document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseAll();});
 function rgb(c){return `rgb(${Math.round(((c>>11)&31)*255/31)},${Math.round(((c>>5)&63)*255/63)},${Math.round((c&31)*255/31)})`;}
 function plot(mode,x,y,w,h,color){const signal=state?.visual,bins=state?.spectrum;if(!signal)return;ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();ctx.strokeStyle=rgb(color);ctx.fillStyle=rgb(color);ctx.lineWidth=1;
  if(mode===1&&bins){for(let i=0;i<32;i++){const height=bins[i]*h/32768;ctx.fillRect(x+i*w/32,y+h-height,Math.max(1,w/32-1),height);}}
  else{ctx.beginPath();for(let i=0;i<128;i++){const l=signal[i*2],r=signal[i*2+1],xx=mode===2?x+w/2+(l-r)*w/(4*32768):x+i*w/127,yy=mode===2?y+h/2-(l+r)*h/(4*32768):y+h/2-l*h/(2*32768);if(i)ctx.lineTo(xx,yy);else ctx.moveTo(xx,yy);}ctx.stroke();}ctx.restore();
 }
 function draw(commands){if(!commands||!font)return;const bytes=new Uint8Array(commands),v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength);for(let p=0;p<bytes.length;p+=56){const type=v.getUint32(p,true),x=v.getUint32(p+4,true),y=v.getUint32(p+8,true),w=v.getUint32(p+12,true),h=v.getUint32(p+16,true),color=v.getUint32(p+20,true);ctx.fillStyle=rgb(color);if(type===0)ctx.fillRect(x,y,w,h);else if(type===2){plot(bytes[p+24],x,y,w,h,color);}else if(type===1){let xx=x;for(let c=p+24;c<p+56&&bytes[c];c++,xx+=12){const ch=bytes[c];if(ch<32||ch>127)continue;for(let col=0;col<5;col++){const bits=font[(ch-32)*5+col];for(let row=0;row<7;row++)if(bits&(1<<row))ctx.fillRect(xx+col*2,y+row*2,2,2);}}}}}
 return {
  enabled(value){if(!value)releaseAll();enabled=value;host.querySelectorAll('button,input').forEach(b=>b.disabled=!value);},
  visible(value){visible=value;if(!value)releaseAll();},release:releaseAll,
  finish(value){shell.dataset.finish=value;},font(bytes){font=new Uint8Array(bytes);},
  /* Boot splash from the same C rasterizer as the SMK LCD (40 frames, 40 ms).
   * A button or pad press ends it early, as on hardware; the UI then repaints. */
  splash(exports,prompt){const render=exports&&exports.af_splash_render;if(!render)return Promise.resolve();
   const image=ctx.createImageData(240,240),out=image.data;let frame=0,timer=0;splashing=true;
   return new Promise(done=>{
    splashEnd=()=>{clearTimeout(timer);splashing=false;splashEnd=null;pads.forEach((b,i)=>b.classList.toggle('lit',!!(state&&state.padLights&(1<<i))));if(state)draw(state.display);else if(prompt){ctx.fillStyle='#080808';ctx.fillRect(0,220,240,20);ctx.fillStyle='#e3c1ff';ctx.font='11px monospace';ctx.textAlign='center';ctx.fillText(prompt,120,233);ctx.textAlign='start';}done();};
    const tick=()=>{if(frame>=40){splashEnd();return;}const px=new Uint16Array(exports.memory.buffer,render(frame),57600);
     for(let i=0,o=0;i<57600;i++,o+=4){const c=px[i];out[o]=((c>>11)&31)*255/31;out[o+1]=((c>>5)&63)*255/63;out[o+2]=(c&31)*255/31;out[o+3]=255;}
     ctx.putImageData(image,0,0);const lit=exports.af_splash_pads?exports.af_splash_pads(frame):0;pads.forEach((b,i)=>b.classList.toggle('lit',!!(lit&(1<<i))));frame++;timer=setTimeout(tick,40);};
    tick();});},
  update(s){
   state=s;const u=s.ui||{};track=u.lane||0;if(!splashing)draw(s.display);
   wheelInputs.forEach((wheel,i)=>{if(document.activeElement!==wheel.input){wheel.input.value=(s.wheels||[0,0])[i];wheel.paint();}});
   pads.forEach((b,i)=>{b.title=(s.padLabels||[])[i]||'Pad '+(i+1);b.classList.toggle('lit',!!(s.padLights&(1<<i)));b.classList.toggle('cursor',u.view===0&&!u.shift&&s.playing&&s.position===u.step_page*16+i);});
   buttons.forEach(({b,id})=>b.classList.toggle('lit',!!(s.buttonLights&(1<<id))));
   faders.forEach((f,i)=>{const label=(s.faderLabels||[])[i]||'Inactive';f.disabled=!enabled||!s.fadersEnabled;f.title=label;f.parentNode.firstChild.textContent='F'+(i+1)+' '+label;if(document.activeElement!==f)f.value=(s.faderValues||[0,0,0,0])[i];});
   knobs.forEach((k,i)=>{k.hint.textContent=(s.knobLabels||[])[i]||'';});
  }
 };
}

