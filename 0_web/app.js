/* Presentation controller; DOM only. No WebSerial, fetch, websocket, device access. */
'use strict';
const sim=new DemoEngine.Simulator(DemoModel);
const $=id=>document.getElementById(id);
const nameOf=drive=>drive===1?'전진':drive===-1?'후진':'정지';
const textState=s=>`${nameOf(s.drive)} · 조명 ${s.lights?'ON':'OFF'} · 와이퍼 ${s.wiper?'ON':'OFF'} · 경적 ${s.horn?'ON':'OFF'}`;
const escaped=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let paused=false,speed=.25,lastFrame=performance.now(),lastRender=0,selectedBoard='main',demoTasks=[];
const boards={
 main:{title:'MAIN · 전체 협력의 중심',description:'문장을 최대 4절로 나누고 모든 절을 검증한 뒤 하나의 명령 묶음을 전송합니다. 신경망 점수는 SLM에 요청합니다. 원하는 상태와 적용 확인 상태를 별도로 관리합니다.',pins:'D0/D1: PC USB · D2 RX/D3 TX: 작동 보드 UART · D4: ALERT PCINT20 · A4/A5: ADAS I2C · D10~D13: SLM SPI',detail:'USART0 + Timer1/INT0 software UART + TWI + SPI + Timer0 + PCINT20. 세 통신 방식 동시 운용. 출력 ACK를 받기 전 known=0.'},
 slm:{title:'SLM · 영어 명령의 의미 계산',description:'복사한 실제 학습 가중치로 embedding16 → mean pooling → ReLU64 → 10개 점수를 계산합니다. 출력 bias C를 포함하며 MAIN이 다시 더하지 않습니다. 일반 문장 생성 모델은 아닙니다.',pins:'D10 SS · D11 MOSI · D12 MISO · D13 SCK · D5 추론 계측 출력',detail:'SPI Mode0 / MSB first / 125kHz. 어휘245개 · 최대16단어/절 · confidence ≥50%. 모델95616d56. 상태 snapshot은 저장만 하며 NN 입력에 섞지 않습니다.'},
 adas:{title:'ADAS · HC-SR04 독립 판단',description:'2~400cm 측정 범위. 20cm 이하는 즉시 위험, 25cm 이상 세 번 연속이면 해제합니다. 중간 거리에서는 clear 횟수를0으로 만듭니다. MAIN은 별도로 정상3회 조건도 확인합니다.',pins:'D7 Trig · D8 Echo ICP1 · D4 open-drain ALERT · A4 SDA/A5 SCL',detail:'70ms 주기 · echo timeout30ms · stale >200ms · I2C0x12/100kHz. ALERT 한 선 → MAIN D4 + 작동 D3. 4.7kΩ 외부 풀업. LOW만 구동.'},
 actuator:{title:'ACTUATOR · 실제 출력과 독립 정지',description:'UART의 session/epoch/command를 검사한 뒤 모터·조명·와이퍼·경적을 적용합니다. ADAS INT1은 MAIN을 기다리지 않고 출력 레지스터를 직접 끕니다.',pins:'D0 RX/D1 TX UART · D3 ALERT INT1 · D2 로컬 정지 · D4/D7/D5 모터 · D12 STBY · D8 LED · D9 서보 · D6 피에조',detail:'주행 lease500ms, duty75/249 · 방향 반전100ms 대기 · servo45~135° · piezo1kHz. STOP은 모든 출력 신호OFF. 모터 관성이나 서보 전원까지 제거하는 의미는 아닙니다.'}
};
function selectBoard(board){selectedBoard=board;document.querySelectorAll('[data-board]').forEach(el=>{el.classList.toggle('selected',el.dataset.board===board);el.setAttribute('aria-pressed',el.dataset.board===board?'true':'false');});const b=boards[board];$('board-detail').innerHTML=`<h3>${b.title}</h3><p>${b.description}</p><p class="pin-line">${b.pins}</p><p class="detail-note">${b.detail}</p>`;}
function snapshotHTML(s){if(!s)return '<span>첫 STATE_SYNC 응답 대기</span>';return `<strong>revision ${s.revision} · epoch ${s.epoch}</strong><span>${escaped(textState(s.desired))}</span><span>적용 ${s.known?'확인':'미확인'} · ${escaped(textState(s.applied))}</span>`;}
function render(){
 $('sim-time').textContent=(sim.now/1000).toFixed(2)+'s';$('distance-value').textContent=sim.distance+' cm';$('clear-count').textContent=sim.clearCount+' / 3';$('alert-value').textContent=sim.alert?'LOW · 위험':'HIGH · 물리 신호 해제';$('epoch-value').textContent=sim.epoch;
 $('status-badge').textContent=sim.inhibited?'SAFETY STOP':sim.ready?'READY':'PROCESSING';$('status-badge').className='status-badge '+(sim.inhibited?'danger':sim.ready?'ready':'busy');$('status-text').textContent=sim.inhibited?'센서 정상 확인 후 새 명령이 필요합니다.':sim.ready?'영어 명령을 보내세요.':'통신과 상태 확인을 진행 중입니다.';$('phase-text').textContent=sim.phase;
 $('requested-state').textContent=textState(sim.desired);$('applied-state').textContent=(sim.known?'확인됨: ':'미확인: ')+textState(sim.applied);$('applied-state').classList.toggle('unknown',!sim.known);
 $('snapshot-main').innerHTML=snapshotHTML(sim.snapshot());$('snapshot-slm').innerHTML=snapshotHTML(sim.snapshots.slm);$('snapshot-adas').innerHTML=snapshotHTML(sim.snapshots.adas);
 const car=$('car-scene');car.dataset.drive=sim.actor.drive;car.dataset.lights=sim.actor.lights;car.dataset.wiper=sim.actor.wiper;car.dataset.horn=sim.actor.horn;car.dataset.alert=sim.alert;car.classList.toggle('paused',paused);$('car-direction').textContent=sim.reversing?'방향 반전 대기 100ms':nameOf(sim.actor.drive)+(sim.actor.drive?' · lease500ms':'');
 $('obstacle').style.transform=`translateX(${Math.round((sim.distance-60)*.35)}px)`;
 for(const protocol of ['spi','i2c','uart','alert']){const el=$('protocol-'+protocol);el.classList.toggle('active',(sim.protocols[protocol]||0)>sim.now);el.classList.toggle('danger',protocol==='alert'&&sim.alert);}
 $('event-log').innerHTML=sim.events.slice(-18).reverse().map(e=>`<li><time>${(e.time/1000).toFixed(2)}s</time><b>${escaped(e.type)}</b><span>${escaped(e.text)}</span></li>`).join('');
 $('classifier-results').innerHTML=sim.results.length?sim.results.map(r=>`<section class="result-clause"><strong>“${escaped(r.text)}”</strong><p>${escaped(DemoEngine.labels[r.effective])} · ${r.confidence?Math.round(r.confidence*100)+'% · ':''}${escaped(r.reason)}</p>${r.probabilities.length?[...r.probabilities].map((p,i)=>({p,i})).sort((a,b)=>b.p-a.p).slice(0,3).map(({p,i})=>`<div class="score-row"><span>${DemoEngine.labels[i]}</span><div class="score-track"><i style="width:${(p*100).toFixed(2)}%"></i></div><b>${(p*100).toFixed(1)}%</b></div>`).join(''):'<small>입력 검증에서 거절 · 신경망 요청 생략</small>'}</section>`).join(''):'<p class="empty-state">명령을 보내면 절별 분류와 확률이 표시됩니다. 전체 단독 <code>stop</code>은 NN을 거치지 않습니다.</p>';
 $('pause-demo').textContent=paused?'재생':'일시정지';$('pause-demo').setAttribute('aria-pressed',String(paused));
}
function inputDistance(distance){sim.distance=distance;$('distance-input').value=distance;render();}
function reset(){sim.reset();demoTasks=[];paused=false;inputDistance(60);$('sensor-fault').checked=false;render();}
function demo(type){reset();if(type==='normal'){demoTasks=[{at:350,fn:()=>sim.send('go forward, lights on, wipers on, horn on')}];}
 if(type==='obstacle'){demoTasks=[{at:350,fn:()=>sim.send('go forward, lights on, wipers on, horn on')},{at:550,fn:()=>inputDistance(15)}];}
 if(type==='recovery'){demoTasks=[{at:350,fn:()=>sim.send('lights on and horn on')},{at:800,fn:()=>inputDistance(15)},{at:1100,fn:()=>inputDistance(60)},{at:1800,fn:()=>{sim.log('GUIDE','장애물 해제 후에도 OFF 유지 · 새 명령 전송');sim.send('go forward and lights on');}}];}
 if(type==='stop'){demoTasks=[{at:350,fn:()=>sim.send('lights on and wipers on and horn on')},{at:1000,fn:()=>sim.send('stop')}];}
 if(type==='conflict'){demoTasks=[{at:350,fn:()=>sim.send('go forward and reverse')}];}
 sim.log('GUIDE',type==='obstacle'?'추론 중 장애물 개입: 오래된 결과는 출력되지 않습니다.':type==='recovery'?'정지 → 3회 정상 확인 → OFF 유지 → 새 명령':type==='conflict'?'방향 충돌: 일부 동작 없이 전체 거절':'안내 시나리오 시작');render();}
$('command-form').addEventListener('submit',e=>{e.preventDefault();demoTasks=[];sim.send($('command-input').value);render();});
$('stop-all').addEventListener('click',()=>{demoTasks=[];sim.stop();render();});$('reset-demo').addEventListener('click',reset);$('pause-demo').addEventListener('click',()=>{paused=!paused;render();});$('step-demo').addEventListener('click',()=>{paused=true;advance(70);render();});
$('distance-input').addEventListener('input',e=>inputDistance(Number(e.target.value)));$('sensor-fault').addEventListener('change',e=>{sim.sensorFault=e.target.checked;sim.log('SENSOR',sim.sensorFault?'echo 입력 오류 활성화':'echo 정상화 · 연속 3회 확인 필요');render();});
if($('speed-input')){$('speed-input').value='0.25';$('speed-input').addEventListener('change',e=>{speed=Number(e.target.value);});}
document.querySelectorAll('[data-command]').forEach(el=>el.addEventListener('click',()=>{$('command-input').value=el.dataset.command;demoTasks=[];sim.send(el.dataset.command);render();}));document.querySelectorAll('[data-demo]').forEach(el=>el.addEventListener('click',()=>demo(el.dataset.demo)));document.querySelectorAll('[data-board]').forEach(el=>el.addEventListener('click',()=>selectBoard(el.dataset.board)));
function advance(ms){const end=sim.now+ms;while(sim.now<end){sim.tick(Math.min(10,end-sim.now));for(let i=0;i<demoTasks.length;){if(demoTasks[i].at<=sim.now){demoTasks.splice(i,1)[0].fn();}else i++;}}}
function frame(now){const dt=Math.min(100,now-lastFrame);lastFrame=now;if(!paused)advance(dt*speed);if(now-lastRender>70){render();lastRender=now;}requestAnimationFrame(frame);}
selectBoard('main');render();requestAnimationFrame(frame);
