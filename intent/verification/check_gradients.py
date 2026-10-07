"""Independent finite differences, never imports/reexecutes the training script."""
import csv,json,sys
from pathlib import Path
import numpy as np
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from reference import normalize,EMBED,HIDDEN
rows=list(csv.DictReader((R/'dataset/sentences.csv').open()));vocab=sorted({w for r in rows if r['split']=='train' for w in normalize(r['text'])[0]});idx={w:i for i,w in enumerate(vocab)}
selected=[r for r in rows if r['split']=='train'][::97][:20]
x=np.zeros((len(selected),len(vocab)));y=np.array([int(r['label']) for r in selected])
for n,r in enumerate(selected):
 words=normalize(r['text'])[0]
 for w in words:x[n,idx[w]]+=1/len(words)
def loss(p):
 f=x@p[0];z=np.maximum(f@p[1]+p[2],0)@p[3]+p[4]
 q=np.exp(z-z.max(1,keepdims=True));q/=q.sum(1,keepdims=True)
 return -np.log(q[np.arange(len(y)),y]).mean()
def grad(p):
 f=x@p[0];pre=f@p[1]+p[2];h=np.maximum(pre,0);z=h@p[3]+p[4]
 q=np.exp(z-z.max(1,keepdims=True));q/=q.sum(1,keepdims=True);q[np.arange(len(y)),y]-=1;q/=len(y)
 dp=(q@p[3].T)*(pre>0)
 return [x.T@(dp@p[1].T),f.T@dp,dp.sum(0),h.T@q,q.sum(0)]
rng=np.random.default_rng(20261007);initial=[rng.normal(0,.2,(len(vocab),EMBED)),rng.normal(0,.2,(EMBED,HIDDEN)),np.zeros(HIDDEN),rng.normal(0,.2,(HIDDEN,10)),np.zeros(10)]
with np.load(R/'model_float.npz') as a:trained=[a[k].astype(float) for k in ('e','w','b','o','c')]
results=[]
for phase,p in [('initial',initial),('trained',trained)]:
 g=grad(p)
 for k,name in enumerate(('e','w','b','o','c')):
  # Nonzero gradients improve diagnostic signal; select top absolute three entries.
  for flat in np.argsort(abs(g[k]).ravel())[-3:]:
   pos=np.unravel_index(flat,p[k].shape);old=p[k][pos];eps=1e-5
   p[k][pos]=old+eps;hi=loss(p);p[k][pos]=old-eps;lo=loss(p);p[k][pos]=old
   num=(hi-lo)/(2*eps);ana=float(g[k][pos]);err=abs(num-ana)
   results.append({'phase':phase,'tensor':name,'index':[int(v) for v in pos],'analytical':ana,'numeric':float(num),'absolute_error':float(err)})
summary={'epsilon':1e-5,'dtype':'float64','samples':len(selected),'checks':results,'max_abs_error':max(r['absolute_error'] for r in results),'passed':all(r['absolute_error']<1e-7 for r in results),'reporting_bug':'training metrics analytical g is overwritten by last training iteration; numeric is initial finite difference; values are unrelated'}
(Path(__file__).parent/'gradient_audit.json').write_text(json.dumps(summary,indent=2)+'\n');print(summary['passed'],summary['max_abs_error'])
