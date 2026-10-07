#!/usr/bin/env python3
import pathlib, subprocess, tempfile
import numpy as np
from train import VOCAB,CTX,HID,EMB
ROOT=pathlib.Path(__file__).resolve().parents[1]
with np.load(ROOT/'model.npz') as saved: a={k:saved[k] for k in saved.files}
def encode(text):
    # Match byte-wise ASCII normalization, including non-ASCII UTF-8 bytes.
    out=[]
    for c in text.encode():
        if 65<=c<=90:c+=32
        ch=chr(c); out.append(VOCAB.index(ch) if ch in VOCAB else 1)
    return out

def context(text):return ([0]*CTX+encode(text))[-CTX:]
def logits(ctx):
    # Scalar float32 loop replicates the AVR/C++ summation order.
    h=np.zeros(HID,dtype=np.float32)
    for j in range(HID):
        s=np.float32(a['b'][j])
        for i in range(CTX):
            for k in range(EMB):
                e=np.float32(a['e'][ctx[i],k])*a['se']; w=np.float32(a['w'][i*EMB+k,j])*a['sw']
                s=np.float32(s+np.float32(e*w))
        h[j]=max(s,0)
    z=a['c'].copy()
    for t in range(len(VOCAB)):
        for j in range(HID):z[t]=np.float32(z[t]+np.float32(h[j]*np.float32(np.float32(a['o'][j,t])*a['so'])))
    return z

def generate(text):
    ctx=context(text); result=''
    for _ in range(80):
        t=int(np.argmax(logits(ctx)))
        if not t:break
        result+=VOCAB[t];ctx=ctx[1:]+[t]
    return result
with tempfile.TemporaryDirectory() as td:
    exe=pathlib.Path(td)/'host'; subprocess.run(['c++','-std=c++11','-O2','-ffp-contract=off',str(ROOT/'tools/host.cpp'),'-o',str(exe)],check=True)
    contexts=['','hello','the cat','a robot','HELLO','!!!','안녕','a'*100,'the sun','hello\nfriend']
    worst=0
    for text in contexts:
        got=np.array(subprocess.check_output([str(exe),'logits',text],text=True).split(),dtype=np.float32)
        expected=logits(context(text)); error=float(np.max(np.abs(got-expected)));worst=max(worst,error)
        np.testing.assert_allclose(got,expected,rtol=2e-6,atol=2e-5)
    for text in ['hello','the cat','a robot']:
        got=subprocess.check_output([str(exe),'generate',text],text=True); expected=generate(text)
        assert got==expected,(text,got,expected)
        assert got.endswith('.') and len(got)<80,(text,got)
        print(repr(text+got))
    print(f'PASS: {len(contexts)} logit contexts, 3 full generations; max logit error={worst:.9g}')
