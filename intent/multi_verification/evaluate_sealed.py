"""Evaluate immutable reference infer_plan on sealed cases, preserving all failures."""
import collections,csv,hashlib,importlib.util,json,pathlib,sys
D=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(D));from grammar_oracle import split
R=pathlib.Path(sys.argv[1]).resolve()
run_name=sys.argv[2] if len(sys.argv)>2 else 'first'
manifest=json.loads((D/'sealed_manifest.json').read_text());raw=(D/'sealed_cases.jsonl').read_bytes()
assert hashlib.sha256(raw).hexdigest()==manifest['sha256']
cases=[json.loads(s) for s in raw.splitlines()]
corrections=json.loads((D/'pre_model_oracle_corrections.json').read_text())['corrections']
if run_name=='corrected':corrections.update(json.loads((D/'post_freeze_oracle_corrections.json').read_text())['corrections'])
for c in cases:c.update({k:v for k,v in corrections.get(c['id'],{}).items() if k!='explanation'})
spec=importlib.util.spec_from_file_location('frozen_reference',R/'reference.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
assert hasattr(m,'infer_plan')
normal=lambda t:' '.join(t.lower().rstrip(' .!?').split())
rows=list(csv.DictReader((R/'dataset/sentences.csv').open()))
train={normal(r['text']) for r in rows if r['split']=='train'}
LABELS=['FORWARD','REVERSE','STOP','LIGHT_ON','LIGHT_OFF','WIPER_ON','WIPER_OFF','UNKNOWN','HORN_ON','HORN_OFF']
def labels(actions):
 return [LABELS[a] if isinstance(a,int) else a for a in actions]
def run(case):
 result=m.infer_plan(case['text'])
 actions=labels(result['actions'])
 accepted=result['reason']=='ok'
 result['accepted']=accepted
 clauses,reason=split(case['text'])
 overlap=normal(case['text']) in train
 clause_overlap=sum(normal(c) in train for c in (clauses or []))
 expected=case['expected_actions']
 tested=expected is not None
 correct=not tested or (actions==expected and accepted==(not bool(case['expected_reject'])))
 unsafe=tested and any(a!='STOP' and a not in expected for a in actions)
 return dict(**case,result=result,actual_actions=actions,exact_match=correct,action_scored=tested,unsafe=unsafe,normalized_whole_train_overlap=overlap,normalized_clause_train_overlap=clause_overlap,clause_count=len(clauses or []))
results=[run(c) for c in cases]
(D/(run_name+'_results.jsonl')).write_text(''.join(json.dumps(r,ensure_ascii=True)+'\n' for r in results))
def metric(rr):
 scored=[r for r in rr if r['action_scored']];pos=[r for r in scored if not r['expected_reject']];neg=[r for r in scored if r['expected_reject']]
 return dict(n=len(rr),scored=len(scored),exact_matches=sum(r['exact_match'] for r in scored),exact_plan_accuracy=sum(r['exact_match'] for r in scored)/len(scored) if scored else None,positive_n=len(pos),positive_exact_accuracy=sum(r['exact_match'] for r in pos)/len(pos) if pos else None,positive_coverage=sum(r['result']['accepted'] for r in pos)/len(pos) if pos else None,rejection_n=len(neg),rejection_accuracy=sum(r['exact_match'] for r in neg)/len(neg) if neg else None,unsafe_unexpected_actions=sum(r['unsafe'] for r in scored))
summary=dict(seal=manifest,full=metric(results),primary_whole_train_nonoverlap=metric([r for r in results if not r['normalized_whole_train_overlap']]),by_category={cat:metric([r for r in results if r['category']==cat]) for cat in sorted({r['category'] for r in results})},whole_train_overlap=sum(r['normalized_whole_train_overlap'] for r in results),clause_train_overlap=sum(r['normalized_clause_train_overlap'] for r in results),total_parsed_clauses=sum(r['clause_count'] for r in results),failures=[dict(id=r['id'],category=r['category'],expected=r['expected_actions'],actual=r['actual_actions'],reason=r['result'].get('reason')) for r in results if not r['exact_match']])
(D/(run_name+'_summary.json')).write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
