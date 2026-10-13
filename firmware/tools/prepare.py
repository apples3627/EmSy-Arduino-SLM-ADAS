#!/usr/bin/env python3
"""Copy frozen portable model and protocol into standalone Arduino sketches."""
from pathlib import Path
import shutil,json,re
ROOT=Path(__file__).resolve().parents[2]
meta=json.loads((ROOT/'intent/metadata.json').read_text())
protocol=ROOT/'firmware/common/protocol.h'
text=protocol.read_text()
text=re.sub(r'MODEL_ID=0x[0-9a-fA-F]+UL', 'MODEL_ID=0x'+meta['model_id']+'UL', text)
text=re.sub(r'VOCAB_TAG=0x[0-9a-fA-F]+UL', 'VOCAB_TAG=0x'+meta['vocabulary_sha256'][:8]+'UL', text)
for key,value in [('VOCAB_SIZE',len(meta['vocabulary'])),('LABELS_VERSION',meta['labels_version']),('EMBED_SIZE',meta['embedding']),('HIDDEN_TOTAL',meta['hidden'])]:
    text=re.sub(key+r'=\d+',key+'='+str(value),text)
protocol.write_text(text)
for role in ('main','slm_uart','adas_i2c','peripheral_spi'):
    folder=ROOT/'firmware'/role/'src'
    folder.mkdir(parents=True,exist_ok=True)
    shutil.copy2(ROOT/'firmware/common/protocol.h',folder/'protocol.h')
    if role in ('slm_uart',):
        model=folder/'model'
        shutil.copytree(ROOT/'intent/uno_probe/src',model,dirs_exist_ok=True)
print('Prepared standalone src/protocol.h and frozen src/model/ assets')
