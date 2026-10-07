#!/usr/bin/env python3
import sys,json,pathlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
from reference import infer
if len(sys.argv)>1:print(json.dumps(infer(' '.join(sys.argv[1:]))))
else:
 for line in sys.stdin:print(json.dumps(infer(line.rstrip('\n'))))
