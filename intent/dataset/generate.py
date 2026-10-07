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
UNKNOWN=['hello','what is the weather','the cat likes milk','tell me a story','this car is red','where are the lights','how does the wiper work','the horn is loud','the car is moving','i like the headlights','a robot can learn','lights are bright','the glass is clean','nothing to do','what does forward mean','why is the horn loud','i said forward yesterday','maybe the car could move','the driver likes lamps','the wiper was on']
for phrases in PHRASES.values():
 for phrase in phrases[:5]:
  UNKNOWN += ['do not '+phrase,'if it rains '+phrase,'when i arrive '+phrase,'i wonder whether to '+phrase]
UNKNOWN += ['go forward and turn the lights on','move back and honk','start the wiper and drive forward','turn on the horn then move ahead','turn the lights on or off','forward backward','on off','do not move forward','do not turn the lights on']
UNKNOWN=random.Random(20261007).sample(list(dict.fromkeys(UNKNOWN)),60)
TEMPLATES=[('train','bare',['{}']),('train','request-prefix',['please {}','can you {}','could you {}','would you {}']),('train','request-suffix',['{} now','{} for me','{} right now','{} please','{} for a moment']),('val','adverb-prefix',['now please {}','for me please {}','please now {}']),('test','polite-combination',['can you please {} now','would you {} for me please','{} right now for me','could you {} now please'])]
def main():
 rows=[];seen={}
 for split,family,templates in TEMPLATES:
  for label,phrases in sorted({**PHRASES,7:UNKNOWN}.items()):
   limit=60 if label==7 else len(phrases)
   for phrase in phrases[:limit]:
    for template in templates:
     text=template.format(phrase)
     if len(text)>96 or len(text.split())>16:continue
     if text in seen:
      assert seen[text]==label,('contradictory label',text);continue
     seen[text]=label;rows.append(dict(text=text,label=label,label_name=LABELS[label],family=family,split=split,provenance='authored synthetic English phrase + grouped template v1'))
 path=ROOT/'dataset/sentences.csv'
 with path.open('w',newline='') as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
 info={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'rows':len(rows),'split_counts':{s:sum(r['split']==s for r in rows) for s in ['train','val','test']},'family_splits':{f:s for s,f,_ in TEMPLATES},'labels':LABELS,'split_policy':'Whole template families disjoint; action phrases/known vocabulary shared intentionally; normalized duplicate texts removed globally. Test manifest frozen before training.'}
 (ROOT/'dataset/manifest.json').write_text(json.dumps(info,indent=2)+'\n');print(info)
if __name__=='__main__':main()
