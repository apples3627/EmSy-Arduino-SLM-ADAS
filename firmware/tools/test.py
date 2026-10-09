#!/usr/bin/env python3
"""Run endpoint and actual MAIN-register orchestration tests without hardware."""
from pathlib import Path
import subprocess,sys,tempfile,re
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,str(ROOT/'firmware/tools/prepare.py')],check=True)
subprocess.run([sys.executable,str(ROOT/'firmware/tests/compute_host_test.py')],cwd=ROOT,check=True)
with tempfile.TemporaryDirectory() as td:
 for name in ('main_policy','actuator_engine','multi_fw_actor','protocol'):
  exe=Path(td)/name
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra',str(ROOT/f'firmware/tests/{name}_test.cpp'),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
subprocess.run([str(ROOT/'firmware/tests/main_integration_test.sh')],cwd=ROOT,check=True)
patterns=[r'\b(?:Serial|Wire|SPI)\s*\.',r'\b(?:digitalWrite|digitalRead|analogWrite|analogRead|millis|micros|delay|delayMicroseconds|tone|noTone)\s*\(',r'#\s*include\s*[<"](?:Servo|SoftwareSerial|Wire|SPI)\.h']
for role in ('main','compute_i2c','compute_spi','actuator_uart'):
 for p in (ROOT/'firmware'/role).rglob('*'):
  if p.suffix not in ('.h','.cpp','.ino') or 'model' in p.parts:continue
  s=p.read_text();s=re.sub(r'/\*.*?\*/|//[^\n]*','',s,flags=re.S)
  for pat in patterns:assert not re.search(pat,s),(str(p),pat)
main=(ROOT/'firmware/main/src/board_app.h').read_text()
assert 'Intent::logits(' not in main,'MAIN must request worker computation'
print('PASS register-only I/O API scan and MAIN does not compute neural logits')
