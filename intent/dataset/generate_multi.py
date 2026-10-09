#!/usr/bin/env python3
"""Composition tests reuse known TRAIN clauses; text/combo groups are disjoint, not unseen vocabulary claims."""
import json,random,hashlib,pathlib,sys
R=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from reference import combine_labels
pool={0:['go forward','move ahead','drive forward'],1:['go backward','reverse','move back'],2:['stop everything','stop all','halt everything'],3:['lights on','turn on the lights','headlights on'],4:['lights off','turn off the lights','headlights off'],5:['wipers on','turn on the wipers','wiper on'],6:['wipers off','turn off the wipers','wiper off'],7:['lights','horn','wipers'],8:['horn on','honk','turn on the horn'],9:['horn off','turn off the horn','stop honking']}
rng=random.Random(404);seen=set();rows=[]
for split,sep in [('train',' and '),('val',' also '),('test',' plus ')]:
 for i in range(600):
  while True:
   labels=[rng.choice(list(pool)) for _ in range(rng.choice([2,3,4]))]
   clauses=[rng.choice(pool[x]) for x in labels];text=sep.join(clauses)
   if text not in seen:break
  seen.add(text);actions,reason=combine_labels(labels)
  rows.append(dict(text=text,expected_actions=actions,expected_reason=reason,split=split,family='known_clause_composition_'+sep.strip(),labels=labels))
 # Invalid grammar/OOV/conditional/negation, stop dominates only after successful splitting.
 for tail in ['do not honk','if rain honk','honk then lights on','turn on lights or horn','zzzzzz','turn on lights and horn']:
  text='go forward'+sep+tail
  rows.append(dict(text=text,expected_actions=[],expected_reason='inference_rejected',split=split,family='invalid_'+sep.strip()))
p=R/'dataset/multi_commands.jsonl';p.write_text(''.join(json.dumps(x)+'\n' for x in rows))
(R/'dataset/multi_manifest.json').write_text(json.dumps(dict(sha256=hashlib.sha256(p.read_bytes()).hexdigest(),rows=len(rows),split_counts={s:sum(x['split']==s for x in rows) for s in ['train','val','test']},usage='All test rows frozen before final training; multi text never NN training. Known train phrases reused to measure composition, not lexical generalization; separator families disjoint; global text dedup.'),indent=2)+'\n')
