#!/usr/bin/env python3
"""Run endpoint and actual MAIN-register orchestration tests without hardware."""
from pathlib import Path
import subprocess,sys,tempfile,re
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,str(ROOT/'firmware/tools/prepare.py')],check=True)
for script in ('slm_full_host_test.py','adas_nodes_host_test.py'):
 subprocess.run([sys.executable,str(ROOT/'firmware/tests'/script)],cwd=ROOT,check=True)
subprocess.run([sys.executable,str(ROOT/'firmware/verification/adas/run.py')],cwd=ROOT,check=True)
with tempfile.TemporaryDirectory() as td:
 exe=Path(td)/'arbiter_main'
 subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware/tests/main_fake_avr'),str(ROOT/'firmware/tests/adas_arbiter_main_test.cpp'),str(ROOT/'firmware/tests/adas_arbiter_bridge.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
 exe=Path(td)/'arbiter_actor'
 subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware/tests/main_fake_avr'),str(ROOT/'firmware/tests/adas_arbiter_actuator_register_test.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
patterns=[r'\b(?:Serial|Wire|SPI)\s*\.',r'\b(?:digitalWrite|digitalRead|analogWrite|analogRead|millis|micros|delay|delayMicroseconds|tone|noTone)\s*\(',r'#\s*include\s*[<"](?:Servo|SoftwareSerial|Wire|SPI)\.h']
for role in ('main','slm_spi','adas_i2c','actuator_uart'):
 for p in (ROOT/'firmware'/role).rglob('*'):
  if p.suffix not in ('.h','.cpp','.ino') or 'model' in p.parts:continue
  s=p.read_text();s=re.sub(r'/\*.*?\*/|//[^\n]*','',s,flags=re.S)
  for pat in patterns:assert not re.search(pat,s),(str(p),pat)
main=(ROOT/'firmware/main/src/board_app.h').read_text()
assert 'Intent::logits(' not in main,'MAIN must request worker computation'
print('PASS register-only I/O API scan and MAIN does not compute neural logits')
