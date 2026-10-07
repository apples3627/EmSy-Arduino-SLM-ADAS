#!/usr/bin/env python3
"""Copy frozen portable model and protocol into standalone Arduino sketches."""
from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[2]
for role in ('main','compute_i2c','compute_spi','actuator_uart'):
    folder=ROOT/'firmware'/role/'src'
    folder.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'firmware/common/protocol.h',folder/'protocol.h')
    if role!='actuator_uart':
        model=folder/'model'
        shutil.copytree(ROOT/'intent/uno_probe/src',model,dirs_exist_ok=True)
print('Prepared standalone src/protocol.h and frozen src/model/ assets')
