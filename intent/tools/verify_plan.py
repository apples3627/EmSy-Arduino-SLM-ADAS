#!/usr/bin/env python3
import sys,pathlib,json,subprocess,tempfile,random
R=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(R))
from reference import infer_plan,PLAN_REASONS,split_plan
cases=['go forward and turn on the lights','lights off and horn on','lights','horn','wipers','honk','lights off and lights on','reverse plus go forward','lights on and horn on and wipers on and go forward','go forward and zzzzzz','go forward and zzzzzz and stop everything','honk then lights on','turn on lights and horn','lights on, and horn on','lights on and also horn on','lights on; plus horn on','lights on and and horn on','lights on,, horn on','lights on,','  LIGHTS ON!?  , HORN OFF!','honk and honk','and lights on','go forward and lights on and horn on and wipers on and stop everything','a'*97,'x'*193,'go\tforward','']
cases += [json.loads(l)['text'] for l in (R/'dataset/multi_commands.jsonl').read_text().splitlines() if json.loads(l)['split']=='val']
# Grammar byte bounds and generated supported connector parity, no sealed audit access.
rng=random.Random(91)
for _ in range(100):
 clauses=rng.choices(['honk','lights on','wipers off','stop everything','horn off'],k=rng.randint(1,5));cases.append(rng.choice([' and ',' also ',' plus ',',',';',' , and ',' and also ']).join(clauses))
with tempfile.TemporaryDirectory() as td:
 exe=pathlib.Path(td)/'plan';subprocess.run(['c++','-O2','-std=c++11','-ffp-contract=off',str(R/'tools/plan_host.cpp'),'-o',str(exe)],check=True)
 out=subprocess.check_output([str(exe)],input='\n'.join(cases)+'\n',text=True)
 for text,line in zip(cases,out.splitlines()):
  got=json.loads(line);want=infer_plan(text)
  assert got['clauses']==want['clauses'],(text,got,want)
  assert PLAN_REASONS[got['reason']]==want['reason'],(text,got,want)
  assert got['actions']==want['actions'],(text,got,want)
report=dict(cases=len(cases),python_cpp_plan_parity=True,expected_examples={s:infer_plan(s)['actions'] for s in cases[:6]})
(R/'experiments/v4_plan_parity.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
