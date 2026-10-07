import csv,json,hashlib,sys,collections,argparse
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from reference import infer,load_model,prepare,quant_logits,LABELS,normalize
from audit_metrics import metrics,split_leakage
V=Path(__file__).parent
parser=argparse.ArgumentParser();parser.add_argument('--holdout',default='sealed_holdout.jsonl');parser.add_argument('--suffix',default='');parser.add_argument('--frozen',default='ecfd77b65340561d65f1e83093d59d1f299f7e0b');args=parser.parse_args()
a,m=load_model();sealed=[json.loads(s) for s in (V/args.holdout).read_text().splitlines()]
results=[]
for case in sealed:
 r=infer(case['text']);r.update(case);r['decision']=LABELS.index(r['effective_label']);r['raw_decision']=LABELS.index(r['raw_label']) if r['raw_label'] else None
 words,_=normalize(case['text']);r['oov_tokens']=sorted(set(words or [])-set(m['vocabulary']));results.append(r)
(V/f'sealed_results{args.suffix}.jsonl').write_text(''.join(json.dumps(r,sort_keys=True)+'\n' for r in results))
semantic=[r for r in results if r['suite']=='semantic'];known=[r for r in semantic if r['expected']!=7];invocab=[r for r in known if not r['oov_tokens']];raw=[dict(r,decision=r['raw_decision']) for r in semantic if r['raw_decision'] is not None]
summary={'frozen_commit':args.frozen,'model_sha256':hashlib.sha256((ROOT/'model_int8.npz').read_bytes()).hexdigest(),'model_hash_matches':hashlib.sha256((ROOT/'model_int8.npz').read_bytes()).hexdigest()==m['model_sha256'],'sealed_semantic':metrics(semantic),'raw_on_encodable_cases':metrics(raw),'known_in_vocab':metrics(invocab),'known_oov_cases':len(known)-len(invocab),'known_rejection_reasons':dict(collections.Counter(r['reason'] for r in known if r['decision']==7)),'normalization_results':[(r['id'],r['decision'],r['expected'],r['reason']) for r in results if r['suite']=='normalization'],'malformed_all_rejected':all(r['decision']==7 for r in results if r['suite']=='malformed')}
rows=list(csv.DictReader((ROOT/'dataset/sentences.csv').open()));summary['dataset']={'sha256':hashlib.sha256((ROOT/'dataset/sentences.csv').read_bytes()).hexdigest(),'rows':len(rows),'split_counts':dict(collections.Counter(r['split'] for r in rows)),'split_label_counts':{s:dict(collections.Counter(r['label'] for r in rows if r['split']==s)) for s in ('train','val','test')},'cross_split_normalized_duplicates':split_leakage(rows),'families_by_split':{s:sorted(set(r['family'] for r in rows if r['split']==s)) for s in ('train','val','test')}}
summary['dataset']['sealed_overlaps']={s:sum(' '.join(normalize(r['text'])[0] or []) in {' '.join(normalize(t['text'])[0] or []) for t in rows if t['split']==s} for r in semantic) for s in ('train','val','test')}
# Independently formed vectorized int8 network, not trainer qbatch or portable scalar.
def logits(ids):
 pooled=(a['e'][ids].astype(np.float64)*float(a['se'])).mean(axis=0)
 return np.maximum(pooled@(a['w'].astype(np.float64)*float(a['sw']))+a['b'],0)@(a['o'].astype(np.float64)*float(a['so']))+a['c']
maxerr=0;dataset_results=[];rawcorr=0
for row in rows:
 ids,reason,guard=prepare(row['text'],m['vocabulary']);z=logits(ids);scalar=quant_logits(ids,a);maxerr=max(maxerr,float(abs(z-scalar).max()));rawcorr+=int(np.argmax(z)==int(row['label']))
 r=infer(row['text']);dataset_results.append({'expected':int(row['label']),'decision':LABELS.index(r['effective_label']),'split':row['split']})
summary['dataset']['independent_raw_accuracy']=rawcorr/len(rows);summary['dataset']['independent_matrix_scalar_max_abs_error']=maxerr
summary['dataset']['recomputed_guarded']={s:metrics([r for r in dataset_results if r['split']==s]) for s in ('train','val','test')}
traintexts={' '.join(normalize(r['text'])[0] or []) for r in rows if r['split']=='train'}
summary['nonoverlapping_sealed_semantic']=metrics([r for r in semantic if ' '.join(normalize(r['text'])[0] or []) not in traintexts])
summary['npz_arrays']={}
for filename in ('model_float.npz','model_int8.npz'):
 with np.load(ROOT/filename) as model:summary['npz_arrays'][filename]={k:{'shape':list(model[k].shape),'dtype':str(model[k].dtype)} for k in model.files}
history=json.loads((ROOT/'training_log.json').read_text());summary['loss_history']={'first':history[0],'last':history[-1],'minimum_validation':min(history,key=lambda r:r['val_loss'])}
summary['shard_max_abs_error']=max(float(abs(quant_logits(prepare(r['text'],m['vocabulary'])[0],a)- (quant_logits(prepare(r['text'],m['vocabulary'])[0],a,0,len(a['b'])//2,False)+quant_logits(prepare(r['text'],m['vocabulary'])[0],a,len(a['b'])//2,len(a['b']),False)+a['c'])).max()) for r in rows[::10])
(V/f'audit_summary{args.suffix}.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:v for k,v in summary.items() if k not in ('dataset','sealed_semantic','raw_on_encodable_cases','known_in_vocab')},indent=2));print('semantic',summary['sealed_semantic']['accuracy'],summary['sealed_semantic']['macro_f1'],'known',summary['sealed_semantic']['known_accuracy'],'dangerous',summary['sealed_semantic']['dangerous_false_actions'],'in_vocab',len(invocab),summary['known_in_vocab']['accuracy']);print('dataset',summary['dataset']['split_counts'],'leakage',len(summary['dataset']['cross_split_normalized_duplicates']),'overlap',summary['dataset']['sealed_overlaps'])
