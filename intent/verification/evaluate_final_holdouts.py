"""Re-evaluate sealed final cases and explicitly labelled older-case regressions."""
import csv,json,hashlib,sys,collections
from pathlib import Path
R=Path(__file__).resolve().parents[1];V=Path(__file__).parent;sys.path.insert(0,str(R))
from reference import infer,LABELS,normalize,load_model
from audit_metrics import metrics,split_leakage
a,m=load_model();assert m['model_id']=='30918170','This report is tied to the frozen V3 model'
rows=list(csv.DictReader((R/'dataset/sentences.csv').open()));sets={s:{' '.join(normalize(r['text'])[0] or []) for r in rows if r['split']==s} for s in ('train','val','test')}
summary={'frozen_commit':'129763735099aba77883ae2ac137715c621a8bfc','model_id':m['model_id'],'model_sha256':hashlib.sha256((R/'model_int8.npz').read_bytes()).hexdigest(),'suites':{}}
for version,file in [(3,'sealed_holdout_v3.jsonl'),(1,'sealed_holdout.jsonl'),(2,'sealed_holdout_v2.jsonl')]:
 out=[]
 for case in map(json.loads,(V/file).read_text().splitlines()):
  p=infer(case['text']);words,_=normalize(case['text']);p.update(case);p['decision']=LABELS.index(p['effective_label']);p['oov_tokens']=sorted(set(words or [])-set(m['vocabulary']));out.append(p)
 (V/f'v3_results_audit{version}.jsonl').write_text(''.join(json.dumps(r,sort_keys=True)+'\n' for r in out));semantic=[r for r in out if r['suite']=='semantic'];novel=[r for r in semantic if ' '.join(normalize(r['text'])[0] or []) not in sets['train']];positive=[r for r in semantic if r['expected']!=7];invocab=[r for r in positive if not r['oov_tokens']]
 raw=lambda subset:metrics([dict(r,decision=LABELS.index(r['raw_label'])) for r in subset if r['raw_label'] is not None])
 summary['suites'][str(version)]={'status':'sealed_first_evaluation' if version==3 else 'post_audit_regression','all':metrics(semantic),'excluding_train_matches':metrics(novel),'raw_encodable':raw(semantic),'raw_encodable_excluding_train_matches':raw(novel),'in_vocab_positive':metrics(invocab),'oov_positive_count':len(positive)-len(invocab),'overlaps':{s:sum(' '.join(normalize(r['text'])[0] or []) in texts for r in semantic) for s,texts in sets.items()},'failures':[{'id':r['id'],'expected':r['expected'],'decision':r['decision'],'reason':r['reason'],'oov_tokens':r['oov_tokens']} for r in semantic if r['expected']!=r['decision']]}
summary['dataset']={'rows':len(rows),'split_counts':dict(collections.Counter(r['split'] for r in rows)),'split_label_counts':{s:dict(collections.Counter(r['label'] for r in rows if r['split']==s)) for s in sets},'cross_split_normalized_duplicates':split_leakage(rows),'dataset_sha256':hashlib.sha256((R/'dataset/sentences.csv').read_bytes()).hexdigest(),'family_splits':{s:sorted(set(r['family'] for r in rows if r['split']==s)) for s in sets}}
(V/'final_v3_summary.json').write_text(json.dumps(summary,indent=2)+'\n');print('Fresh nonoverlap effective',summary['suites']['3']['excluding_train_matches']['accuracy'],'raw',summary['suites']['3']['raw_encodable_excluding_train_matches']['n'],summary['suites']['3']['raw_encodable_excluding_train_matches']['accuracy'])
