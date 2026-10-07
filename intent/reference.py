"""Portable contract and independently scalar float32 reference."""
import pathlib,json,re
import numpy as np
ROOT=pathlib.Path(__file__).resolve().parent
LABELS=['FORWARD','REVERSE','STOP','LIGHT_ON','LIGHT_OFF','WIPER_ON','WIPER_OFF','UNKNOWN','HORN_ON','HORN_OFF']
MAX_BYTES=96;MAX_TOKENS=16;EMBED=16;HIDDEN=64
GUARD={'not','no','never','and','or','then','if','when','unless','whether','what','why','how','where','wonder','said','was','is','are','likes','maybe','yesterday','while','before','after','because','without','only','maybe','might','should'}
def normalize(text):
 try:raw=text.encode('ascii')
 except UnicodeEncodeError:return None,'non_ascii'
 if len(raw)>MAX_BYTES:return None,'too_long'
 if not raw or any(c==0 for c in raw):return None,'empty_or_nul'
 if any(c<32 or c>126 for c in raw):return None,'unsupported'
 text=text.lower().strip(' ')
 text=text.rstrip('.!?').rstrip(' ')
 if any(not(c==' ' or 'a'<=c<='z') for c in text):return None,'unsupported'
 words=text.split()
 if not words:return None,'empty'
 if len(words)>MAX_TOKENS:return None,'too_many_tokens'
 return words,'ok'
def polite_like(words):
 words=list(words)
 if words and words[0] in {'please','kindly','just'}:words=words[1:]
 return (len(words)>=6 and words[:5] in [['i','would','like','you','to'],['we','would','like','you','to']]) or (len(words)>=5 and words[:4]==['would','you','like','to'])
def is_guarded(words):
 return bool(GUARD.intersection(words)) or ('like' in words and not polite_like(words))
def prepare(text,vocab):
 words,reason=normalize(text)
 if words is None:return None,reason,False
 mapping={s:i for i,s in enumerate(vocab)}
 if any(w not in mapping for w in words):return None,'oov',is_guarded(words)
 return np.array([mapping[w] for w in words],dtype=int),'ok',is_guarded(words)
def quantize(a):
 scale=np.float32(max(float(np.max(np.abs(a)))/127,1e-12));return np.rint(a/scale).clip(-127,127).astype(np.int8),scale

def quant_logits(ids,a,lo=0,hi=HIDDEN,bias=True):
 f=np.zeros(EMBED,np.float32)
 for t in ids:
  for k in range(EMBED):f[k]=np.float32(f[k]+np.float32(np.float32(a['e'][t,k])*a['se']))
 for k in range(EMBED):f[k]=np.float32(f[k]/np.float32(len(ids)))
 out=a['c'].copy() if bias else np.zeros(10,np.float32)
 for j in range(lo,hi):
  z=np.float32(a['b'][j])
  for k in range(EMBED):z=np.float32(z+np.float32(f[k]*np.float32(np.float32(a['w'][k,j])*a['sw'])))
  z=max(z,np.float32(0))
  for t in range(10):out[t]=np.float32(out[t]+np.float32(z*np.float32(np.float32(a['o'][j,t])*a['so'])))
 return out

def decision(logits,guard,threshold,margin):
 if not np.isfinite(logits).all():return 7,'nonfinite',0.,0.
 prob=np.exp(logits.astype(np.float64)-max(logits));prob/=prob.sum();order=np.argsort(-prob,kind='stable');t=int(order[0]);conf=float(prob[t]);gap=float(prob[t]-prob[order[1]])
 if guard:return 7,'safety_guard',conf,gap
 if t==7:return 7,'unknown',conf,gap
 if conf<threshold or gap<margin:return 7,'low_confidence',conf,gap
 return t,'accepted',conf,gap

def load_model():
 with np.load(ROOT/'model_int8.npz') as s:a={k:s[k] for k in s.files}
 meta=json.loads((ROOT/'metadata.json').read_text());return a,meta

def infer(text):
 a,m=load_model();ids,reason,guard=prepare(text,m['vocabulary'])
 if ids is None:return dict(text=text,raw_label=None,effective_label='UNKNOWN',reason=reason,scores=None,policy_action='GLOBAL_STOP')
 z=quant_logits(ids,a);t,why,c,g=decision(z,guard,m['threshold'],m['margin'])
 return dict(text=text,raw_label=LABELS[int(np.argmax(z))],effective_label=LABELS[t],reason=why,confidence=c,margin=g,scores=z.tolist(),policy_action='GLOBAL_STOP' if t==7 else LABELS[t])
