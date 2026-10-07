"""Third compact holdout before final model disclosure; category feedback from V1/V2 acknowledged."""
from pathlib import Path
import json,hashlib
V=Path(__file__).parent
C={
0:['could you please move the car forward','drive ahead now please','please get the car moving forward','go straight forward for me'],
1:['could you please move the car backward','drive back now please','please get the car moving backward','go backwards for me'],
2:['could you please stop all outputs','halt everything now please','please shut the whole system down','stop the entire car system'],
3:['could you please switch the headlights on','light the lamps now please','please get the headlights on','turn on the car light for me'],
4:['could you please switch the headlights off','extinguish the lights now please','please get the headlights off','turn off the car light for me'],
5:['could you please switch the wipers on','begin wiping now please','please get the windshield wipers running','turn on the car wiper for me'],
6:['could you please switch the wipers off','end wiping now please','please get the windshield wipers stopped','turn off the car wiper for me'],
8:['could you please switch the horn on','sound the buzzer now please','please get the horn sounding','turn on the car buzzer for me'],
9:['could you please switch the horn off','silence the buzzer now please','please get the horn silent','turn off the car buzzer for me'],
7:['please never move the car ahead','do not switch the buzzer on now','if I arrive please sound the horn','when I ask start the lights','go backwards and turn on wipers','switch the lamps on or off','start the horn then stop the horn','the car was driving ahead','the wipers are off now','why was the buzzer sounding','tell me how to reverse','I like the car headlights','please wash the car','set the speed to ten','what is the weather now']}
seen={json.loads(s)['text'] for name in ('sealed_holdout.jsonl','sealed_holdout_v2.jsonl') for s in (V/name).read_text().splitlines()};rows=[]
for label,texts in C.items():
 for i,s in enumerate(texts):
  assert s not in seen
  rows.append({'id':f'v3_c{label}_{i:02d}','text':s,'expected':label,'suite':'semantic','category':'known_action' if label!=7 else 'ambiguous_or_noncommand'})
raw=''.join(json.dumps(r,sort_keys=True)+'\n' for r in rows).encode();(V/'sealed_holdout_v3.jsonl').write_bytes(raw);meta={'schema':1,'known_cases':36,'unknown_cases':15,'sha256':hashlib.sha256(raw).hexdigest(),'authorship':'before V3 disclosure, independently authored; prior audit aggregate categories were given to trainer, no literal cases','policy':'First V3 evaluation sealed; V1/V2 on V3 are regressions.'};(V/'sealed_manifest_v3.json').write_text(json.dumps(meta,indent=2)+'\n');print(meta)
