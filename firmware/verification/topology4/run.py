#!/usr/bin/env python3
"""Read-only independent topology verification; never prepares sources or uploads."""
from pathlib import Path
import argparse, hashlib, json, os, re, shutil, subprocess, tempfile, zipfile

HERE = Path(__file__).resolve().parent
p = argparse.ArgumentParser()
p.add_argument('--source-root', type=Path, default=HERE.parents[2])
p.add_argument('--build-uno', action='store_true')
p.add_argument('--verify-packages', action='store_true')
a = p.parse_args()
src = a.source_root.resolve()
seal = json.loads((HERE / 'scenario_seal.json').read_text())
assert hashlib.sha256((HERE / 'scenarios.json').read_bytes()).hexdigest() == seal['sha256']

def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return dict(command=cmd, exit_code=r.returncode, output=r.stdout + r.stderr)

results = dict(source_root=str(src), source_commit=run(['git','-C',str(src),'rev-parse','HEAD'])['output'].strip(),
               scenario_seal=seal, hardware_verified=False, host_tests=[], builds=[], packages=[])
results['source_files_sha256'] = {
    str(f.relative_to(src)): hashlib.sha256(f.read_bytes()).hexdigest()
    for board in ('main','slm_uart','adas_i2c','peripheral_spi')
    for f in sorted((src/'firmware'/board).rglob('*')) if f.is_file()
}
headers = {
    'protocol_contract': ('PROTOCOL_HEADER', 'common/protocol.h'),
    'main_contract': ('MAIN_BOARD_HEADER', 'main/src/board_app.h'),
    'peripheral_contract': ('PERIPHERAL_ENGINE_HEADER', 'peripheral_spi/src/engine.h'),
    'peripheral_register_contract': ('PERIPHERAL_BOARD_HEADER', 'peripheral_spi/src/board_app.h'),
    'lcd_display_contract': ('PERIPHERAL_LCD_HEADER', 'peripheral_spi/src/lcd.h'),
    'slm_contract': ('SLM_SESSION_HEADER', 'slm_uart/src/session.h'),
    'adas_contract': ('ADAS_POLICY_HEADER', 'adas_i2c/src/ownengine/adapters/policy.h'),
    'adas_capture_contract': ('ADAS_CAPTURE_HEADER', 'adas_i2c/src/ownengine/adapters/capture.h'),
    'adas_board_contract': ('ADAS_BOARD_HEADER', 'adas_i2c/src/ownengine/adapters/board_app.h'),
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
        if name == 'slm_contract':
            cmd.insert(-3, f'-DSLM_PLANNER_HEADER="{src}/firmware/slm_uart/src/planner.h"')
        r = run(cmd)
        if not r['exit_code']:
            r = run([str(exe)])
        r['name'] = name
        results['host_tests'].append(r)
        print(name, r['exit_code'], r['output'].strip(), flush=True)
        if name == 'peripheral_register_contract':
            cmd.insert(-3, '-DPERIPHERAL_ACTIVE_BUZZER=0')
            cmd.insert(-3, '-DPERIPHERAL_LCD_ADDRESS=0x3f')
            r = run(cmd)
            if not r['exit_code']:
                r = run([str(exe)])
            r['name'] = name + '_passive_3f'
            results['host_tests'].append(r)
            print(r['name'], r['exit_code'], r['output'].strip(), flush=True)
    objects = []
    bridge_failures = []
    for peer in (0,1):
        obj = Path(d)/f'peer{peer}.o'
        cmd = [os.environ.get('CXX','c++'),'-std=c++17','-O2',f'-DPEER={peer}',
               f'-DPERIPHERAL_ENGINE_HEADER="{src}/firmware/peripheral_spi/src/engine.h"',
               f'-DSLM_SESSION_HEADER="{src}/firmware/slm_uart/src/session.h"',
               f'-DSLM_PLANNER_HEADER="{src}/firmware/slm_uart/src/planner.h"',
               '-c',str(HERE/'peer_bridge.cpp'),'-o',str(obj)]
        r = run(cmd)
        if r['exit_code']:
            bridge_failures.append(r)
        objects.append(str(obj))
    exe = Path(d)/'actual_peer_integration'
    r = bridge_failures[0] if bridge_failures else run([os.environ.get('CXX','c++'),'-std=c++17','-O2',
        f'-DMAIN_ENGINE_HEADER="{src}/firmware/main/src/control_engine.h"',str(HERE/'e2e_contract.cpp'),*objects,'-o',str(exe)])
    if not r['exit_code']:
        r = run([str(exe)])
    r['name'] = 'actual_peer_integration'
    results['host_tests'].append(r)
    print(r['name'],r['exit_code'],r['output'].strip(),flush=True)
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
    if a.verify_packages:
        manifest = json.loads((src/'firmware/packages/manifest.json').read_text())
        assert manifest['model_id']=='95616d56' and manifest['protocol_version']==4 and manifest['labels_version']==3
        assert set(manifest['boards'])=={'main','slm_uart','adas_i2c','peripheral_spi'}
        for board, metadata in manifest['boards'].items():
            archive = src/'firmware/packages'/metadata['archive']
            assert hashlib.sha256(archive.read_bytes()).hexdigest()==metadata['sha256']
            extracted = Path(d)/'packages'/board
            extracted.mkdir(parents=True)
            with zipfile.ZipFile(archive) as z:
                assert set(z.namelist())=={item['path'] for item in metadata['files']}
                for item in metadata['files']:
                    name = item['path']
                    assert name.startswith(board+'/') and '..' not in Path(name).parts and not Path(name).is_absolute()
                    data = z.read(name)
                    assert hashlib.sha256(data).hexdigest()==item['sha256']
                    relative = Path(name).relative_to(board)
                    if (src/'firmware'/name).is_file():
                        expected = (src/'firmware'/name).read_bytes()
                        if relative == Path('GUIDE.md'):
                            expected = expected.replace(b'(../PIN_MAP.md)',b'(PIN_MAP.md)').replace(b'(../INTEGRATION_CHECKLIST.md)',b'(INTEGRATION_CHECKLIST.md)')
                        assert data==expected, name
                    target = extracted/relative
                    target.parent.mkdir(parents=True,exist_ok=True)
                    target.write_bytes(data)
            assert (extracted/(board+'.ino')).is_file()
            for f in extracted.rglob('*'):
                if f.suffix in ('.h','.ino','.cpp'):
                    for relative in re.findall(r'^\s*#\s*include\s*"([^"]+)"',f.read_text(),re.MULTILINE):
                        target = (f.parent/relative).resolve()
                        assert target.is_relative_to(extracted.resolve()) and target.is_file(),(f,relative)
            result = dict(board=board,exit_code=0,archive_sha256=metadata['sha256'],file_count=len(metadata['files']),standalone_includes=True)
            if a.build_uno:
                result['build'] = run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(Path(d)/('package-build-'+board)),str(extracted)])
                result['exit_code'] = result['build']['exit_code']
            results['packages'].append(result)
            print('package',board,result['exit_code'],flush=True)
results['passed'] = all(r['exit_code']==0 for r in results['host_tests']) and (not a.build_uno or all(r['exit_code']==0 and r.get('limits_pass',False) for r in results['builds']))
results['passed'] = results['passed'] and (not a.verify_packages or len(results['packages'])==4 and all(r['exit_code']==0 for r in results['packages']))
results['source_files_stable'] = all((src/name).is_file() and hashlib.sha256((src/name).read_bytes()).hexdigest()==digest for name,digest in results['source_files_sha256'].items())
results['passed'] = results['passed'] and results['source_files_stable']
(HERE/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(0 if results['passed'] else 1)
