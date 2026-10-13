#!/usr/bin/env python3
"""Independent production-path verification. No source preparation, upload or hardware IO."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tempfile, zipfile
HERE = Path(__file__).resolve().parent
P = argparse.ArgumentParser()
P.add_argument('--source-root', type=Path, default=HERE.parents[2])
P.add_argument('--main-root', type=Path)
P.add_argument('--peripheral-root', type=Path)
P.add_argument('--slm-root', type=Path)
P.add_argument('--build-uno', action='store_true')
P.add_argument('--verify-packages', action='store_true')
P.add_argument('--output', default='results.json')
A = P.parse_args()
SRC = A.source_root.resolve()
ROOTS = {'main': (A.main_root or SRC).resolve(), 'peripheral_spi': (A.peripheral_root or SRC).resolve(), 'slm_uart': (A.slm_root or SRC).resolve(), 'adas_i2c': SRC}
SEAL_SHA = 'f699ad195b9f51f7fe9e28f1acbf43e2c37cfcac511f597c28b4ab3b1a1fcf44'
assert hashlib.sha256((HERE/'acceptance_seal.json').read_bytes()).hexdigest() == SEAL_SHA

def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return dict(command=cmd, exit_code=r.returncode, output=r.stdout+r.stderr)
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
R = dict(source_root=str(SRC), source_roots={k:str(v) for k,v in ROOTS.items()}, source_commits={k:run(['git','-C',str(v),'rev-parse','HEAD'])['output'].strip() for k,v in ROOTS.items()}, acceptance_seal_sha256=SEAL_SHA, hardware_verified=False, host_tests=[], builds=[], packages=[])
FILES = [(root, f) for board, root in ROOTS.items() for f in sorted((root/'firmware'/board).rglob('*')) if f.is_file()]
FILES += [(SRC, f) for f in sorted((SRC/'firmware/common').rglob('*')) if f.is_file()]
R['source_files_sha256']={str(f):sha(f) for _,f in FILES}
DEFS = {
 'protocol_contract': ('PROTOCOL_HEADER', SRC/'firmware/common/protocol.h'),
 'main_strict_contract': ('MAIN_ENGINE_HEADER', ROOTS['main']/'firmware/main/src/control_engine.h'),
 'main_register_contract': ('MAIN_BOARD_HEADER', ROOTS['main']/'firmware/main/src/board_app.h'),
 'peripheral_contract': ('PERIPHERAL_ENGINE_HEADER', ROOTS['peripheral_spi']/'firmware/peripheral_spi/src/engine.h'),
 'peripheral_register_contract': ('PERIPHERAL_BOARD_HEADER', ROOTS['peripheral_spi']/'firmware/peripheral_spi/src/board_app.h'),
 'lcd_display_contract': ('PERIPHERAL_LCD_HEADER', ROOTS['peripheral_spi']/'firmware/peripheral_spi/src/lcd.h'),
 'slm_contract': ('SLM_SESSION_HEADER', ROOTS['slm_uart']/'firmware/slm_uart/src/session.h'),
 'adas_contract': ('ADAS_POLICY_HEADER', SRC/'firmware/adas_i2c/src/ownengine/adapters/policy.h'),
 'adas_capture_contract': ('ADAS_CAPTURE_HEADER', SRC/'firmware/adas_i2c/src/ownengine/adapters/capture.h'),
 'adas_board_contract': ('ADAS_BOARD_HEADER', SRC/'firmware/adas_i2c/src/ownengine/adapters/board_app.h'),
}
with tempfile.TemporaryDirectory(prefix='partial5-independent-') as d:
    D=Path(d)
    for name,(macro,header) in DEFS.items():
        test=HERE/(name+'.cpp')
        if not test.exists(): test=HERE.parent/'topology4'/(name+'.cpp')
        variants=[('',[])]
        if name=='peripheral_register_contract': variants.append(('_passive_3f',['-DPERIPHERAL_ACTIVE_BUZZER=0','-DPERIPHERAL_LCD_ADDRESS=0x3f']))
        if name=='lcd_display_contract': variants.append(('_3f',['-DPERIPHERAL_LCD_ADDRESS=0x3f']))
        for suffix,extra in variants:
            exe=D/(name+suffix)
            cmd=[os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-I'+str(HERE.parent/'topology4/fake_avr'),f'-D{macro}="{header}"',*extra]
            if name=='slm_contract': cmd.append(f'-DSLM_PLANNER_HEADER="{ROOTS["slm_uart"]}/firmware/slm_uart/src/planner.h"')
            r=run([*cmd,str(test),'-o',str(exe)])
            if not r['exit_code']: r=run([str(exe)])
            r['name']=name+suffix;R['host_tests'].append(r);print(r['name'],r['exit_code'],r['output'].strip(),flush=True)
    objs=[];failures=[]
    for peer in (0,1):
        obj=D/f'peer{peer}.o'
        cmd=[os.environ.get('CXX','c++'),'-std=c++17','-O2',f'-DPEER={peer}',f'-DPERIPHERAL_ENGINE_HEADER="{ROOTS["peripheral_spi"]}/firmware/peripheral_spi/src/engine.h"',f'-DSLM_SESSION_HEADER="{ROOTS["slm_uart"]}/firmware/slm_uart/src/session.h"',f'-DSLM_PLANNER_HEADER="{ROOTS["slm_uart"]}/firmware/slm_uart/src/planner.h"','-c',str(HERE/'peer_bridge.cpp'),'-o',str(obj)]
        r=run(cmd)
        if r['exit_code']:failures.append(r)
        objs.append(str(obj))
    exe=D/'actual_peer_integration'
    r=failures[0] if failures else run([os.environ.get('CXX','c++'),'-std=c++17','-O2',f'-DMAIN_ENGINE_HEADER="{ROOTS["main"]}/firmware/main/src/control_engine.h"',str(HERE/'e2e_contract.cpp'),*objs,'-o',str(exe)])
    if not r['exit_code']:r=run([str(exe)])
    r['name']='actual_peer_integration';R['host_tests'].append(r);print(r['name'],r['exit_code'],r['output'].strip(),flush=True)
    cli=os.environ.get('ARDUINO_CLI') or shutil.which('arduino-cli') or '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
    if A.build_uno:
        for board,root in ROOTS.items():
            r=run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(D/board),str(root/'firmware'/board)])
            r['board']=board
            flash=re.search(r'Sketch uses (\d+) bytes',r['output']);ram=re.search(r'Global variables use (\d+) bytes',r['output'])
            if flash and ram:r.update(flash_bytes=int(flash[1]),static_sram_bytes=int(ram[1]),static_sram_spare=2048-int(ram[1]),limits_pass=int(flash[1])<=32256 and int(ram[1])<=1536)
            R['builds'].append(r);print('source',board,r['exit_code'],r.get('flash_bytes'),r.get('static_sram_bytes'),flush=True)
    if A.verify_packages:
        try:
            manifest=json.loads((SRC/'firmware/packages/manifest.json').read_text())
            assert manifest['model_id']=='95616d56' and manifest['protocol_version']==5 and manifest['labels_version']==3
            assert set(manifest['boards'])==set(ROOTS)
            for board,m in manifest['boards'].items():
                archive=SRC/'firmware/packages'/m['archive'];assert sha(archive)==m['sha256'];extracted=D/'packages'/board;extracted.mkdir(parents=True)
                with zipfile.ZipFile(archive) as z:
                    assert set(z.namelist())=={item['path'] for item in m['files']}
                    for item in m['files']:
                        name=item['path'];assert name.startswith(board+'/') and '..' not in Path(name).parts and not Path(name).is_absolute()
                        data=z.read(name);assert hashlib.sha256(data).hexdigest()==item['sha256'];relative=Path(name).relative_to(board)
                        if (SRC/'firmware'/name).is_file():
                            expected=(SRC/'firmware'/name).read_bytes()
                            if relative==Path('GUIDE.md'):expected=expected.replace(b'(../PIN_MAP.md)',b'(PIN_MAP.md)').replace(b'(../INTEGRATION_CHECKLIST.md)',b'(INTEGRATION_CHECKLIST.md)')
                            assert data==expected,name
                        target=extracted/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
                assert (extracted/(board+'.ino')).is_file()
                for f in extracted.rglob('*'):
                    if f.suffix in ('.h','.ino','.cpp'):
                        for relative in re.findall(r'^\s*#\s*include\s*"([^"]+)"',f.read_text(),re.MULTILINE):
                            target=(f.parent/relative).resolve();assert target.is_relative_to(extracted.resolve()) and target.is_file(),(f,relative)
                result=dict(board=board,exit_code=0,archive_sha256=m['sha256'],file_count=len(m['files']),standalone_includes=True)
                if A.build_uno:
                    result['build']=run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(D/('package-'+board)),str(extracted)])
                    result['exit_code']=result['build']['exit_code']
                R['packages'].append(result);print('package',board,result['exit_code'],flush=True)
        except Exception as e:R['packages'].append(dict(exit_code=1,error=repr(e)));print('package FAILURE',repr(e),flush=True)
R['source_files_stable']=all(Path(f).is_file() and sha(Path(f))==digest for f,digest in R['source_files_sha256'].items())
R['passed']=all(r['exit_code']==0 for r in R['host_tests']) and (not A.build_uno or len(R['builds'])==4 and all(r['exit_code']==0 and r.get('limits_pass',False) for r in R['builds'])) and (not A.verify_packages or len(R['packages'])==4 and all(r['exit_code']==0 for r in R['packages'])) and R['source_files_stable']
(HERE/A.output).write_text(json.dumps(R,indent=2)+'\n')
raise SystemExit(0 if R['passed'] else 1)
