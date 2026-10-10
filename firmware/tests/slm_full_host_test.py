#!/usr/bin/env python3
"""Actual portable SLM job/frames against frozen independent Python scalar model."""
from pathlib import Path
import re,sys,subprocess,tempfile
import numpy as np
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'intent'))
import reference
samples=re.findall(r'"([^"]+)"',(root/'firmware/tests/slm_full_test.cpp').read_text().split('const char* samples[]=')[1].split(';')[0])
assert len(samples)==33
model,meta=reference.load_model()
with tempfile.TemporaryDirectory(prefix='slm-full-') as tmp:
    binary=Path(tmp)/'slm'
    subprocess.run(['c++','-std=c++11','-O2','-ffp-contract=off',str(root/'firmware/tests/slm_full_test.cpp'),'-o',str(binary)],check=True)
    got=np.array([[float(x) for x in line.split()] for line in subprocess.check_output([str(binary)],text=True).splitlines()],np.float32)
    worst=0
    for text,row in zip(samples,got):
        ids,reason,guard=reference.prepare(text,meta['vocabulary']);assert reason=='ok',text
        target=reference.quant_logits(ids,model)
        np.testing.assert_allclose(row,target,rtol=2e-6,atol=2e-5)
        worst=max(worst,float(np.max(np.abs(row-target))))
    # Compound planning supplies each clause as a separate full-SLM job, never sends an OOV connector.
    for text in ['move forward and turn lights on','stop and silence horn','start wipers; honk horn']:
        clauses,reason=reference.split_plan(text);assert reason=='ok'
        for clause in clauses:assert clause in samples,clause
    print(f'PASS full H64+C job protocol and independent Python parity: 33 samples, 3 compound plans; max error={worst:.9g}')
