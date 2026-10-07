"""Independent metric and leakage helpers; no trainer dependency."""
import collections, re
LABELS=tuple(range(10))
def normalize(s):
 return ' '.join(re.findall(r'[a-z0-9]+',s.lower()))
def metrics(rows):
 cm=[[0]*10 for _ in LABELS]
 for r in rows: cm[r['expected']][r['decision']]+=1
 n=sum(map(sum,cm)); f=[]; per={}
 for y in LABELS:
  tp=cm[y][y]; support=sum(cm[y]); predicted=sum(row[y] for row in cm)
  precision=tp/predicted if predicted else 0.; recall=tp/support if support else 0.
  f1=2*precision*recall/(precision+recall) if precision+recall else 0.
  if support: f.append(f1)
  per[str(y)]={'support':support,'correct':tp,'precision':precision,'recall':recall,'f1':f1,'acceptance':sum(cm[y][p] for p in LABELS if p!=7)/support if support else None}
 known=[r for r in rows if r['expected']!=7]
 dangerous=[r for r in rows if r['decision'] in (0,1,3,5,8) and r['decision']!=r['expected']]
 return {'n':n,'accuracy':sum(cm[y][y] for y in LABELS)/n if n else 0,'macro_f1':sum(f)/len(f) if f else 0,'known_accuracy':sum(r['expected']==r['decision'] for r in known)/len(known) if known else None,'dangerous_false_actions':len(dangerous),'confusion_matrix':cm,'per_class':per}
def split_leakage(rows):
 by=collections.defaultdict(set)
 for r in rows: by[normalize(r['text'])].add(r['split'])
 return {s:sorted(v) for s,v in by.items() if len(v)>1}
