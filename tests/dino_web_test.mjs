// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync('web/editor.html','utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const E=vm.runInNewContext(proto+';({makeMockDevice,CMD,req,parse,Link,frame,unframe,strEnc,backupCapture,backupRestore,bank,crc32,b64enc})',
  {setTimeout,clearTimeout,setInterval,clearInterval,console,TextEncoder,TextDecoder,Blob,Response,DecompressionStream});
const mock=E.makeMockDevice({dino:true,auto:false});
const input=[...mock.access.inputs.values()][0],output=[...mock.access.outputs.values()][0];
const link=new E.Link(d=>output.send(d),{timeout:300});
input.onmidimessage=e=>link.receive(e.data);
const request=(op,...args)=>link.request([E.CMD.DINO,[op,...args]]).then(E.parse[E.CMD.DINO]);
const info=E.parse[E.CMD.INFO](await link.request(E.req.info()));
assert.match(info.version,/^DINO SYNTH 0\.2/);assert.equal(info.proto,13);assert.equal(info.nengines,12);
let state=await request(0);
assert.equal(state.mode,0);assert.equal(state.split,60);assert.equal(state.style,12);
assert.equal(state.slots.length,4);
for(let engine=0;engine<info.nengines;engine++){
  await link.request(E.req.preset(engine,0));
  const dump=E.parse[E.CMD.DUMP](await link.request(E.req.dump()),info);
  assert.deepEqual(Array.from(dump.p.slice(33,37)),[0,0,0,0],`engine ${engine} starts dry`);
}
state=await request(1,2,65,1,15);
assert.equal(state.mode,2);assert.equal(state.split,65);assert.equal(state.focus,1);assert.equal(mock.state.sel,2);
state=await request(3,1,1);assert.equal(state.status,2);assert.equal(state.slots[1].kind,1);
await request(5,1,...E.strEnc('COSMIC SPLIT'));
await request(1,0,60,0,12);
state=await request(2,1);
assert.equal(state.mode,2);assert.equal(state.split,65);assert.equal(state.style,15);
assert.equal(state.slots[1].name,'COSMIC SPLIT');
assert.equal((await request(1,4,60,0,12)).rc,1,'reject invalid mode');
state=await request(1,2,65,1,16);
assert.equal(state.rc,1,'reject style 16');assert.equal(state.style,15,'invalid style keeps the previous choice');
assert.equal((await request(2,0)).rc,2,'an empty memory cannot load');
for(const cmd of [E.CMD.STEP_SET,E.CMD.PROJECT,E.CMD.LOCK_SET,E.CMD.FILL_SET]){
  const response=await new Promise(resolve=>{
    input.onmidimessage=e=>resolve(E.unframe(e.data).a);
    output.send(E.frame(cmd,[]));
  });
  input.onmidimessage=e=>link.receive(e.data);
  assert.equal(response[0],6,'retired command is rejected');
}
await link.request(E.req.live(1,1));assert.equal(mock.state.playing,false,'legacy LIVE PLAY cannot start patterns');
await link.request(E.req.live(4));assert.equal(mock.state.rec,0,'legacy REC cannot record');
const archive=await E.backupCapture((r,o)=>link.request(r,o),info);
await request(1,0,42,0,12);
await E.backupRestore((r,o)=>link.request(r,o),archive);
state=await request(0);assert.equal(state.split,65,'backup restores live setup');
assert.equal(state.style,15,'backup restores CONWAY DREAM');

// Incompatible archives must fail before the first bank is written, including files without firmware metadata.
function archiveWith(bytes) {
  const objects=[[0,bytes],[1,new Uint8Array([1])],[6,new Uint8Array([99])]].map(([id,data])=>
    ({id,len:data.length,crc:E.crc32(data),data:E.b64enc(data)}));
  return {format:'sloop-backup',version:1,objects};
}
function infoReply(version) {
  return [...E.strEnc(version),12,71,49,64,63,...Array.from({length:12},()=>E.strEnc('ENGINE')).flat(),4,13];
}
async function rejectsBeforeWrite(bytes,version) {
  return rejectsFileBeforeWrite(archiveWith(new Uint8Array(bytes)),version);
}
async function rejectsFileBeforeWrite(file,version) {
  const commands=[];
  await assert.rejects(E.backupRestore(async([cmd])=>{
    commands.push(cmd);
    assert.equal(cmd,E.CMD.INFO,'no bank, project, or sample writes before compatibility validation');
    return infoReply(version);
  },file),e=>e.code==='bkNewer');
  assert.deepEqual(commands,[E.CMD.INFO]);
}
for(let format=1;format<=6;format++)await rejectsBeforeWrite([0x30+format,0x4e,0x55,0x46],'DINO SYNTH 0.1');
for(const magic of [[0x44,0x49,0x4e,0x31],[0x44,0x49,0x41,0x31]])
  await rejectsBeforeWrite(magic,'FELUCCA SLOOP 2.7');
for(const magic of [[0x44,0x49,0x4e,0x31],[0x44,0x49,0x41,0x31]]) {
  const record=new Uint8Array(34);record.set(magic);record[33]=15;
  const file=archiveWith(record),commands=[];
  await assert.rejects(E.backupRestore(async(r,o)=>{
    commands.push(r[0]);
    const reply=await link.request(r,o);
    return r[0]===E.CMD.INFO?infoReply('DINO SYNTH 0.1'):reply;
  },file),e=>e.code==='bkNewer');
  assert.deepEqual(commands,[E.CMD.INFO],'CONWAY DREAM is rejected before BK_BEGIN, including metadata-free records');
  record[33]=16;await rejectsFileBeforeWrite(archiveWith(record),'DINO SYNTH 0.1');
  for(const [style,target] of [[14,'DINO SYNTH 0.1'],[14,'DINO SYNTH 0.2'],[15,'DINO SYNTH 0.2']]) {
    record[33]=style;const compatible=archiveWith(record);compatible.firmware='DINO SYNTH 0.1';
    const written=[];
    await E.backupRestore(async([cmd,args])=>{
      if(cmd===E.CMD.INFO)return infoReply(target);
      assert.equal(cmd,E.CMD.BK_PUT);written.push(args[1]);return [args[0],args[1],0];
    },compatible);
    assert(written.includes(0),'compatible visual records pass preflight and restore');
  }
}
const preservedLegacy=archiveWith(new Uint8Array([0x44,0x49,0x41,0x31]));
const legacySlot=format=>{const data=new Uint8Array([0x30+format,0x4e,0x55,0x46]);
  return {id:2,len:data.length,crc:E.crc32(data),data:E.b64enc(data)};};
preservedLegacy.objects.push(legacySlot(6));
const restored=[];
await E.backupRestore(async([cmd,args])=>{
  if(cmd===E.CMD.INFO)return infoReply('DINO SYNTH 0.1');
  assert.equal(cmd,E.CMD.BK_PUT);restored.push(args[1]);return [args[0],args[1],0];
},preservedLegacy);
assert(restored.includes(2),'DINO backups may preserve a legacy FUN6 memory without playing it');
for(let format=1;format<6;format++) {
  preservedLegacy.objects[preservedLegacy.objects.length-1]=legacySlot(format);
  await rejectsFileBeforeWrite(preservedLegacy,'DINO SYNTH 0.1');
}

// Execute the actual browser helpers, not just their protocol mock or syntax.
class Element {
  constructor(attrs={}){Object.assign(this,{value:'',children:[],dataset:{},listeners:{}},attrs);}
  replaceChildren(...children){this.children=children;}
  addEventListener(name,callback){(this.listeners[name]??=[]).push(callback);}
  get selectedOptions(){return this.children.filter(child=>String(child.value)===String(this.value));}
}
const elements=new Map(),windowEvents=new Map(),documentEvents=new Map(),sent=[];
const element=id=>{if(!elements.has(id))elements.set(id,new Element());return elements.get(id);};
element('dino-kind').value='0';element('dino-slot').value='0';
let wrapped=0,reloaded=0,bankRendered=0;
const dump=E.parse[E.CMD.DUMP](await link.request(E.req.dump()),info);
dump.g[14]=0;dump.g[33]=1;dump.g[24]=10;
const device={info,link,input,output:{send:data=>{sent.push(Array.from(data));output.send(data);}},dump,sel:2,
  dino:await request(4),bank:await E.bank.list((r,o)=>link.request(r,o))};
const context={dev:device,CMD:E.CMD,parse:E.parse,req:E.req,bank:E.bank,strEnc:E.strEnc,v14enc:v=>[v&127,v>>7],
  rq:(r,o)=>link.request(r,o),op:async action=>{wrapped++;return action();},afterSoundChange:async()=>{reloaded++;},
  isDino:()=>!!context.dev?.info?.version.startsWith('DINO SYNTH'),needReload:false,
  $:element,el:(_tag,attrs,...children)=>{const node=new Element(attrs);node.children=children;return node;},
  document:{activeElement:null,hidden:false,addEventListener:(name,callback)=>documentEvents.set(name,callback)},
  window:{addEventListener:(name,callback)=>windowEvents.set(name,callback)},setInterval:()=>0,
  fm6Names:()=>{},renderBank:()=>{bankRendered++;},renderLib:()=>{},MOCK:false,localAudio:null,
  startPreview:async()=>false,report:error=>{throw error;},confirm:()=>true,busy:0,
  stopPreview:()=>{},renderPanels:()=>{},renderMixer:()=>{},renderSysinfo:()=>{},renderLive:()=>{},
  setLabel:()=>{},t:key=>key,sayK:()=>{},console};
const begin=html.indexOf('/* DINO performance uses'),end=html.indexOf('\n$("connect").addEventListener',begin);
const disconnect=html.slice(html.indexOf('function disconnect('),html.indexOf('\nfunction onState(',html.indexOf('function disconnect(')));
const U=vm.runInNewContext(html.slice(begin,end)+'\n'+disconnect+
  ';({dinoWrite,dinoRead,dinoMidiChannel,dinoKeyOn,dinoKeyOff,dinoAllOff,dinoHeld,dinoBend,dinoCenter,disconnect})',context);
assert.equal(element('dino-visual').children.length,16,'all sixteen visual styles are offered');
assert.equal(element('dino-visual').children[15].text,'CONWAY DREAM');
await U.dinoWrite(1,1,65,1,15);
assert.equal(wrapped,1,'DINO writes use the operation wrapper instead of shadowing it');
assert.equal(reloaded,1);assert.equal(device.dino.mode,1);assert.equal(device.sel,2);
assert.equal(device.dino.style,15,'the browser helper can select CONWAY DREAM');
device.dino.mode=3;assert.equal(U.dinoMidiChannel(),9,'drums use DR CH');
device.dump.g[14]=7;assert.equal(U.dinoMidiChannel(),5,'explicit receive channel overrides DR CH');
device.dump.g[14]=1;assert.equal(U.dinoMidiChannel(),null,'CLOCK only has no note channel');
device.dump.g[14]=0;device.dump.g[24]=0;assert.equal(U.dinoMidiChannel(),null,'OFF stays OFF');
device.dump.g[24]=10;device.dino.mode=0;device.dump.g[33]=4;assert.equal(U.dinoMidiChannel(),3);
device.dump.g[33]=1;
await U.dinoWrite(3,0,4);
assert(device.bank.slots[4].used,'completed SAVE refreshes the USER bank');
assert.match(element('dino-slot').children[4].text,/USER 05/);
assert(!element('dino-slot').children[4].text.includes('EMPTY'),'SAVE destination reflects committed patch');
assert(bankRendered>0);
const before=bankRendered;
context.rq=async(r,o)=>{
  const reply=await link.request(r,o);
  if(r[0]===E.CMD.DINO&&r[1][0]===3){const pending=Array.from(reply);pending[7]=1;return pending;}
  return reply;
};
await U.dinoWrite(3,0,5);
assert(device.bank.slots[5].used&&bankRendered>before,'pending-to-saved polling refreshes the bank');
context.rq=(r,o)=>link.request(r,o);
await request(1,0,60,0,12);context.needReload=false;await U.dinoRead();
assert(context.needReload,'front-panel focus change schedules the sound controls reload');
await U.dinoKeyOn('old-device',60);
const oldOutput=device.output,newSent=[];
device.output={send:data=>newSent.push(Array.from(data))};
U.dinoKeyOff('old-device');
assert(sent.some(data=>data[0]===0x80&&data[1]===60),'note-off uses its original output');
assert.equal(newSent.length,0);device.output=oldOutput;
U.dinoBend(2048);device.dump.g[14]=1;U.dinoCenter();device.dump.g[14]=0;
assert.deepEqual(sent[sent.length-1],[0xe0,0,64],'bend release centers its original channel after RX becomes CLOCK only');
await U.dinoKeyOn('hidden',61);context.document.hidden=true;documentEvents.get('visibilitychange')();
assert.equal(U.dinoHeld.size,0,'hiding the page releases DINO notes');context.document.hidden=false;
await U.dinoKeyOn('disconnect',62);U.disconnect();
assert.equal(context.dev,null);assert.equal(U.dinoHeld.size,0,'disconnect releases and clears virtual notes');
assert(sent.some(data=>data[0]===0x80&&data[1]===62));
link.close();
const scripts=[...html.matchAll(/<script[^>]*>([\s\S]*?)<\/script>/g)].map(x=>x[1]);
for(const source of scripts)new vm.Script(source);
assert(html.includes('id="dino-mode"')&&html.includes('id="dino-save"'));
console.log('DINO web: dry engines, direct modes, UI handlers/channels, SAVE bank refresh, recall, zero-write backup preflight, held-note release, retired commands and JS syntax PASS');
