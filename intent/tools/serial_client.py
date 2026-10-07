#!/usr/bin/env python3
"""UART transport only, never loads NumPy, vocabulary, weights or predicts intent."""
import argparse,time
import serial
p=argparse.ArgumentParser();p.add_argument('port');a=p.parse_args()
with serial.Serial(a.port,9600,timeout=1) as board:
 time.sleep(2)
 while True:
  line=board.readline().decode('ascii',errors='replace').strip()
  if line:print(line)
  if 'READY' in line:break
 while True:
  try:text=input('English> ')
  except (EOFError,KeyboardInterrupt):break
  if not text.isascii() or len(text)>96:print('Maximum 96 ASCII bytes.');continue
  board.write((text+'\n').encode('ascii'));board.flush()
  deadline=time.monotonic()+30
  while time.monotonic()<deadline:
   line=board.readline().decode('ascii',errors='replace').strip()
   if line:print(line)
   if line=='READY':break
  else:raise TimeoutError('No board READY')
