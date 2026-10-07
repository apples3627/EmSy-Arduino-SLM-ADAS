"""Second independent sealed audit: authored before revised model disclosure."""
import json,hashlib
from pathlib import Path
V=Path(__file__).parent
C={
0:['move the car forward now','please start driving forward','go ahead with the car','could you drive straight forward','turn on forward movement','start moving the car ahead','please move forward slowly'],
1:['move the car backward now','please start driving backward','go back with the car','could you drive backwards please','turn on reverse movement','start moving the car back','please move backward slowly'],
2:['please turn off everything now','stop all outputs now please','shut down the whole car','could you halt everything now','stop every function please','turn off the whole system','please shut all outputs off'],
3:['please switch on the car lights','could you turn the headlights on now','start the lights please','enable headlights now please','turn the light on for me','please switch the lamps on now','put the headlights on please'],
4:['please switch off the car lights','could you turn the headlights off now','stop the lights please','disable headlights now please','turn the light off for me','please switch the lamps off now','put the headlights off please'],
5:['please switch on the car wipers','could you turn the windshield wipers on now','begin the wipers please','enable wiping now please','turn the wipers on for me','please start the wiper now','put the windshield wiper on please'],
6:['please switch off the car wipers','could you turn the windshield wipers off now','end the wipers please','disable wiping now please','turn the wipers off for me','please stop the wiper now','put the windshield wiper off please'],
8:['please switch on the car horn','could you turn the buzzer on now','begin sounding the horn please','enable the buzzer now please','turn the horn on for me','please start honking now','put the buzzer on please'],
9:['please switch off the car horn','could you turn the buzzer off now','end sounding the horn please','disable the buzzer now please','turn the horn off for me','please stop honking now','put the buzzer off please'],
7:['do not start the car forward','never switch on the horn','please do not enable headlights','if the car moves sound the horn','when it rains start wiping please','move the car ahead then back','turn on the horn and lights','turn off the wiper and start the horn','go forward or backwards','the headlights were on','I was moving backward','the horn is beeping now','why do the lights work','how do I enable the wipers','what does reverse mean','I wonder whether the horn works','please tell me the weather','make the car red','turn up the music please','hello please']}
first={json.loads(s)['text'] for s in (V/'sealed_holdout.jsonl').read_text().splitlines()};rows=[]
for label,texts in C.items():
 for i,s in enumerate(texts):
  assert s not in first
  rows.append({'id':f'v2_c{label}_{i:02d}','text':s,'expected':label,'suite':'semantic','category':'known_action' if label!=7 else 'ambiguous_or_noncommand'})
raw=''.join(json.dumps(r,sort_keys=True)+'\n' for r in rows).encode();(V/'sealed_holdout_v2.jsonl').write_bytes(raw)
meta={'schema':1,'authorship':'independent second audit before revised model disclosure; first audit aggregate failure categories known to trainer','known_cases':63,'unknown_cases':20,'sha256':hashlib.sha256(raw).hexdigest(),'policy':'V1 cases are regression only after repair. V2 cases never disclosed for training; case set not edited after model results.'};(V/'sealed_manifest_v2.json').write_text(json.dumps(meta,indent=2)+'\n');print(meta)
