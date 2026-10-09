import csv,importlib.util,json,pathlib
import numpy as np
D=pathlib.Path(__file__).resolve().parent;R=D.parent
spec=importlib.util.spec_from_file_location('reference',R/'reference.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
a,meta=m.load_model();rows=[r for r in csv.DictReader((R/'dataset/sentences.csv').open()) if r['split']=='test'];ix={w:i for i,w in enumerate(meta['vocabulary'])};x=np.zeros((len(rows),len(ix)));y=np.array([int(r['label']) for r in rows]);guards=[]
for n,r in enumerate(rows):
 words,why=m.normalize(r['text']);assert words
 for w in words:x[n,ix[w]]+=1/len(words)
 guards.append(m.is_guarded(words))
z=np.maximum((x@(a['e'].astype(float)*a['se']))@(a['w'].astype(float)*a['sw'])+a['b'],0)@(a['o'].astype(float)*a['so'])+a['c']
raw=z.argmax(1);eff=np.array([m.decision(zz,g,meta['threshold'],meta['margin'])[0] for zz,g in zip(z,guards)]);positive=y!=7;accepted=eff!=7
out=dict(cases=len(rows),raw_nn_accuracy=float(np.mean(raw==y)),raw_nn_wrong=int(np.sum(raw!=y)),guarded_accuracy=float(np.mean(eff==y)),positive_coverage=float(np.mean(accepted[positive])),guarded_unsafe_actions=int(np.sum((eff!=y)&accepted)),scope='Independent vectorized saved-tensor calculation on generated synthetic English test corpus; not public real-user data. Vectorized arithmetic differs from target scalar float32; separately sealed scalar/C++ scores verify kernel parity.')
(D/'synthetic_test_recomputation.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
