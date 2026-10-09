"""Independent manually specified combinations; never consumes training/model files."""
import json, hashlib, pathlib
D=pathlib.Path(__file__).resolve().parent
cases=[]
def add(text, actions=None, reject=None, category='positive', grammar=None):
 cases.append(dict(id=f'M{len(cases)+1:03}',text=text,expected_actions=actions,expected_reject=reject,category=category,expected_grammar_valid=grammar))
# Explicit, standard-domain imperative clauses. The combinations are independent.
phrases=[('move forward','FORWARD'),('go backward','REVERSE'),('turn on the lights','LIGHT_ON'),('switch off the lights','LIGHT_OFF'),('turn on the wiper','WIPER_ON'),('switch off the wiper','WIPER_OFF'),('turn on the horn','HORN_ON'),('switch off the horn','HORN_OFF')]
for text, label in phrases:
 add(text,[label]);add('please '+text,[label]);add('kindly '+text,[label])
# Two-clause ordering and object binding; no inverse component pairs.
pairs=[(0,2),(0,3),(0,4),(0,5),(0,6),(0,7),(1,2),(1,3),(1,4),(1,5),(1,6),(1,7),(2,4),(2,5),(2,6),(2,7),(3,4),(3,5),(3,6),(3,7),(4,6),(4,7),(5,6),(5,7)]
seps=[' and ',' also ',' plus ', ', ', '; ', ' AND ']
for n,(i,j) in enumerate(pairs):
 a,b=phrases[i],phrases[j];add(a[0]+seps[n%6]+b[0],[a[1],b[1]])
for i,j in [(3,6),(6,3),(2,7),(7,2),(4,3),(3,4),(1,7),(7,1)]:
 a,b=phrases[i],phrases[j];add('please '+a[0]+', and '+b[0],[a[1],b[1]])
triples=[(0,2,6),(0,3,7),(1,4,6),(1,5,7),(2,4,6),(3,5,7),(6,3,4),(7,2,5),(2,6,0),(5,1,3),(4,7,0),(7,5,3)]
for n,inds in enumerate(triples):
 sep=seps[n%6];add(sep.join(phrases[i][0] for i in inds),[phrases[i][1] for i in inds])
quads=[(0,2,4,6),(0,3,5,7),(1,2,5,6),(1,3,4,7),(6,4,2,0),(7,5,3,1),(3,6,1,5),(4,0,7,2)]
for n,inds in enumerate(quads):
 add(seps[n%6].join(phrases[i][0] for i in inds),[phrases[i][1] for i in inds])
for t,l in [('lights on','LIGHT_ON'),('lights off','LIGHT_OFF'),('horn on','HORN_ON'),('horn off','HORN_OFF'),('wiper on','WIPER_ON'),('wiper off','WIPER_OFF')]:add(t,[l])
for t,act in [('lights off and horn on',['LIGHT_OFF','HORN_ON']),('horn off and lights on',['HORN_OFF','LIGHT_ON']),('horn on and lights off',['HORN_ON','LIGHT_OFF']),('lights on and horn off',['LIGHT_ON','HORN_OFF'])]:add(t,act)
for t,act in [('move forward and move forward',['FORWARD']),('lights on, lights on; horn off',['LIGHT_ON','HORN_OFF']),('horn on and lights off and horn on',['HORN_ON','LIGHT_OFF']),('wiper off plus wiper off plus wiper off plus wiper off',['WIPER_OFF'])]:add(t,act,category='duplicate')
for t in ['stop','stop and turn on the lights','move forward and stop','lights on and stop and horn on','stop; lights off; horn off; wiper on','horn on, lights off, wiper off, stop','move forward and stop and go backward']:
 add(t,['STOP'],category='stop')
for t in ['move forward and go backward','go backward and move forward','lights on and lights off','horn off and horn on','wiper on and wiper off','lights on; horn off; lights off','move forward; lights on; horn on; go backward']:
 add(t,[],True,'conflict')
for t in ['turn on lights and horn','lights and horn on','move forward and sing a song','lights off and open the door','horn on; wiper off; bake bread','lights on, horn off, move forward, fly away']:
 add(t,[],True,'unknown')
for t in ['do not move forward','never turn on the horn','lights on and do not move forward','if it rains turn on the wiper','lights on or horn on','move forward then lights on','lights on and then horn on','turn on lights when dark']:
 add(t,[],True,'guard')
for t in ['', '   ','and move forward','move forward and','lights on and and horn on','lights on,,horn off','lights on;;horn off','lights on; ;horn off','lights on, and','lights on and horn off and wiper on and move forward and lights on']:
 add(t,[],True,'grammar',False)
add('forward'+' '*89,['FORWARD'],category='boundary',grammar=True)
add('forward'+' '*90,[],True,'grammar',False)
add('forward'+' '*89+';'+' '*86+'lights on',['FORWARD','LIGHT_ON'],category='boundary',grammar=True)
add('forward'+' '*89+';'+' '*87+'lights on',[],True,'grammar',False)
add('go '*15+'forward',None,None,'token_boundary',True)
add('go '*16+'forward',[],True,'grammar',False)
add('lights on\x00 and horn off',[],True,'grammar',False)
add('lights on and 경적 켜기',[],True,'grammar',False)
for t in ['sing a song and stop','stop and bake bread','do not move forward and stop','lights on or horn on; stop','lights on and lights off and stop','go '*16+'forward; stop']:
 add(t,['STOP'],category='stop_dominance')
for t in ['stop and', 'stop; lights on; horn on; wiper on; move forward']:
 add(t,[],True,'grammar',False)
assert 100<=len(cases)<=150,len(cases)
data=''.join(json.dumps(x,ensure_ascii=True,separators=(',',':'))+'\n' for x in cases).encode()
(D/'sealed_cases.jsonl').write_bytes(data)
manifest=dict(format_version=1,case_count=len(cases),sha256=hashlib.sha256(data).hexdigest(),authorship='Manually specified independent imperative templates and combinations; no training/model files consumed.',scope='Known standard English words, explicit clauses. Grammar-only token-limit diagnostic excluded from action accuracy.',status='SEALED BEFORE NEW MODEL ACCESS')
(D/'sealed_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps(manifest,indent=2))
