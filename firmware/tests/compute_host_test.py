#!/usr/bin/env python3
"""Compile actual shard workers and the frozen full kernel; verify protocol and score composition."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='compute-test-') as tmp:
    outputs=[]
    for name,source,flags in [('i2c','compute_worker_test.cpp',['-DTEST_PART=1']),('spi','compute_worker_test.cpp',['-DTEST_PART=2']),('full','compute_full_reference.cpp',[])]:
        binary=Path(tmp)/name
        subprocess.run(['c++','-std=c++11','-O2',*flags,str(root/source),'-o',str(binary)],check=True)
        outputs.append([[float(x) for x in line.split()] for line in subprocess.check_output([str(binary)],text=True).splitlines()])
    left,right,full=outputs
    assert len(left)==len(right)==10 and len(full)==11
    max_error=0
    for a,b,target in zip(left,right,full[:10]):
        result=[x+y+bias for x,y,bias in zip(a,b,full[-1])]
        max_error=max(max_error,max(abs(x-y) for x,y in zip(result,target)))
        assert max(abs(x-y) for x,y in zip(result,target))<1e-4
        assert max(range(10),key=result.__getitem__)==max(range(10),key=target.__getitem__)
    print(f'PASS both worker protocols, RX queues, ten model samples; max score error {max_error:.3g}')
