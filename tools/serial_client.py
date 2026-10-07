#!/usr/bin/env python3
"""Transport only: no model, weights, or inference on this computer."""
import argparse,time
import serial
ap=argparse.ArgumentParser();ap.add_argument('port');args=ap.parse_args()
with serial.Serial(args.port,9600,timeout=1) as board:
    time.sleep(2)
    def receive():
        deadline=time.monotonic()+120
        while time.monotonic()<deadline:
            line=board.readline().decode('ascii',errors='replace').strip()
            if line:print(line)
            if line=='READY':return
        raise TimeoutError('Board did not become READY within 120 seconds')
    receive()
    while True:
        try:prompt=input('prompt> ')
        except (EOFError,KeyboardInterrupt):break
        if not prompt.isascii():print('Use ASCII English prompts.');continue
        if len(prompt)>63:print('Maximum 63 bytes.');continue
        board.write((prompt+'\n').encode('ascii'));board.flush();receive()
