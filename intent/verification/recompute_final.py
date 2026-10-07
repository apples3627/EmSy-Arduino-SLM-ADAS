"""Independent matrix scoring of all final corpus rows and complete header payload checks."""
from pathlib import Path
import csv,json,sys,re,hashlib
import numpy as np
R=Path(__file__).resolve().parents[1];V=Path(__file__).parent;sys.path.insert(0,str(R));from reference import load_model,prepare,normalize,LABELS
from audit_metrics import metrics
a,m=load_model();rows=list(csv.DictReader((R/'dataset/sentences.csv').open()));pooled=[];guards=[]
for r in rows:
 ids,reason,guard=prepare(r['text'],m['vocabulary']);assert ids is not None
 pooled.append(a['e'][ids].astype(np.float64).mean(0)*float(a['se']));guards.append(guard)
z=np.maximum(np.array(pooled)@(a['w'].astype(float)*float(a['sw']))+a['b'],0)@(a['o'].astype(float)*float(a['so']))+a['c']
probs=np.exp(z-z.max(1,keepdims=True));probs/=probs.sum(1,keepdims=True);pred=z.argmax(1);order=np.sort(probs,axis=1);conf=order[:,-1];margin=order[:,-1]-order[:,-2];effective=pred.copy();effective[np.array(guards)|(conf<m['threshold'])|(margin<m['margin'])]=7
result={'scope':'All final corpus matrix scores independent of trainer batch implementation; actual scalar/C++ parity sampled separately','rows':len(rows),'raw':{},'guarded':{}}
for split in ('train','val','test'):
 select=[i for i,r in enumerate(rows) if r['split']==split]
 result['raw'][split]=metrics([{'expected':int(rows[i]['label']),'decision':int(pred[i])} for i in select]);result['guarded'][split]=metrics([{'expected':int(rows[i]['label']),'decision':int(effective[i])} for i in select])
# Complete canonical header tensors, including original shard scales and bias once.
checks={}
def array(text,name,dtype):
 match=re.search(r'\b'+name+r'\[\d+\][^=]*=\{([^}]+)\}',text)
 assert match,name
 return np.array([float(v.rstrip('f')) for v in match.group(1).split(',')],dtype=dtype)
for file,lo,hi in [('model_full',0,64),('model_i2c',0,32),('model_spi',32,64)]:
 text=(R/'uno_probe/src/generated'/f'{file}.h').read_text();checks[file]={}
 for name,expected in [('E',a['e']),('W',a['w'][:,lo:hi]),('B',a['b'][lo:hi]),('O',a['o'][lo:hi,:])]:
  got=array(text,name,expected.dtype);checks[file][name]=bool(np.array_equal(got,expected.ravel()))
 for name,key in [('SCALE_E','se'),('SCALE_W','sw'),('SCALE_O','so')]:
  got=np.float32(float(re.search(r'\b'+name+r'=([^;]+);',text).group(1).rstrip('f')));checks[file][name]=bool(got==a[key])
 bias=(R/'uno_probe/src/generated/main_bias.h').read_text();checks['main_bias_once']=bool(np.array_equal(array(bias,'C',a['c'].dtype),a['c']))
result['header_payload_checks']=checks;result['headers_all_equal']=all(v for row in checks.values() for v in (row.values() if isinstance(row,dict) else [row]))
with np.load(R/'model_float.npz') as f:
 result['quantization_matches_float']={}
 for name,scale in [('e','se'),('w','sw'),('o','so')]:
  expected=np.rint(f[name]/a[scale]).clip(-127,127).astype(np.int8);result['quantization_matches_float'][name]=bool(np.array_equal(expected,a[name]))
history=json.loads((R/'training_log.json').read_text());result['checkpoint_min_val_step']=min(history,key=lambda r:r['val_loss'])['step'];result['selected_step']=m['selected_step'];result['dataset_sha256']=hashlib.sha256((R/'dataset/sentences.csv').read_bytes()).hexdigest();result['metadata_dataset_hash_matches']=result['dataset_sha256']==m['dataset_sha256'];result['train_only_vocabulary_matches']=sorted({w for r in rows if r['split']=='train' for w in normalize(r['text'])[0]})==m['vocabulary'];result['vocabulary_hash_matches']=hashlib.sha256('\n'.join(m['vocabulary']).encode()).hexdigest()==m['vocabulary_sha256'];result['model_sha256']=hashlib.sha256((R/'model_int8.npz').read_bytes()).hexdigest();result['metadata_model_hash_matches']=result['model_sha256']==m['model_sha256']
(V/'final_corpus_recomputation.json').write_text(json.dumps(result,indent=2)+'\n');print('allheader',result['headers_all_equal'],'quant',result['quantization_matches_float']);print('raw',[(s,r['accuracy']) for s,r in result['raw'].items()]);print('guard',[(s,r['accuracy'],r['known_accuracy'],r['dangerous_false_actions']) for s,r in result['guarded'].items()])
