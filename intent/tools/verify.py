#!/usr/bin/env python3
import sys,pathlib,csv,json,subprocess,tempfile
import numpy as np
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from reference import LABELS,load_model,prepare,quant_logits,decision,normalize
model,meta=load_model();vocab=meta['vocabulary'];rows=list(csv.DictReader((ROOT/'dataset/sentences.csv').open()))
texts=[r['text'] for r in rows if r['split']=='train'][::23]+['HELLO!','  Please move forward!?  ','please stop the wiper','stop everything','honk','stop beeping','do not move forward','go forward and honk','please honk if it rains']
# This parity check deliberately avoids test/audit data; it cannot tune the model.
with tempfile.TemporaryDirectory() as td:
 binaries=[]
 for part in range(3):
  exe=pathlib.Path(td)/str(part);subprocess.run(['c++','-O2','-std=c++11','-ffp-contract=off',f'-DINTENT_PART={part}',str(ROOT/'tools/host.cpp'),'-o',str(exe)],check=True);binaries.append(exe)
 def invoke(exe,texts):
  out=subprocess.run([str(exe)],input='\n'.join(texts)+'\n',text=True,capture_output=True,check=True).stdout
  return [json.loads(line) for line in out.splitlines()]
 full=invoke(binaries[0],texts);one=invoke(binaries[1],texts);two=invoke(binaries[2],texts);worst=0;spliterror=0
 for text,got,left,right in zip(texts,full,one,two):
  ids,reason,guard=prepare(text,vocab)
  if ids is None:continue
  assert got['tokens']==ids.tolist(),text;assert got['guard']==guard,text
  z=quant_logits(ids,model);error=float(np.max(np.abs(z-got['scores'])));worst=max(worst,error)
  np.testing.assert_allclose(got['scores'],z,rtol=1e-6,atol=1e-4)
  merged=np.array(left['scores'])+np.array(right['scores'])+model['c'];err=float(np.max(np.abs(merged-z)));spliterror=max(spliterror,err)
  np.testing.assert_allclose(merged,z,rtol=1e-5,atol=1e-4);assert int(np.argmax(merged))==int(np.argmax(z))
  expected,_,_,_=decision(z,guard,meta['threshold'],meta['margin']);assert got['effective_id']==expected,(text,got,expected)
 invalid=['','a'*97,'go\x00forward','안녕','move\tforward',"don't move",'please '*17,'flibbertigibbet']
 for text in invalid:
  got=json.loads(subprocess.check_output([str(binaries[0]),text],text=True)) if '\x00' not in text else invoke(binaries[0],[text])[0]
  assert got['effective_id']==7 and got['reason_id']!=0,text
print(f'PASS: {len(texts)} train/contract C++/Python contexts, max score error {worst:.9g}; two-shard max error {spliterror:.9g}; 8 invalid inputs rejected')
# Dataset leakage gates on metadata/normalized strings, not test predictions.
seen={};families={}
for row in rows:
 key=' '.join(normalize(row['text'])[0]);assert key not in seen;seen[key]=(row['split'],row['label']);family=row['family'];assert families.get(family,row['split'])==row['split'];families[family]=row['split']
assert set(meta['labels'])==set(LABELS)
print('PASS: global normalized dedup, disjoint template families, fixed label contract')
