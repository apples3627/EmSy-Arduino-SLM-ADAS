#!/usr/bin/env python3
"""Unchanged full H64+C scores against the independent scalar Python model."""
from pathlib import Path
import re, subprocess, sys, tempfile
import numpy as np
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'intent'))
import reference
source=root/'firmware/tests/topology_slm_uart_test.cpp'
samples=re.findall(r'"([^"]+)"',source.read_text().split('const char* samples[]=')[1].split(';')[0])
assert len(samples)==33
model,meta=reference.load_model()
with tempfile.TemporaryDirectory(prefix='topology-slm-uart-') as tmp:
    binary=Path(tmp)/'test'
    subprocess.run(['c++','-std=c++11','-O2','-ffp-contract=off',str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    rows=np.array([[float(x) for x in row.split()] for row in subprocess.check_output([str(binary),'--scores'],text=True).splitlines()],np.float32)
    worst=0
    for text,row in zip(samples,rows):
        ids,reason,guard=reference.prepare(text,meta['vocabulary'])
        assert reason=='ok',text
        expected=reference.quant_logits(ids,model)
        np.testing.assert_allclose(row,expected,rtol=2e-6,atol=2e-5)
        worst=max(worst,float(np.max(np.abs(row-expected))))
    print(f'PASS independent model956 fullNN parity: {len(samples)} phrases; max error={worst:.9g}')
