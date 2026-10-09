import hashlib,importlib.util,json,pathlib,subprocess,sys,tempfile
import numpy as np
D=pathlib.Path(__file__).resolve().parent;R=D.parent
spec=importlib.util.spec_from_file_location('reference',R/'reference.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
casefile=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else D/'sealed_cases.jsonl'
run_name=sys.argv[2] if len(sys.argv)>2 else 'cpp'
cases=[json.loads(l) for l in casefile.read_text().splitlines()]
with tempfile.TemporaryDirectory() as td:
 td=pathlib.Path(td);common=['c++','-O2','-std=c++11','-ffp-contract=off']
 for part,name in [(1,'left'),(2,'right')]:subprocess.run(common+[f'-DINTENT_PART={part}','-c',str(D/'shard_host.cpp'),'-o',str(td/(name+'.o'))],check=True)
 subprocess.run(common+[str(D/'plan_host.cpp'),str(td/'left.o'),str(td/'right.o'),'-o',str(td/'plan')],check=True)
 lines=subprocess.check_output([str(td/'plan')],input='\n'.join(c['text'] for c in cases)+'\n',text=True).splitlines()
 assert len(lines)==len(cases)
 results=[];maxerror=maxshard=0.;failures=[];clauses=raw=0
 for c,line in zip(cases,lines):
  got=json.loads(line);want=m.infer_plan(c['text']);same=got['actions']==want['actions'] and m.PLAN_REASONS[got['reason']]==want['reason'];shard=got['shard_actions']==got['actions'] and got['shard_reason']==got['reason']
  for g,w in zip(got['clause_results'],want['clause_results']):
   clauses+=1
   if w['scores'] is not None:
    raw+=1;e=float(np.max(np.abs(np.array(g['full_scores'])-np.array(w['scores']))));s=float(np.max(np.abs(np.array(g['shard_scores'])-np.array(g['full_scores']))));maxerror=max(maxerror,e);maxshard=max(maxshard,s)
   same &= g['full_label']==m.LABELS.index(w['effective_label'])
  if not same or not shard:failures.append(c['id'])
  results.append(dict(id=c['id'],python_cpp_equal=bool(same),full_shard_equal=bool(shard),cpp=got))
(D/(run_name+'_results.jsonl')).write_text(''.join(json.dumps(r)+'\n' for r in results))
out=dict(cases=len(cases),parsed_clauses=clauses,scored_nn_clauses=raw,failures=failures,max_python_cpp_full_logit_error=maxerror,max_cpp_full_shard_logit_error=maxshard,all_equal=not failures,scope='Actual generated full/shard NN kernels and actual shared CommandPlan; each clause sequential, logits compared separately from grammar/guard decisions.')
(D/(run_name+'_summary.json')).write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2));assert not failures
