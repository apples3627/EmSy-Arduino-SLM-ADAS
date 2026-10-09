import collections,json,pathlib
import numpy as np
D=pathlib.Path(__file__).resolve().parent;R=D.parent
truth={'move forward':'FORWARD','go backward':'REVERSE','turn on the lights':'LIGHT_ON','switch off the lights':'LIGHT_OFF','turn on the wiper':'WIPER_ON','switch off the wiper':'WIPER_OFF','turn on the horn':'HORN_ON','switch off the horn':'HORN_OFF','lights on':'LIGHT_ON','lights off':'LIGHT_OFF','horn on':'HORN_ON','horn off':'HORN_OFF','wiper on':'WIPER_ON','wiper off':'WIPER_OFF','forward':'FORWARD','stop':'STOP'}
known=[]
for r in map(json.loads,(D/'corrected_results.jsonl').read_text().splitlines()):
 for c in r['result']['clause_results']:
  text=c['text'].removeprefix('please ').removeprefix('kindly ')
  if text in truth:known.append(dict(case=r['id'],truth=truth[text],raw=c['raw_label'],effective=c['effective_label']))
with np.load(R/'model_int8.npz') as z:
 learned=int(sum(z[k].size for k in ('e','w','b','o','c')));storage=int(sum(z[k].nbytes for k in ('e','w','b','o','c','se','sw','so')))
out=dict(known_explicit_positive_clause_instances=len(known),raw_nn_correct=sum(x['raw']==x['truth'] for x in known),raw_nn_accuracy=sum(x['raw']==x['truth'] for x in known)/len(known),effective_nn_correct=sum(x['effective']==x['truth'] for x in known),per_class=dict(collections.Counter(x['truth'] for x in known)),learned_parameters=learned,quant_tensor_and_scale_bytes=storage,scope='Only independently unambiguous exact clause annotations; grammar, guard rejection and plan reduction excluded. Repeated clause instances included; vocabulary/clauses commonly overlap training.')
(D/'nn_clause_summary.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
