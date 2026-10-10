#!/usr/bin/env python3
"""Run portable ADAS policy/transport and capture register tests without hardware."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory(prefix='adas-nodes-') as tmp:
    for name,flags in [('policy',[]),('capture',['-I'+str(root/'adas_nodes_fake_avr')])]:
        binary=Path(tmp)/name
        subprocess.run(['c++','-std=c++17','-O2',*flags,str(root/f'adas_nodes_{name}_test.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
