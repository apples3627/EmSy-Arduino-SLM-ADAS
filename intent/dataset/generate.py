#!/usr/bin/env python3
"""Authored synthetic templates; no downloads or external generated data."""
import csv,pathlib,hashlib,json,random
ROOT=pathlib.Path(__file__).resolve().parents[1]
LABELS=['FORWARD','REVERSE','STOP','LIGHT_ON','LIGHT_OFF','WIPER_ON','WIPER_OFF','UNKNOWN','HORN_ON','HORN_OFF']
PHRASES={
0:['go forward','move forward','drive forward','roll forward','proceed forward','move ahead','go ahead','drive ahead','roll ahead','proceed ahead','move straight','drive straight','go straight','move the car forward','drive the car forward','move the car ahead','go straight ahead','move forward slowly'],
1:['go backward','move backward','drive backward','roll backward','go backwards','move backwards','drive backwards','roll backwards','reverse','reverse the car','drive in reverse','move in reverse','back up','back the car up','move back','go back','drive back','move the car backward'],
2:['stop','halt','stop everything','halt everything','stop all','turn everything off','switch everything off','shut everything down','all off','stop the car','stop moving','halt the car','stop all outputs','stop all functions','turn all outputs off','shut all functions down','stop the whole car','everything off'],
3:['turn the lights on','turn on the lights','switch the lights on','switch on the lights','lights on','turn the headlights on','turn on the headlights','switch the headlights on','headlights on','turn the lamps on','switch on the lamps','lamps on','enable the lights','enable the headlights','start the lights','light the lamps','turn the light on','switch the light on'],
4:['turn the lights off','turn off the lights','switch the lights off','switch off the lights','lights off','turn the headlights off','turn off the headlights','switch the headlights off','headlights off','turn the lamps off','switch off the lamps','lamps off','disable the lights','disable the headlights','stop the lights','extinguish the lamps','turn the light off','switch the light off'],
5:['start the wiper','start the wipers','turn the wiper on','turn on the wipers','switch on the wiper','wiper on','wipers on','wipe the windshield','start wiping','begin wiping','enable the wipers','enable the wiper','start the windshield wiper','turn the windshield wiper on','wipe the glass','clean the windshield','start wiping the glass','begin the wiper sweep'],
6:['stop the wiper','stop the wipers','turn the wiper off','turn off the wipers','switch off the wiper','wiper off','wipers off','stop wiping the windshield','stop wiping','halt wiping','disable the wipers','disable the wiper','stop the windshield wiper','turn the windshield wiper off','stop the wiper sweep','halt the wiper sweep','stop wiping the glass','end the wiper sweep'],
8:['beep','honk','sound the horn','turn the horn on','turn on the horn','horn on','start the horn','enable the horn','turn the buzzer on','turn on the buzzer','buzzer on','start the buzzer','enable the buzzer','sound the buzzer','start beeping','make the horn sound','start honking','switch on the horn'],
9:['stop the horn','stop honking','silence the horn','turn the horn off','turn off the horn','horn off','disable the horn','stop the buzzer','silence the buzzer','turn the buzzer off','turn off the buzzer','buzzer off','disable the buzzer','stop beeping','mute the horn','mute the buzzer','switch off the horn','end the horn sound']}
# V2 broader independent authored lexical combinations, not audit literals.
movement=['go','move','drive','roll','proceed','travel','start moving','start driving','begin moving','keep moving','make the car go','send the car','take the car','move the vehicle','drive the vehicle','move the motor']
PHRASES[0] += [f'{v} {d} {a}'.strip() for v in movement for d in ['forward','forwards','ahead','straight','straight ahead','to the front'] for a in ['','slowly','a little','a little bit','gently','at low speed']]
PHRASES[0] += ['advance','advance the car','proceed','start going forward','keep driving ahead']
PHRASES[1] += [f'{v} {d} {a}'.strip() for v in movement for d in ['back','backward','backwards','in reverse','to the rear'] for a in ['','slowly','a little','a little bit','gently','at low speed']]
PHRASES[1] += ['retreat','retreat the car','reverse the vehicle','back it up','start reversing','keep reversing']
PHRASES[2] += [f'{v} {obj}' for v in ['stop','halt','disable','turn off','switch off','power off','shut down'] for obj in ['all','everything','all functions','all outputs','the whole car','the car','the vehicle','all systems']]
PHRASES[2] += ['emergency stop','full stop','stop immediately','stop completely','shut the car down','stop the motor','all functions off','shut down everything','stop all motion']
objects={3:['light','lights','headlights','headlamps','lamps','lamp','front lights','car lights','vehicle lights','lighting'],5:['wiper','wipers','windshield wiper','windshield wipers','windscreen wiper','windscreen wipers','window wipers','wiping','windshield wiping','wiper sweep'],8:['horn','buzzer','beeper','piezo','piezo buzzer','car horn','vehicle horn','beeping','honking','alarm']}
for label,objs in objects.items():
 on=['turn on','switch on','enable','activate','start','power on','begin']
 off=['turn off','switch off','disable','deactivate','stop','halt','end']
 if label==5:on+=['run','engage'];off+=['disengage']
 if label==8:on+=['sound','play'];off+=['silence','mute']
 for v in on:PHRASES[label]+=[f'{v} the {o}' for o in objs]
 for v in off:PHRASES[label+1]+=[f'{v} the {o}' for o in objs]
 for o in objs:
  PHRASES[label]+=[f'turn the {o} on',f'switch the {o} on',f'{o} on']
  PHRASES[label+1]+=[f'turn the {o} off',f'switch the {o} off',f'{o} off']
PHRASES[5] += ['wipe the windscreen','clean the windscreen','wipe the window','clean the glass','move the wipers','start wiping the windshield','keep wiping','run the wipers','begin sweeping the wipers']
PHRASES[6] += ['stop wiping the windscreen','stop cleaning the windshield','stop the sweep','end wiping','stop wiping the window','stop sweeping the wipers']
PHRASES[8] += ['make a beep','make a sound','honk the horn','beep the horn','start sounding the horn','let the horn sound','make some noise']
PHRASES[9] += ['stop the sound','stop the noise','quiet the horn','quiet the buzzer','stop sounding the horn','stop making noise']
# Include all original v1 expressions and broad deterministically sampled additions.
original_lengths={0:18,1:18,2:18,3:18,4:18,5:18,6:18,8:18,9:18}
for label in PHRASES:
 unique=list(dict.fromkeys(PHRASES[label]));base=unique[:original_lengths[label]]
 PHRASES[label]=base+random.Random(20261007+label).sample(unique[len(base):],min(82,len(unique)-len(base)))
# V3: bounded lexical breadth in natural command constructions.
vehicles=['car','vehicle','rover','robot','automobile','truck','cart','machine','motor','wheels']
verbs=['go','move','drive','roll','travel','proceed','start moving','begin moving','keep moving','continue moving','get moving','make the car move','send the vehicle','command the car to move','tell the car to move']
PHRASES[0] += [f'{verb} {direction} {tail}'.strip() for verb in verbs for direction in ['forward','forwards','ahead','straight ahead','toward the front','towards the front','in the forward direction','to the front','forward a short distance','ahead a little','forward slightly'] for tail in ['','slowly','gently','carefully','smoothly','a bit','a little more','a short way','a small distance','at low speed']]
PHRASES[1] += [f'{verb} {direction} {tail}'.strip() for verb in verbs for direction in ['back','backward','backwards','in reverse','toward the rear','towards the rear','in the backward direction','to the back','back a short distance','back slightly','back a little'] for tail in ['','slowly','gently','carefully','smoothly','a bit','a little more','a short way','a small distance','at low speed']]
for vehicle in vehicles:
 for verb in ['move','drive','roll','send','make']:
  PHRASES[0]+=[f'{verb} the {vehicle} forward',f'{verb} the {vehicle} ahead',f'get the {vehicle} moving forward',f'start the {vehicle} moving forward']
  PHRASES[1]+=[f'{verb} the {vehicle} backward',f'{verb} the {vehicle} back',f'get the {vehicle} moving in reverse',f'start the {vehicle} moving backward']
PHRASES[0]+=['advance','advance the vehicle','continue forward motion','start forward movement','begin forward travel','resume moving ahead','keep going ahead','set the direction to forward']
PHRASES[1]+=['retreat','retreat the vehicle','continue backward motion','start reverse movement','begin backward travel','resume moving back','keep going back','set the direction to reverse','reverse it','back it up']
PHRASES[2]+=[f'{verb} {obj}' for verb in ['stop','halt','shut down','power down','power off','disable','cancel','end','cease','cut off'] for obj in ['everything','all systems','all functions','all outputs','all operations','every function','every system','all activity','all actions','all motion','the entire car','the entire system','the whole vehicle','all devices']]
PHRASES[2]+=['shut it all down','turn it all off','make everything stop','stop each function','nothing should remain on','bring everything to a stop','come to a complete stop','bring the car to a halt','full shutdown','complete shutdown','emergency halt','stop the whole machine','all systems off','all functions off','shut down the system completely','turn all equipment off']
objects={3:['light','lights','headlight','headlights','headlamp','headlamps','lamp','lamps','front lamps','front lights','car lights','vehicle lights','lighting','illumination','beam','beams'],5:['wiper','wipers','windshield wiper','windshield wipers','windscreen wiper','windscreen wipers','window wiper','window wipers','wiping','wiper motor','windshield cleaner','windshield wiping','wiper sweep','wiping system'],8:['horn','car horn','vehicle horn','buzzer','beeper','piezo','piezo buzzer','piezoelectric buzzer','beeping','honking','alarm','horn sound','audible signal','sound signal']}
for label,objs in objects.items():
 for obj in objs:
  for verb in ['activate','enable','engage','start','run','operate','use','switch on','turn on','power on','bring on']:
   PHRASES[label]+=[f'{verb} the {obj}',f'please {verb} the {obj}']
  for verb in ['deactivate','disable','disengage','stop','halt','switch off','turn off','power off','shut off']:
   PHRASES[label+1]+=[f'{verb} the {obj}',f'please {verb} the {obj}']
  for state in ['on','active','enabled','running']:
   PHRASES[label]+=[f'set the {obj} {state}',f'make the {obj} {state}',f'put the {obj} {state}',f'set the {obj} to {state}']
  for state in ['off','inactive','disabled','stopped']:
   PHRASES[label+1]+=[f'set the {obj} {state}',f'make the {obj} {state}',f'put the {obj} {state}',f'set the {obj} to {state}']
  PHRASES[label]+=[f'get the {obj} going',f'keep the {obj} on']
  PHRASES[label+1]+=[f'keep the {obj} off',f'leave the {obj} off']
PHRASES[3]+=['illuminate the road','light up the headlights','make the lamps bright','put the lights on','switch the lighting on']
PHRASES[4]+=['extinguish the lights','make the lights dark','switch the lighting off','put the lights out']
PHRASES[5]+=['sweep the windshield','wipe the front window','clear the windshield','clean the windscreen','start the wiping function','continue wiping','resume the wiper','begin the wiping action','sweep the glass']
PHRASES[6]+=['cease wiping','end the wiping action','stop the windshield cleaner','suspend the wiper','cancel wiping','stop the window cleaning','finish wiping','halt the sweep']
PHRASES[8]+=['make a beep','make a sound','sound a warning','give a beep','give a honk','beep once','start the horn noise','make the beeper beep','let the horn ring','ring the buzzer']
PHRASES[9]+=['stop the noise','stop the sound','make the horn quiet','make the buzzer silent','silence the beeper','quiet the buzzer','stop the horn noise','end the beeping','cancel the horn','stop the audible signal']
# Greedy lexical coverage then deterministic balanced sample; do not discard rare synonyms.
for label in PHRASES:
 pool=list(dict.fromkeys(PHRASES[label]));chosen=pool[:18];covered=set(' '.join(chosen).split())
 for phrase in pool:
  words=set(phrase.split())
  if words-covered:chosen.append(phrase);covered|=words
 remaining=[p for p in pool if p not in chosen]
 chosen+=random.Random(303+label).sample(remaining,min(max(0,140-len(chosen)),len(remaining)))
 PHRASES[label]=chosen
UNKNOWN=['hello','what is the weather','the cat likes milk','tell me a story','this car is red','where are the lights','how does the wiper work','the horn is loud','the car is moving','i like the headlights','a robot can learn','lights are bright','the glass is clean','nothing to do','what does forward mean','why is the horn loud','i said forward yesterday','maybe the car could move','the driver likes lamps','the wiper was on']
for phrases in PHRASES.values():
 for phrase in phrases[:5]:
  UNKNOWN += ['do not '+phrase,'if it rains '+phrase,'when i arrive '+phrase,'i wonder whether to '+phrase]
UNKNOWN += [f'{guard} {phrases[i]}' for phrases in PHRASES.values() for i in [0,3,8] for guard in ['do not','never','while waiting','before dinner','after dinner','without permission','what happens if','why would you','only if asked']]
UNKNOWN += ['go forward and turn the lights on','move back and honk','start the wiper and drive forward','turn on the horn then move ahead','turn the lights on or off','forward backward','on off','do not move forward','do not turn the lights on']
UNKNOWN=[p for p in UNKNOWN if not any(w in p.split() for w in ['and','also','plus'])]
UNKNOWN=random.Random(20261007).sample(list(dict.fromkeys(UNKNOWN)),150)
# Explicit state-free objects and elliptical fragments teach UNKNOWN rather than inventing a state.
UNKNOWN += ['lights','light','headlights','lamps','horn','buzzer','wiper','wipers','the lights','the horn','the wipers','forward backward','on off','turn','switch','turn on','turn off','on the','off the']
for label, objs in objects.items():
 for obj in objs:
  for verb in ['turn','switch']:
   PHRASES[label]+=[f'{verb} on {obj}',f'{verb} {obj} on']
   PHRASES[label+1]+=[f'{verb} off {obj}',f'{verb} {obj} off']
  PHRASES[label]+=[f'{obj} on']; PHRASES[label+1]+=[f'{obj} off']
TEMPLATES=[('train','bare',['{}']),('train','request-prefix',['please {}','can you {}','just {}','will you {}','i want you to {}','could you {}','would you {}','i would like you to {}','kindly {}','let us {}','we need you to {}','please help me {}','i request that you {}']),('train','request-suffix',['{} now','{} right now','{} for me','{} for a moment','{} a bit','{} immediately','{} as soon as possible','{} at once']),('val','mixed-request',['please {} now for me','can you {} please for a moment','kindly {} right now']),('test','held-combination',['could you kindly {} right now','i would like you to {} please','please just {} now','{} right now for me'])]

def main():
 rows=[];seen={}
 for split,family,templates in TEMPLATES:
  for label,phrases in sorted({**PHRASES,7:UNKNOWN}.items()):
   limit=len(phrases)
   for phrase in phrases[:limit]:
    for template in templates:
     text=template.format(phrase)
     if len(text)>96 or len(text.split())>16:continue
     if text in seen:
      assert seen[text]==label,('contradictory label',text);continue
     seen[text]=label;rows.append(dict(text=text,label=label,label_name=LABELS[label],family=family,split=split,provenance='authored synthetic English phrase + grouped template v4'))
 path=ROOT/'dataset/sentences.csv'
 with path.open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
 info={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'rows':len(rows),'split_counts':{s:sum(r['split']==s for r in rows) for s in ['train','val','test']},'family_splits':{f:s for s,f,_ in TEMPLATES},'labels':LABELS,'split_policy':'Whole template families disjoint; action phrases/known vocabulary shared intentionally; normalized duplicate texts removed globally. Test manifest frozen before training.'}
 (ROOT/'dataset/manifest.json').write_text(json.dumps(info,indent=2)+'\n');print(info)
if __name__=='__main__':main()
