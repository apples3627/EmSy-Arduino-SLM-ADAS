"""Author-independent holdout, created before model disclosure. Never training data."""
import hashlib, json
from pathlib import Path
root=Path(__file__).parent
classes={
0:['move forward','please drive ahead','go forwards now','could you move ahead please','start moving forward','drive forward please','advance the car','send the vehicle forward','forward motion please','please go straight ahead'],
1:['move backward','please drive in reverse','go backwards now','could you back up please','start moving backward','reverse the car please','back the vehicle up','send the vehicle backward','reverse motion please','please go into reverse'],
2:['stop everything','please halt all outputs','bring everything to a stop','could you shut everything off please','stop all movement and accessories','turn every output off','halt the whole system','stop the car and all accessories','please stop everything now','all outputs off please'],
3:['switch the lights on','please illuminate the lamps','turn on the headlights now','could you enable the lights please','light up the headlights','activate the vehicle lights','put the lights on please','please turn the lamps on','start the headlights','headlights on now'],
4:['switch the lights off','please extinguish the lamps','turn off the headlights now','could you disable the lights please','shut the headlights off','deactivate the vehicle lights','put the lights off please','please turn the lamps off','stop the headlights','headlights off now'],
5:['switch the wipers on','please start the windshield wipers','turn on the wiper now','could you enable the wipers please','begin wiping the windshield','activate the windshield wiper','put the wipers on please','please turn the wiper on','start wiping please','wipers on now'],
6:['switch the wipers off','please stop the windshield wipers','turn off the wiper now','could you disable the wipers please','stop wiping the windshield','deactivate the windshield wiper','put the wipers off please','please turn the wiper off','stop wiping please','wipers off now'],
8:['switch the horn on','please sound the horn','turn on the horn now','could you enable the horn please','start sounding the horn','activate the buzzer','put the horn on please','please turn the buzzer on','start the horn please','horn on now'],
9:['switch the horn off','please silence the horn','turn off the horn now','could you disable the horn please','stop sounding the horn','deactivate the buzzer','put the horn off please','please turn the buzzer off','stop the horn please','horn off now'],
7:['do not move forward','never turn the lights on','do not sound the horn','if it rains turn on the wipers','when I say go move forward','turn the lights on and off','move forward and reverse','sound the horn and turn it off','the wipers are running','the car is moving forward','are the headlights on','what does the horn do','I like bright lights','drive to the supermarket','make some coffee','turn on the air conditioner','set the horn volume to fifty','reverse the light color','could you tell me about the wipers','lights might be useful']}
rows=[]
for label, texts in classes.items():
 for i,s in enumerate(texts): rows.append({'id':f'c{label}_{i:02d}','text':s,'expected':label,'suite':'semantic','category':'known_action' if label!=7 else 'ambiguous_or_noncommand'})
# Independent normalization variants are recorded separately to avoid inflating semantic coverage.
for i,(s,y) in enumerate([('  MOVE   FORWARD  ',0),('Please\tturn\tthe\tlights\ton.',3),('STOP EVERYTHING!!!',2),('Horn OFF?',9),('wipers\non',5)]):
 rows.append({'id':f'n_{i:02d}','text':s,'expected':y,'suite':'normalization','category':'case_whitespace_punctuation'})
# Limits are parametrized after disclosure of the published contract; malformed inputs stay separate.
for i,(s,kind) in enumerate([('move forward\x00then reverse','embedded_nul'),('móve forward','non_ascii'),('','empty'),('   ','whitespace_only')]):
 rows.append({'id':f'm_{i:02d}','text':s,'expected':7,'suite':'malformed','category':kind})
raw=''.join(json.dumps(r,ensure_ascii=True,sort_keys=True)+'\n' for r in rows).encode()
(root/'sealed_holdout.jsonl').write_bytes(raw)
meta={'schema':1,'authorship':'independent verifier, before trained-model disclosure','semantic_cases':110,'normalization_cases':5,'malformed_cases':4,'sha256':hashlib.sha256(raw).hexdigest(),'policy':'No model tuning or case selection after seeing model. Repairs must be labelled post-audit. Limit checks are contract-parametrized separately.'}
(root/'sealed_manifest.json').write_text(json.dumps(meta,indent=2)+'\n')
print(json.dumps(meta))
