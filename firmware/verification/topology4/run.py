#!/usr/bin/env python3
"""Read-only independent topology verification; never prepares sources or uploads."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tempfile

HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument('--source-root', type=Path, required=True)
p.add_argument('--build-uno', action='store_true')
a = p.parse_args()
src = a.source_root.resolve()
seal = json.loads((HERE / 'scenario_seal.json').read_text())
assert hashlib.sha256((HERE / 'scenarios.json').read_bytes()).hexdigest() == seal['sha256']

def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return dict(command=cmd, exit_code=r.returncode, output=r.stdout + r.stderr)

results = dict(source_root=str(src), source_commit=run(['git','-C',str(src),'rev-parse','HEAD'])['output'].strip(),
               scenario_seal=seal, hardware_verified=False, host_tests=[], builds=[])
headers = {
    'protocol_contract': ('PROTOCOL_HEADER', 'common/protocol.h'),
    'main_contract': ('MAIN_BOARD_HEADER', 'main/src/board_app.h'),
    'peripheral_contract': ('PERIPHERAL_ENGINE_HEADER', 'peripheral_spi/src/control_engine.h'),
    'peripheral_register_contract': ('PERIPHERAL_BOARD_HEADER', 'peripheral_spi/src/board_app.h'),
    'slm_contract': ('SLM_WORKER_HEADER', 'slm_uart/src/ownengine/adapters/worker.h'),
    'adas_contract': ('ADAS_POLICY_HEADER', 'adas_i2c/src/ownengine/adapters/policy.h'),
}
with tempfile.TemporaryDirectory(prefix='topology4-independent-') as d:
    for name, (macro, header) in headers.items():
        test = HERE / (name + '.cpp')
        if not test.exists():
            results['host_tests'].append(dict(name=name,exit_code=1,output='Missing required verifier'))
            continue
        exe = Path(d) / name
        cmd = [os.environ.get('CXX','c++'),'-std=c++17','-O2','-Wall','-Wextra','-I'+str(HERE/'fake_avr'),
               f'-D{macro}="{src}/firmware/{header}"',str(test),'-o',str(exe)]
        r = run(cmd)
        if not r['exit_code']:
            r = run([str(exe)])
        r['name'] = name
        results['host_tests'].append(r)
        print(name, r['exit_code'], r['output'].strip(), flush=True)
    if a.build_uno:
        cli = os.environ.get('ARDUINO_CLI') or shutil.which('arduino-cli') or '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
        for board in ('main','slm_uart','adas_i2c','peripheral_spi'):
            r = run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(Path(d)/board),str(src/'firmware'/board)])
            r['board'] = board
            flash = re.search(r'Sketch uses (\d+) bytes',r['output'])
            ram = re.search(r'Global variables use (\d+) bytes',r['output'])
            if flash and ram:
                r.update(flash_bytes=int(flash[1]),static_sram_bytes=int(ram[1]),static_sram_spare=2048-int(ram[1]))
                r['limits_pass'] = int(flash[1])<=32256 and int(ram[1])<=1536
            results['builds'].append(r)
            print(board, r['exit_code'], r.get('static_sram_spare'), flush=True)
results['passed'] = all(r['exit_code']==0 for r in results['host_tests']) and (not a.build_uno or all(r['exit_code']==0 and r.get('limits_pass',False) for r in results['builds']))
(HERE/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(0 if results['passed'] else 1)
