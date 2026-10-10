#!/usr/bin/env python3
"""Compile actual frozen sources independently; no upload, prepare, source edits."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
p=argparse.ArgumentParser();p.add_argument('--source-root',type=Path,default=ROOT);p.add_argument('--build-uno',action='store_true');a=p.parse_args();src=a.source_root.resolve()
def run(cmd):
 r=subprocess.run(cmd,capture_output=True,text=True);return dict(command=cmd,exit_code=r.returncode,output=r.stdout+r.stderr)
seal=json.loads((HERE/'scenario_seal.json').read_text());assert hashlib.sha256((HERE/'scenarios.json').read_bytes()).hexdigest()==seal['sha256']
commit=run(['git','-C',str(src),'rev-parse','HEAD'])['output'].strip()
status=run(['git','-C',str(src),'status','--porcelain','--','firmware/main','firmware/slm_spi','firmware/adas_i2c','firmware/actuator_uart','firmware/common'])['output'].strip()
results=dict(source_commit=commit,source_dirty=status,model_id='0x95616d56',scenario_seal=seal,hardware_verified=False,host_tests=[],builds=[])
headers={'protocol_contract':None,'node_contract':('ADAS_POLICY_HEADER','adas_i2c/src/ownengine/adapters/policy.h'),'slm_contract':('SLM_WORKER_HEADER','slm_spi/src/ownengine/adapters/worker.h'),'adas_board_contract':('ADAS_BOARD_HEADER','adas_i2c/src/ownengine/adapters/board_app.h'),'capture_contract':('ADAS_CAPTURE_HEADER','adas_i2c/src/ownengine/adapters/capture.h'),'actor_register_contract':('ACTOR_BOARD_HEADER','actuator_uart/src/board_app.h'),'main_contract':('MAIN_BOARD_HEADER','main/src/board_app.h')}
with tempfile.TemporaryDirectory(prefix='adas-independent-') as d:
 for name,header in headers.items():
  exe=Path(d)/name;cmd=[os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-I'+str(HERE/'fake_avr')]
  if header:cmd+=[f'-D{header[0]}="{src}/firmware/{header[1]}"']
  cmd += [str(HERE/(name+'.cpp')),'-o',str(exe)];r=run(cmd)
  if not r['exit_code']:r=run([str(exe)])
  r['name']=name;results['host_tests'].append(r);print(name,r['exit_code'],r['output'].strip())
 objects=[];bridge_results=[]
 for peer,macro,header in [(0,'ACTOR_ENGINE_HEADER','actuator_uart/src/control_engine.h'),(1,'ADAS_WORKER_HEADER','adas_i2c/src/ownengine/adapters/worker.h'),(2,'SLM_WORKER_HEADER','slm_spi/src/ownengine/adapters/worker.h')]:
  obj=Path(d)/f'peer{peer}.o';r=run([os.environ.get('CXX','c++'),'-std=c++17','-O2',f'-DPEER={peer}',f'-D{macro}="{src}/firmware/{header}"','-c',str(HERE/'peer_bridge.cpp'),'-o',str(obj)]);bridge_results.append(r);objects.append(str(obj))
 exe=Path(d)/'e2e';r=next((x for x in bridge_results if x['exit_code']),None)
 if r is None:r=run([os.environ.get('CXX','c++'),'-std=c++17','-O2','-I'+str(HERE/'fake_avr'),f'-DMAIN_BOARD_HEADER="{src}/firmware/main/src/board_app.h"',str(HERE/'e2e_contract.cpp'),*objects,'-o',str(exe)])
 if not r['exit_code']:r=run([str(exe)])
 r['name']='e2e_actual_peers';results['host_tests'].append(r);print(r['name'],r['exit_code'],r['output'].strip())
 if a.build_uno:
  cli=os.environ.get('ARDUINO_CLI') or shutil.which('arduino-cli') or '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
  for board in ('main','slm_spi','adas_i2c','actuator_uart'):
   r=run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(Path(d)/board),str(src/'firmware'/board)]);r['board']=board
   flash=re.search(r'Sketch uses (\d+) bytes',r['output']);ram=re.search(r'Global variables use (\d+) bytes',r['output'])
   if flash and ram:r.update(flash_bytes=int(flash[1]),static_sram_bytes=int(ram[1]),static_sram_spare=2048-int(ram[1]));r['limits_pass']=int(flash[1])<=32256 and int(ram[1])<=1536
   results['builds'].append(r);print(board,r['exit_code'],r.get('static_sram_spare'))
results['passed']=not status and all(r['exit_code']==0 for r in results['host_tests']) and (not a.build_uno or all(r['exit_code']==0 and r.get('limits_pass',False) for r in results['builds']))
(HERE/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(0 if results['passed'] else 1)
