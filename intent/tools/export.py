#!/usr/bin/env python3
import sys,pathlib,json,hashlib
import numpy as np
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from reference import GUARD,LABELS
with np.load(ROOT/'model_int8.npz') as z:a={k:z[k] for k in z.files}
m=json.loads((ROOT/'metadata.json').read_text());vocab=m['vocabulary']
def array(name,a,typ):
 vals=','.join(str(int(v)) if typ in ['int8_t','uint16_t'] else f'{float(v):.9e}f' for v in np.asarray(a).ravel())
 return f'static const {typ} {name}[{np.asarray(a).size}] INTENT_STORAGE={{{vals}}};\n'
def blobstrings(name,words):
 blob='';off=[]
 for word in words:off.append(len(blob));blob+=word+'\0'
 return f'static const char {name}_TEXT[] INTENT_STORAGE="'+''.join('\\0' if x=='\0' else x for x in blob)+'";\n'+array(name+'_OFFSETS',off,'uint16_t')
identity=hashlib.sha256((m['model_sha256']+m['vocabulary_sha256']+json.dumps(LABELS)).encode()).hexdigest();m['model_id']=identity[:8];m['labels_version']=2;m['transport']='16 uint16 LE token IDs, unused slots PAD=0xFFFF; embedding pooling ignores PAD; vocabulary96 ID0..95'
(ROOT/'metadata.json').write_text(json.dumps(m,indent=2)+'\n')
config='#pragma once\nnamespace IntentConfig {\nstatic const int VOCAB=96, EMBED=12, HIDDEN=48, LABELS=10, MAX_BYTES=96, MAX_TOKENS=16;\n'
config+=f'static const float THRESHOLD={m["threshold"]}f, MARGIN={m["margin"]}f;\n'
config+=f'static const uint32_t MODEL_ID=0x{m["model_id"]}UL;\nstatic const uint16_t PAD=0xFFFF;\n'
config+=blobstrings('VOCAB',vocab)+blobstrings('LABEL',LABELS)+blobstrings('GUARD',sorted(GUARD))+f'static const int GUARD_COUNT={len(GUARD)};\n}}\n'
# Vocab count and blob prefix must not share the same symbol.
config=config.replace('VOCAB_TEXT','WORD_TEXT').replace('VOCAB_OFFSETS','WORD_OFFSETS')
(ROOT/'generated/config.h').write_text(config)
for name,lo,hi in [('model_full',0,48),('model_i2c',0,24),('model_spi',24,48)]:
 s=f'#pragma once\nnamespace IntentWeights {{\nstatic const int HIDDEN={hi-lo}, OFFSET={lo};\n'
 for key,value,typ in [('E',a['e'],'int8_t'),('W',a['w'][:,lo:hi],'int8_t'),('B',a['b'][lo:hi],'float'),('O',a['o'][lo:hi,:],'int8_t')]:s+=array(key,value,typ)
 for key in ['se','sw','so']:s+=f'static const float {key.upper()}={float(a[key]):.9e}f;\n'
 s+='}\n';(ROOT/f'generated/{name}.h').write_text(s)
(ROOT/'generated/main_bias.h').write_text('#pragma once\nnamespace IntentMain {\n'+array('C',a['c'],'float')+'}\n')
print('Exported full + two original-scale shards, main-only output bias; model ID',m['model_id'])
