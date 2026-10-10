#!/usr/bin/env python3
"""Seal independently selected requirements, never training data or field evidence."""
from pathlib import Path
import hashlib,json
ROOT=Path(__file__).resolve().parent
cases=[]
def add(group,name,criterion,**inputs):
 cases.append(dict(id=f'{group}-{len(cases)+1:02}',group=group,name=name,inputs=inputs,criterion=criterion))
for n in range(1,5):
 for phase in ('early','middle','late'):
  add('priority',f'{n} clauses interrupted {phase}','Active-low ADAS immediately gates actual outputs; obsolete SLM generation cannot dispatch after clear.',clauses=n,phase=phase)
for mm in (199,200,201,249,250):
 add('sensor',f'boundary {mm}mm','<=200mm asserts on first valid sample; >=250mm counts toward release; interior band neither falsely asserts nor releases.',range_mm=mm)
for seq in ([250,250],[250,250,250],[250,None,250,250],[200,250,249,250,250,250]):
 add('sensor','release sequence '+str(seq),'Boot/obstacle only releases after three consecutive valid clear samples; invalid or hysteresis sample resets clear count.',range_mm=seq)
for age in (200,201):
 add('sensor',f'stale age {age}','At >200ms stale sample asserts sensor fault and active-low output.',age_ms=age)
add('sensor','uint32 clock wrap','Unsigned elapsed age remains correct across millis wrap.',start_ms=4294967200,end_ms=105)
for mm in (19,20,4000,4001):
 add('capture',f'physical range {mm}mm','Outside 2..400cm invalid/fault; endpoints valid with integer conversion.',range_mm=mm)
add('capture','Timer1 wrap','Rising near 65535 then falling after wrap yields short correct pulse, never huge distance.',rise_ticks=65500,pulse_ticks=2320)
add('capture','missing echo','30ms echo timeout invalidates range and asserts fault; trigger never blocks CPU.')
add('capture','noise edge','Unmatched or repeated edges cannot create a false clear sample.')
add('registry','independent sources','Clearing one registered source leaves another active contributor asserted.')
add('actor','ISR while MAIN stalled','ISR gates motor/lamp/wiper/horn registers immediately; UART session retained and sync flag set.')
add('actor','IRQ during reversal','Pending reversal canceled; foreground stale state cannot restore output.')
add('actor','old ACT after clear','Old epoch remains inhibited after line release until strictly newer STOP epoch accepted.')
add('actor','strict new epoch STOP','New STOP after line high synchronizes; same epoch does not; fresh model command needed.')
add('main','false clear bus','Physical low line blocks actuation despite valid clear telemetry.')
add('main','stale bus high line','High line with stale/CRC-invalid telemetry blocks actuation.')
add('main','late ACK old epoch','Old epoch ACK cannot mark appliedKnown or restore requested state.')
add('state','explicit SLM STOP','Requested drive 0 published immediately; applied unknown until matching ACK; SLM+ADAS receive coherent revision.')
add('state','drive lease','500ms motor lease expiry becomes actual 0 and desired 0 after observed status, retaining ancillary outputs.')
add('protocol','metadata negatives','H32/old protocol/old labels rejected; full H64 metadata requires include-bias=1.')
add('model','full CPP logits','All ten real full-worker scores match frozen full kernel and include global bias exactly once.')
add('retry','immutable identity','Repeated request ID requires identical operation and payload; changed payload rejected without action.')
p=ROOT/'scenarios.json';p.write_text(json.dumps(cases,indent=2)+'\n')
(ROOT/'scenario_seal.json').write_text(json.dumps(dict(case_count=len(cases),sha256=hashlib.sha256(p.read_bytes()).hexdigest(),purpose='Pre-implementation independent host verification requirements; not field-tested data'),indent=2)+'\n')
print(f'SEALED {len(cases)} requirements')
