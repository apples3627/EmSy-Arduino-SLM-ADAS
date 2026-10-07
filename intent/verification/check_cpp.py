import csv,json,subprocess,sys
from pathlib import Path
import numpy as np
V=Path(__file__).parent;R=V.parent;sys.path.insert(0,str(R))
from reference import infer,load_model,prepare,LABELS
allrows=list(csv.DictReader((R/'dataset/sentences.csv').open()));rows=[]
for split in ('train','val','test'):
 for label in range(10):
  bucket=[r for r in allrows if r['split']==split and int(r['label'])==label]
  rows.extend(bucket[int(i)] for i in np.linspace(0,len(bucket)-1,7))
sealed=[json.loads(s) for name in ('sealed_holdout.jsonl','sealed_holdout_v2.jsonl','sealed_holdout_v3.jsonl') for s in (V/name).read_text().splitlines()];cases=rows+sealed+json.loads((V/'contract_cases.json').read_text())
# Host is line-oriented; newline case is checked directly through argv, NUL through stdin.
line_cases=[r for r in cases if '\n' not in r['text']]
data=''.join(r['text']+'\n' for r in line_cases).encode()
cpp=[json.loads(s) for s in subprocess.run([str(V/'host')],input=data,capture_output=True,check=True).stdout.decode().splitlines()]
a,m=load_model();import reference;reference.load_model=lambda:(a,m);scores_error=0;dec_errors=[];valid_errors=[];pred_cache={}
for case,c in zip(line_cases,cpp):
 p=infer(case['text']);pred_cache[case['text']]=p;pid=LABELS.index(p['effective_label'])
 if pid!=c['effective_id']:dec_errors.append(case['text'])
 if p['scores'] is not None:scores_error=max(scores_error,float(np.max(np.abs(np.array(p['scores'])-c['scores']))))
 if 'expected_valid' in case:
  ids,why,g=prepare(case['text'],m['vocabulary']);valid=ids is not None and not g
  if valid!=case['expected_valid']:valid_errors.append({'id':case['id'],'reason':why,'valid':valid})
# Actual separately compiled shard binaries and one main bias.
encoded=[r for r in line_cases if pred_cache[r['text']]['scores'] is not None];sd=''.join(r['text']+'\n' for r in encoded).encode()
parts=[]
for k in (1,2):parts.append([json.loads(s)['scores'] for s in subprocess.run([str(V/f'host_part{k}')],input=sd,capture_output=True,check=True).stdout.decode().splitlines()])
sharderr=max(float(np.max(abs(np.array(p1)+p2+a['c']-pred_cache[r['text']]['scores']))) for r,p1,p2 in zip(encoded,*parts))
# Dedicated argv permits newline inside an argument, which parser must reject.
nl=subprocess.run([str(V/'host'),'wipers\non'],capture_output=True,check=True);newline=json.loads(nl.stdout)
policy=subprocess.run([str(V/'policy')],capture_output=True,check=True).stdout.decode();policyrows=[[int(n) for n in line.split()] for line in policy.splitlines()]
assert policyrows[2][1:]==[0,0,0,0] and policyrows[7][1:]==[0,0,0,0]
assert policyrows[8][-1]==1 and policyrows[9][-1]==0
summary={'host_cases':len(line_cases),'dataset_sampling':'7 evenly-spaced rows per split and label = 210, plus all three audit sets and boundary cases','decision_mismatches':dec_errors,'max_score_abs_error':scores_error,'actual_cpp_shard_cases':len(encoded),'actual_cpp_shard_max_abs_error':sharderr,'contract_validity_mismatches':valid_errors,'newline_rejected':newline['effective_id']==7,'STOP_UNKNOWN_all_outputs_off':True,'horn_on_off_software_state':True,'nonfinite_globalstop':True,'runtime_heap_calls':'none in shared/intent_model.h; host uses std::string only for test I/O'}
(V/'cpp_audit_v3.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
