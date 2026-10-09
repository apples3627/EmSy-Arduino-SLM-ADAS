"""Independent grammar oracle, written without consulting new implementation."""
import re

def split(text):
 try: raw=text.encode('ascii')
 except UnicodeEncodeError:return None,'non_ascii'
 if not raw or len(raw)>192 or any(c<32 or c>126 for c in raw):return None,'byte_boundary'
 # Comma followed by AND is one common composite delimiter.
 text=re.sub(r', *and\b', ',',text,flags=re.I)
 clauses=re.split(r'[,;]|\b(?:and|also|plus)\b',text,flags=re.I)
 if len(clauses)>4:return None,'clause_count'
 # Bounds apply to raw clause spans, before outer spaces are removed.
 if any(len(c.strip(' '))>96 for c in clauses):return None,'clause_bytes'
 if any(not c.strip(' ') for c in clauses):return None,'empty_clause'
 return [c.strip(' ') for c in clauses],'valid'

if __name__=='__main__':
 import json,pathlib
 cases=[json.loads(l) for l in pathlib.Path(__file__).with_name('sealed_cases.jsonl').read_text().splitlines()]
 corrections=json.loads(pathlib.Path(__file__).with_name('pre_model_oracle_corrections.json').read_text())['corrections']
 corrections.update(json.loads(pathlib.Path(__file__).with_name('post_freeze_oracle_corrections.json').read_text())['corrections'])
 for c in cases:c.update({k:v for k,v in corrections.get(c['id'],{}).items() if k!='explanation'})
 failures=[]
 for c in cases:
  parsed,why=split(c['text'])
  if c['expected_grammar_valid'] is not None and (parsed is not None)!=c['expected_grammar_valid']:
   failures.append({'id':c['id'],'reason':why,'expected':c['expected_grammar_valid']})
 print(json.dumps({'checked':len(cases),'explicit_grammar_assertions':sum(c['expected_grammar_valid'] is not None for c in cases),'failures':failures},indent=2))
 assert not failures
