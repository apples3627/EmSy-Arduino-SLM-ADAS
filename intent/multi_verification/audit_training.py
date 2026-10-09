"""Audit saved model, corpus splits, hashes, and trainer's actual backprop function.
AST extraction executes ONLY forward/lossgrad definitions, never the training loop.
"""
import ast,csv,hashlib,json,pathlib,sys
import numpy as np
R=pathlib.Path(sys.argv[1]).resolve()
trainer=pathlib.Path(sys.argv[2]).resolve()
meta=json.loads((R/'metadata.json').read_text())
rows=list(csv.DictReader((R/'dataset/sentences.csv').open()))
splits={s:[r for r in rows if r['split']==s] for s in ('train','val','test')}
norm=lambda t:' '.join(t.lower().rstrip('.!? ').split())
sets={s:{norm(r['text']) for r in rr} for s,rr in splits.items()}
intersections={a+'_'+b:len(sets[a]&sets[b]) for a,b in [('train','val'),('train','test'),('val','test')]}
source=ast.parse(trainer.read_text()); wanted=[n for n in source.body if isinstance(n,ast.FunctionDef) and n.name in ('forward','lossgrad')]
assert {n.name for n in wanted}=={'forward','lossgrad'},'Trainer function API changed; inspect explicitly'
ns={'np':np};exec(compile(ast.Module(body=wanted,type_ignores=[]),str(trainer),'exec'),ns)
vocab=meta['vocabulary'];ix={w:i for i,w in enumerate(vocab)}
selected=splits['train'][::max(1,len(splits['train'])//17)][:17]
x=np.zeros((len(selected),len(vocab)));y=np.array([int(r['label']) for r in selected])
for n,r in enumerate(selected):
 words=norm(r['text']).split()
 for w in words:x[n,ix[w]]+=1/len(words)
rng=np.random.default_rng(int(meta['seed']))
initial=[rng.normal(0,.2,(len(vocab),16)),rng.normal(0,.2,(16,64)),np.zeros(64),rng.normal(0,.2,(64,10)),np.zeros(10)]
with np.load(R/'model_float.npz') as z:trained=[z[k].astype(float) for k in ('e','w','b','o','c')]
checks=[]
for stage,p in [('initial',initial),('trained',trained)]:
 _,g=ns['lossgrad'](p,x,y)
 for k,name in enumerate(('e','w','b','o','c')):
  for flat in np.argsort(np.abs(g[k]).ravel())[-3:]:
   pos=np.unravel_index(flat,p[k].shape);old=p[k][pos];eps=1e-5
   p[k][pos]=old+eps;hi=ns['lossgrad'](p,x,y)[0]
   p[k][pos]=old-eps;lo=ns['lossgrad'](p,x,y)[0];p[k][pos]=old
   numeric=(hi-lo)/(2*eps);error=abs(numeric-g[k][pos])
   checks.append(dict(stage=stage,tensor=name,index=list(map(int,pos)),analytical=float(g[k][pos]),numeric=float(numeric),absolute_error=float(error)))
out=dict(corpus_counts={s:len(v) for s,v in splits.items()},normalized_split_intersections=intersections,dataset_sha256=hashlib.sha256((R/'dataset/sentences.csv').read_bytes()).hexdigest(),model_sha256=hashlib.sha256((R/'model_int8.npz').read_bytes()).hexdigest(),metadata_hash_match=hashlib.sha256((R/'model_int8.npz').read_bytes()).hexdigest()==meta['model_sha256'],all_tensors_updated=all(not np.array_equal(a,b) for a,b in zip(initial,trained)),selected_step=meta.get('selected_step'),threshold=meta.get('threshold'),margin=meta.get('margin'),gradient_checks=checks,max_gradient_error=max(c['absolute_error'] for c in checks),gradient_pass=all(c['absolute_error']<1e-7 for c in checks),scope='Actual lossgrad AST; finite differences at initial and saved checkpoint; source review still required for VAL-only checkpoint and threshold selection.')
pathlib.Path(__file__).with_name('training_audit.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps({k:v for k,v in out.items() if k!='gradient_checks'},indent=2))
