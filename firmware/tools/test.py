#!/usr/bin/env python3
"""Run endpoint and actual MAIN-register orchestration tests without hardware."""
from pathlib import Path
import subprocess,sys,tempfile,re
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,str(ROOT/'firmware/tools/prepare.py')],check=True)
subprocess.run([sys.executable,str(ROOT/'firmware/tests/topology_slm_uart_host_test.py')],cwd=ROOT,check=True)
with tempfile.TemporaryDirectory() as td:
 for name in ('engine','lcd','register'):
  exe=Path(td)/name
  cmd=['c++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(ROOT/'firmware/tests/topology_peripheral_fake_avr'),str(ROOT/f'firmware/tests/topology_peripheral_{name}_test.cpp'),'-o',str(exe)]
  subprocess.run(cmd,check=True);subprocess.run([str(exe)],check=True)
with tempfile.TemporaryDirectory() as td:
 exe=Path(td)/'uart_repair'
 subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-I'+str(ROOT/'firmware/verification/topology4/fake_avr'),str(ROOT/'firmware/tests/uart_repair_test.cpp'),str(ROOT/'firmware/tests/uart_repair_peer.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
with tempfile.TemporaryDirectory() as td:
 exe=Path(td)/'i2c_finish'
 subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra',str(ROOT/'firmware/tests/i2c_finish_test.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
subprocess.run([sys.executable,str(ROOT/'firmware/verification/topology4/run.py')],cwd=ROOT,check=True)
patterns=[r'\b(?:Serial|Wire|SPI)\s*\.',r'\b(?:digitalWrite|digitalRead|analogWrite|analogRead|millis|micros|delay|delayMicroseconds|tone|noTone)\s*\(',r'#\s*include\s*[<"](?:Servo|SoftwareSerial|Wire|SPI)\.h']
for role in ('main','slm_uart','adas_i2c','peripheral_spi'):
 for p in (ROOT/'firmware'/role).rglob('*'):
  if p.suffix not in ('.h','.cpp','.ino') or 'model' in p.parts:continue
  s=p.read_text();s=re.sub(r'/\*.*?\*/|//[^\n]*','',s,flags=re.S)
  for pat in patterns:assert not re.search(pat,s),(str(p),pat)
main=(ROOT/'firmware/main/src/board_app.h').read_text()
assert 'Intent::logits(' not in main,'MAIN must request worker computation'
print('PASS register-only I/O API scan and MAIN does not compute neural logits')
