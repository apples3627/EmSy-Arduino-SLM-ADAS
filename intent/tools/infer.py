#!/usr/bin/env python3
import sys,json,pathlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
from reference import infer_plan
args=[a for a in sys.argv[1:] if a!='--plan']
if args:print(json.dumps(infer_plan(' '.join(args))))
else:
 for line in sys.stdin:print(json.dumps(infer_plan(line.rstrip('\n'))))
