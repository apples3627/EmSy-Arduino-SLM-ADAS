#!/usr/bin/env python3
"""Compile all four standalone Uno sketches; does not upload or open ports."""
from pathlib import Path
import os,shutil,subprocess,json,re,sys
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,str(ROOT/'firmware/tools/prepare.py')],check=True)
cli=os.environ.get('ARDUINO_CLI') or shutil.which('arduino-cli') or '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
if not Path(cli).is_file():raise SystemExit('Install Arduino IDE/CLI or set ARDUINO_CLI')
measure=[]
for name in ('main','slm_spi','adas_i2c','actuator_uart'):
    folder=ROOT/'firmware'/name;out=ROOT/'firmware/build'/name;out.mkdir(parents=True,exist_ok=True)
    result=subprocess.run([cli,'compile','--warnings','all','--fqbn','arduino:avr:uno','--build-path',str(out),str(folder)],capture_output=True,text=True)
    (out/'compile.log').write_text(result.stdout+result.stderr)
    print(name+': '+result.stdout.strip(),flush=True)
    if result.returncode:print(result.stderr);raise SystemExit(result.returncode)
    f=re.search(r'Sketch uses (\d+) bytes',result.stdout);r=re.search(r'Global variables use (\d+) bytes',result.stdout)
    if not f or not r:raise SystemExit('Unrecognized CLI size output; inspect compile.log')
    flash,ram=int(f[1]),int(r[1]);assert flash<=32256 and ram<=1536,(name,flash,ram)
    measure.append(dict(board=name,flash_bytes=flash,static_sram_bytes=ram,sram_static_spare=2048-ram,flash_limit=32256,sram_limit=2048))
reports=ROOT/'firmware/reports';reports.mkdir(exist_ok=True)
(reports/'build_sizes.json').write_text(json.dumps({'fqbn':'arduino:avr:uno','model_id':json.loads((ROOT/'intent/metadata.json').read_text())['model_id'],'hardware_verified':False,'boards':measure},indent=2)+'\n')
print('PASS all four; reserve values are static linker spare, not measured runtime stack')
