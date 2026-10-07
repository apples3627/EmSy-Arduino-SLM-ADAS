#!/usr/bin/env python3
"""Manual NumPy Adam; train/validation selection only, frozen test evaluated once."""
import csv,json,pathlib,hashlib,time,platform
import numpy as np
from reference import ROOT,LABELS,EMBED,HIDDEN,normalize,quantize,decision,prepare
SEED=20261007;STEPS=1600
rows=list(csv.DictReader((ROOT/'dataset/sentences.csv').open()))
manifest=json.loads((ROOT/'dataset/manifest.json').read_text())
assert hashlib.sha256((ROOT/'dataset/sentences.csv').read_bytes()).hexdigest()==manifest['sha256']
# Test rows never enter vocabulary/features/training/checkpoint/threshold selection.
vocab=sorted({w for r in rows if r['split']=='train' for w in normalize(r['text'])[0]})
lookup={w:i for i,w in enumerate(vocab)}
def batch(split):
 subset=[r for r in rows if r['split']==split];freq=np.zeros((len(subset),len(vocab)))
 for i,r in enumerate(subset):
  words=normalize(r['text'])[0]
  assert all(w in lookup for w in words),'OOV in authored split'
  for w in words:freq[i,lookup[w]]+=1/len(words)
 return subset,freq,np.array([int(r['label']) for r in subset])
tr,x,y=batch('train');vr,vx,vy=batch('val')
def forward(p,x):
 f=x@p[0];pre=f@p[1]+p[2];h=np.maximum(pre,0);z=h@p[3]+p[4]
 return f,pre,h,z

def lossgrad(p,x,y):
 f,pre,h,z=forward(p,x);pr=np.exp(z-z.max(1,keepdims=True));pr/=pr.sum(1,keepdims=True)
 loss=float(-np.log(pr[np.arange(len(y)),y]+1e-20).mean());dz=pr.copy();dz[np.arange(len(y)),y]-=1;dz/=len(y)
 dh=(dz@p[3].T)*(pre>0)
 return loss,[x.T@(dh@p[1].T),f.T@dh,dh.sum(0),h.T@dz,dz.sum(0)]

rng=np.random.default_rng(SEED);p=[rng.normal(0,.2,(len(vocab),EMBED)),rng.normal(0,.2,(EMBED,HIDDEN)),np.zeros(HIDDEN),rng.normal(0,.2,(HIDDEN,10)),np.zeros(10)]
# Real finite-difference backprop sanity check away from ReLU kinks.
loss,g=lossgrad(p,x[:13],y[:13]);old=p[1][2,3];eps=1e-5
p[1][2,3]=old+eps;plus=lossgrad(p,x[:13],y[:13])[0];p[1][2,3]=old-eps;minus=lossgrad(p,x[:13],y[:13])[0];p[1][2,3]=old
numeric=(plus-minus)/(2*eps);gradient_analytical=float(g[1][2,3]);assert abs(numeric-g[1][2,3])<1e-6
m=[np.zeros_like(a) for a in p];u=[np.zeros_like(a) for a in p];bestloss=1e30;best=None;beststep=0;history=[];start=time.monotonic()
for step in range(1,STEPS+1):
 loss,g=lossgrad(p,x,y)
 for j in range(5):
  m[j]=.9*m[j]+.1*g[j];u[j]=.999*u[j]+.001*g[j]**2
  p[j]-=.02*(m[j]/(1-.9**step))/(np.sqrt(u[j]/(1-.999**step))+1e-8)
 if step%50==0:
  vl=lossgrad(p,vx,vy)[0];history.append(dict(step=step,train_loss=loss,val_loss=vl))
  if vl<bestloss:bestloss=vl;best=[a.copy() for a in p];beststep=step
  if step%200==0:print(f'step {step} train={loss:.5f} val={vl:.5f}',flush=True)
p=best
np.savez(ROOT/'model_float.npz',e=p[0],w=p[1],b=p[2],o=p[3],c=p[4])
e,se=quantize(p[0]);w,sw=quantize(p[1]);o,so=quantize(p[3]);b=p[2].astype(np.float32);c=p[4].astype(np.float32)
a=dict(e=e,w=w,b=b,o=o,c=c,se=se,sw=sw,so=so);np.savez(ROOT/'model_int8.npz',**a)
# Batched quant reference used only for calibration/report, portable scalar tested separately.
def qbatch(freq):return np.maximum((freq@(e.astype(float)*se))@(w.astype(float)*sw)+b,0)@(o.astype(float)*so)+c
vz=qbatch(vx);guards=[bool(prepare(r['text'],vocab)[2]) for r in vr]
def metrics(z,subset,threshold=None,margin=None):
 truth=np.array([int(r['label']) for r in subset]);raw=z.argmax(1);pred=raw.copy()
 if threshold is not None:
  pred=np.array([decision(score,prepare(r['text'],vocab)[2],threshold,margin)[0] for score,r in zip(z,subset)])
 conf=np.zeros((10,10),int)
 for t,q in zip(truth,pred):conf[t,q]+=1
 f1=[];recall=[]
 for i in range(10):
  tp=conf[i,i];fp=conf[:,i].sum()-tp;fn=conf[i].sum()-tp;f1.append(float(2*tp/max(1,2*tp+fp+fn)));recall.append(float(tp/max(1,tp+fn)))
 positive=truth!=7;accepted=pred!=7
 return dict(n=len(truth),accuracy=float((pred==truth).mean()),macro_f1=float(np.mean(f1)),per_class_recall=dict(zip(LABELS,recall)),confusion_matrix=conf.tolist(),positive_coverage=float(accepted[positive].mean()),accepted_action_accuracy=float((pred[accepted]==truth[accepted]).mean()) if accepted.any() else None,unknown_reject_rate=float((pred[~positive]==7).mean()),unsafe_action_errors=int(((pred!=truth)&accepted).sum()))
choices=[]
for threshold in [.5,.6,.7,.8,.9]:
 for margin in [.0,.1,.2]:
  met=metrics(vz,vr,threshold,margin);choices.append((met['positive_coverage']>=.9,met['macro_f1'],threshold,margin,met))
choice=max(choices,key=lambda z:(z[0],z[1],z[2],z[3]));_,_,threshold,margin,vm=choice
assert vm['unsafe_action_errors']==0 and vm['positive_coverage']>=.90, 'Validation safety/coverage target failed; do not evaluate frozen test'
weight_hash=hashlib.sha256((ROOT/'model_int8.npz').read_bytes()).hexdigest()
meta=dict(model_version=3,labels=LABELS,vocabulary=vocab,vocabulary_sha256=hashlib.sha256('\n'.join(vocab).encode()).hexdigest(),architecture='learned word embedding16 + mean pooling + ReLU64 + 10 intent logits',embedding=EMBED,hidden=HIDDEN,max_ascii_bytes=96,max_tokens=16,seed=SEED,steps=STEPS,selected_step=beststep,selection='minimum validation cross-entropy; confidence/margin selected only on validation after int8 export',threshold=threshold,margin=margin,dataset_sha256=manifest['sha256'],model_sha256=weight_hash,model_id=weight_hash[:8],quantization='symmetric int8 per original e/w/o tensor; float32 bias/scales; original scales retained in shards',policy='generic guard/OOV/invalid/low-confidence/UNKNOWN => effective UNKNOWN and GLOBAL_STOP; STOP stops motor/lights/wiper/horn; HORN_ON tone, HORN_OFF noTone',shards=[[0,32],[32,64]],weight_bytes=int(e.size+w.size+o.size+4*(b.size+c.size)+12),numpy_version=np.__version__,python_version=platform.python_version(),training_wall_seconds=time.monotonic()-start,test_usage='V3 synthetic family test evaluated after VAL-selected checkpoint/threshold freeze; prior V1/V2 tests and disclosed audit summaries are regression, third sealed audit not read')
(ROOT/'metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
# Freeze exists before constructing final test features.
fr,fx,fy=batch('test');report={'validation_quantized_guarded':vm,'train_float_raw':metrics(forward(p,x)[3],tr),'val_float_raw':metrics(forward(p,vx)[3],vr),'test_float_raw':metrics(forward(p,fx)[3],fr),'test_int8_raw':metrics(qbatch(fx),fr),'test_int8_guarded':metrics(qbatch(fx),fr),'gradient_check':dict(analytical=gradient_analytical,numeric=float(numeric))}
report['test_int8_guarded']=metrics(qbatch(fx),fr,threshold,margin)
(ROOT/'metrics.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'training_log.json').write_text(json.dumps(history,indent=2)+'\n')
print(json.dumps({'selected_step':beststep,'threshold':threshold,'margin':margin,'vocab':len(vocab),'weight_bytes':meta['weight_bytes'],'test_raw_f1':report['test_int8_raw']['macro_f1'],'test_guarded':report['test_int8_guarded']},indent=2))
