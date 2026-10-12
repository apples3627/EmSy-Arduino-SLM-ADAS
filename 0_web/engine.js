/* Educational state machine with frozen real neural weights. Offline; no device APIs. */
(function(root){
'use strict';
const F=Math.fround, labels=['FORWARD','REVERSE','STOP','LIGHT_ON','LIGHT_OFF','WIPER_ON','WIPER_OFF','UNKNOWN','HORN_ON','HORN_OFF'];
const guards=new Set('after also and are because before cannot followed how if is likes maybe might neither never no nor not only or plus said should then unless until was what when where whether while why without wonder yesterday'.split(' '));
const off=()=>({drive:0,lights:false,wiper:false,horn:false});
function parse(text,model){
 if(text.length>96)return {reason:'clause_too_long'};
 if(!text.length)return {reason:'empty'};
 if(/[^\x20-\x7e]/.test(text))return {reason:'unsupported'};
 const words=text.toLowerCase().replace(/ +$/,'').replace(/[.!?]+$/,'').trim().split(/ +/);
 if(words.length>16)return {reason:'too_many_tokens'};
 if(!words[0])return {reason:'empty'};
 if(words.some(w=>!/^[a-z]+$/.test(w)))return {reason:'unsupported'};
 const ids=words.map(w=>model.vocabulary.indexOf(w));
 if(ids.some(i=>i<0))return {reason:'oov',words};
 let polite=words.slice(['please','kindly','just'].includes(words[0])?1:0);
 const likeOK=(polite.length>=6&&['i','we'].includes(polite[0])&&polite.slice(1,5).join(' ')==='would like you to')||(polite.length>=5&&polite.slice(0,4).join(' ')==='would you like to');
 const guard=words.some((w,i)=>guards.has(w)||(w==='like'&&!likeOK)||(w==='once'&&(i===0||words[i-1]!=='at')));
 return {reason:'ok',words,ids,guard};
}
function infer(text,model){
 const p=parse(text,model);if(p.reason!=='ok'||p.guard)return {...p,effective:7,reason:p.guard?'safety_guard':p.reason,scores:[],probabilities:[]};
 const pooled=new Float32Array(16),z=new Float32Array(model.c);
 for(const id of p.ids)for(let k=0;k<16;k++)pooled[k]=F(pooled[k]+F(model.e[id*16+k]*model.se));
 for(let k=0;k<16;k++)pooled[k]=F(pooled[k]/p.ids.length);
 for(let j=0;j<64;j++){
  let h=F(model.b[j]);for(let k=0;k<16;k++)h=F(h+F(pooled[k]*F(model.w[k*64+j]*model.sw)));
  h=Math.max(0,h);for(let t=0;t<10;t++)z[t]=F(z[t]+F(h*F(model.o[j*10+t]*model.so)));
 }
 let best=0;for(let t=1;t<10;t++)if(z[t]>z[best])best=t;
 const probabilities=Array.from(z,v=>F(Math.exp(F(v-z[best]))));let sum=0,second=0;
 probabilities.forEach((v,i)=>{sum=F(sum+v);if(i!==best&&v>second)second=v;});
 const confidence=F(1/sum),margin=F(F(1-second)/sum);const reason=best===7?'unknown':confidence<model.threshold||margin<model.margin?'low_confidence':'ok';
 return {...p,scores:Array.from(z),probabilities:probabilities.map(v=>v/sum),raw:best,effective:reason==='ok'?best:7,confidence,margin,reason};
}
function split(text){
 if(!text||/[^\x20-\x7e]/.test(text))return {reason:'bad_input',clauses:[]};
 if(text.length>192)return {reason:'too_long',clauses:[]};
 const chunks=[],lower=text.toLowerCase();let start=0,i=0;
 const word=(pos,w)=>lower.slice(pos,pos+w.length)===w&&(pos===0||lower[pos-1]===' ')&&(pos+w.length===lower.length||lower[pos+w.length]===' ');
 while(i<text.length){
  const punct=',;'.includes(text[i]);let k=punct?1:word(i,'and')?3:word(i,'also')||word(i,'plus')?4:0;
  if(!k){i++;continue;}const part=text.slice(start,i).trim();if(!part)return {reason:'empty_clause',clauses:[]};if(part.length>96)return {reason:'clause_too_long',clauses:[]};if(chunks.length===4)return {reason:'too_many',clauses:[]};chunks.push(part);
  let wasAnd=k===3;i+=k;while(text[i]===' ')i++;
  if(punct){if(word(i,'and')){i+=3;wasAnd=true;}else if(word(i,'also')||word(i,'plus'))i+=4;while(text[i]===' ')i++;}
  if(wasAnd&&word(i,'also')){i+=4;while(text[i]===' ')i++;}start=i;
 }
 const part=text.slice(start).trim();if(!part)return {reason:'empty_clause',clauses:[]};if(part.length>96)return {reason:'clause_too_long',clauses:[]};if(chunks.length===4)return {reason:'too_many',clauses:[]};chunks.push(part);
 return {reason:'ok',clauses:chunks.map(c=>c.toLowerCase().replace(/ +$/,'').replace(/[.!?]+$/,'').trim())};
}
function plan(text,model){
 const s=split(text),results=[],actions=[];let reason=s.reason;
 if(reason!=='ok')return {...s,results,actions};
 for(const clause of s.clauses){const result=infer(clause,model);results.push({...result,text:clause});const v=result.effective;
  if(v===2)return {...s,results,actions:[2],reason:'ok',stop:true};
  if(v===7){reason='inference_rejected';continue;}
  if([[0,1],[3,4],[5,6],[8,9]].some(pair=>pair.includes(v)&&actions.includes(pair[0]===v?pair[1]:pair[0]))&&reason!=='inference_rejected')reason='conflict';
  if(!actions.includes(v))actions.push(v);
 }
 return {...s,results,actions:reason==='ok'?actions:[],reason};
}
class Simulator{
 constructor(model){this.model=model;this.reset();}
 reset(){this.now=0;this.distance=60;this.sensorFault=false;this.clearCount=0;this.hazard=1;this.valid=false;this.lastValid=0;this.sampleId=0;this.alert=true;this.inhibited=true;this.ready=false;this.busy=false;this.epoch=0;this.revision=0;this.generation=0;this.command=0;this.leaseEnd=0;this.actor=off();this.desired=off();this.applied=off();this.known=false;this.snapshots={slm:null,adas:null};this.events=[];this.queue=[];this.results=[];this.protocols={};this.phase='부팅: 센서 정상 확인과 STOP 동기화';this.sources=32;this.lastSnapshot=0;this.pollKey='';this.pollLogAt=0;this.nextSample=70;this.reversing=false;this.log('BOOT','네 보드 시작 · 모든 출력 OFF · ADAS ALERT LOW');this.sync();}
 log(type,text){this.events.push({time:this.now,type,text});if(this.events.length>90)this.events.shift();}
 wire(protocol,text){this.protocols[protocol]=this.now+140;if(protocol==='i2c'){const key=[this.distance,this.clearCount,this.hazard,this.valid].join('/');if(key===this.pollKey&&this.now-(this.pollLogAt||0)<500)return;this.pollKey=key;this.pollLogAt=this.now;}this.log(protocol.toUpperCase(),text);}
 later(ms,fn){this.queue.push({at:this.now+ms,fn});}
 snapshot(){return {revision:this.revision,epoch:this.epoch,command:this.command,desired:{...this.desired},applied:{...this.applied},known:this.known,sources:this.sources};}
 sync(){this.revision++;const value=this.snapshot();this.later(40,()=>{for(const role of ['slm','adas'])if(!this.snapshots[role]||value.revision>=this.snapshots[role].revision)this.snapshots[role]=JSON.parse(JSON.stringify(value));});this.lastSnapshot=this.now;}
 stop(reason='SLM STOP',source=1){this.generation++;this.busy=false;this.reversing=false;this.ready=false;this.epoch++;const epoch=this.epoch;this.sources=source;this.desired=off();this.known=false;this.phase=reason+' → STOP_ALL / OFF ACK 대기';this.sync();this.wire('uart',`STOP_ALL · epoch ${epoch} · ${reason}`);
  this.later(35,()=>{if(epoch!==this.epoch)return;this.actor=off();this.leaseEnd=0;});
  this.later(70,()=>{if(epoch!==this.epoch)return;this.applied=off();this.known=true;this.ready=!this.inhibited;this.phase=this.ready?'READY · 새 영어 명령 대기':'ADAS 차단 · 모든 출력 OFF';this.wire('uart',`ACK · epoch ${epoch} · 모든 출력 OFF 확인`);this.sync();});
 }
 interrupt(reason,source,physical=true){this.inhibited=true;if(physical){this.actor=off();this.leaseEnd=0;this.alert=true;this.wire('alert','LOW → MAIN PCINT20 + ACTUATOR INT1 · 출력 직접 OFF');}else this.log('MAIN','I2C clearCount<3 → 통신으로 STOP 요청 (ALERT는 HIGH)');this.stop(reason,source);}
 sample(failed=false){if(this.sensorFault&&!failed){this.later(30,()=>this.sample(true));return;}this.sampleId++;
  if(failed){this.valid=false;this.clearCount=0;this.hazard|=4;}
  else if(this.distance<2||this.distance>400){this.valid=false;this.clearCount=0;this.hazard|=4;}
  else{this.valid=true;this.lastValid=this.now;if(this.distance<=20){this.hazard|=2;this.clearCount=0;}else if(this.distance>=25){this.clearCount=Math.min(3,this.clearCount+1);if(this.clearCount===3)this.hazard=0;}else this.clearCount=0;}
  this.wire('i2c',`STATUS · ${this.valid?this.distance+'cm':'echo 오류'} · clear ${this.clearCount}/3 · sample ${this.sampleId}`);
  const physicalBlocked=this.hazard!==0;this.alert=physicalBlocked;const blocked=physicalBlocked||!this.valid||this.clearCount<3;
  if(blocked&&!this.inhibited)this.interrupt(this.hazard&4?'ADAS 센서 오류':physicalBlocked?'ADAS 장애물 ≤20cm':'MAIN 정상 3회 조건 미충족',this.hazard&4?4:physicalBlocked?2:4,physicalBlocked);
  else if(!blocked&&this.inhibited){this.inhibited=false;this.alert=false;this.log('ALERT','HIGH · 정상 3회 확인 · 이전 동작은 재개하지 않음');this.stop('해제 후 새 STOP epoch 동기화',32);}
 }
 tick(ms=10){const target=this.now+ms;while(this.now<target){this.now=Math.min(target,this.now+10);
  if(this.now>=this.nextSample){this.nextSample+=70;this.sample();}
  if(this.valid&&this.now-this.lastValid>200){this.valid=false;this.hazard|=4;this.clearCount=0;if(!this.inhibited)this.interrupt('측정 stale >200ms',4);}
  for(let i=0;i<this.queue.length;){if(this.queue[i].at<=this.now){const task=this.queue.splice(i,1)[0];task.fn();i=0;}else i++;}
  if(this.actor.drive&&this.leaseEnd&&this.now>=this.leaseEnd){this.actor.drive=0;this.leaseEnd=0;this.sources=64;this.log('LEASE','500ms 주행 lease 만료 · 모터만 정지');this.later(40,()=>{this.desired.drive=0;this.applied={...this.actor};this.known=true;this.wire('uart','STATUS · drive=0 · 부가 출력 유지');this.sync();});}
  if(this.now-this.lastSnapshot>=500)this.sync();
 }}
 send(text){if(text.trim().replace(/[.!?]+$/,'').trim().toLowerCase()==='stop'){this.results=[];this.stop();return;}
  if(!this.ready||this.busy){this.stop(this.busy?'busy 입력 → 전체 거절':'not_ready 입력 → 전체 정지',8);return;}
  const p=plan(text,this.model);this.results=[];this.log('INPUT',text);this.busy=true;this.ready=false;this.phase='MAIN: 문장 분리와 절별 검증';const gen=++this.generation,epoch=this.epoch;const safe=fn=>()=>{if(gen===this.generation&&epoch===this.epoch&&!this.inhibited)fn();};
  if(p.clauses.length===0){this.stop('문법 거절: '+p.reason,1);return;}
  let delay=0;
  p.results.forEach((result,i)=>{const local=result.reason==='safety_guard'||!result.scores.length;
   this.later(delay,safe(()=>{this.phase=`MAIN: ${i+1}/${p.clauses.length}절 · ${result.text}`;this.wire('uart','STATUS · 현재 작동 보드 출력 확인');}));
   this.later(delay+35,safe(()=>{if(!local){this.phase='SLM: embedding16 → mean pooling → ReLU64 → 10 scores';this.wire('spi','LOAD_INPUT / START · '+result.words.join(' '));}}));
   this.later(delay+120,safe(()=>{this.results.push(result);if(!local)this.wire('spi',`GET_RESULT · ${labels[result.effective]} · ${(result.confidence*100).toFixed(1)}%`);else this.log('GUARD',result.reason+' · 해당 절은 NN 요청 없이 거절');}));delay+=150;
  });
  this.later(delay,safe(()=>{this.busy=false;if(p.reason!=='ok'||p.stop){this.stop(p.stop?'SLM 전체 STOP':'계획 전체 거절: '+p.reason,1);return;}
   let next={...this.actor};for(const v of p.actions){if(v===0)next.drive=1;if(v===1)next.drive=-1;if(v===3)next.lights=true;if(v===4)next.lights=false;if(v===5)next.wiper=true;if(v===6)next.wiper=false;if(v===8)next.horn=true;if(v===9)next.horn=false;}
   this.command++;this.desired={...next};this.known=false;this.phase='UART: ACTUATE_BATCH → 적용 ACK 대기';this.wire('uart',p.actions.map(v=>labels[v]).join(' + '));this.sync();
   const reversing=next.drive&&this.actor.drive&&next.drive!==this.actor.drive;this.reversing=!!reversing;if(reversing){this.actor.drive=0;this.log('REVERSAL','방향 반전 · 100ms 무구동 · 부가 출력도 대기');}
   this.later(35+(reversing?100:0),safe(()=>{this.actor={...next};this.reversing=false;if(p.actions.includes(0)||p.actions.includes(1))this.leaseEnd=this.now+500;}));
   this.later(70+(reversing?100:0),safe(()=>{this.applied={...this.actor};this.known=true;this.ready=true;this.phase='APPLIED · 출력 적용 확인';this.wire('uart','ACK · 같은 session / epoch / command 확인');this.sync();}));
  }));
 }
}
const api={parse,infer,split,plan,Simulator,labels};if(typeof module!=='undefined'&&module.exports)module.exports=api;else root.DemoEngine=api;
})(typeof globalThis!=='undefined'?globalThis:this);
