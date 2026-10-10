#!/usr/bin/env python3
"""Create per-team standalone source archives with guides and file hashes."""
from pathlib import Path
import subprocess,sys,zipfile,hashlib,json,re
ROOT=Path(__file__).resolve().parents[2]
subprocess.run([sys.executable,str(ROOT/'firmware/tools/prepare.py')],check=True)
out=ROOT/'firmware/packages';out.mkdir(exist_ok=True);manifest={}
for role in ('main','slm_spi','adas_i2c','actuator_uart'):
    folder=ROOT/'firmware'/role;archive=out/(role+'_uno_r3.zip');files=[]
    assert (folder/(role+'.ino')).exists() and (folder/'GUIDE.md').exists()
    start=(f'# {role} 보드 업로드 시작\n\n'
           f'ZIP을 풀어 {role} 폴더 전체를 유지하고 {role}.ino를 Arduino IDE에서 여세요. src/를 함께 둡니다.\n\n'
           '[보드별 업로드·배선 지침](GUIDE.md) · [전체 핀 연결표](PIN_MAP.md) · [팀 통합 체크리스트](INTEGRATION_CHECKLIST.md)\n\n'
           f'Arduino Uno R3/ATmega328P/16MHz를 선택하고 자기 보드 포트를 확인합니다. 외부 배선을 분리한 상태에서 업로드하세요. '
           f'네 보드 모두의 역할별 코드가 있어야 협력 동작합니다. 이 ZIP은 {role} 담당자용입니다.\n')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
        for f in sorted(folder.rglob('*')):
            if f.is_file() and f.name!='.DS_Store':
                path=str(f.relative_to(ROOT/'firmware'));z.write(f,path)
                files.append({'path':path,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
        for name in ('PIN_MAP.md','INTEGRATION_CHECKLIST.md'):
            f=ROOT/'firmware'/name;path=role+'/'+name;z.write(f,path)
            files.append({'path':path,'sha256':hashlib.sha256(f.read_bytes()).hexdigest()})
        z.writestr(role+'/START_HERE.md',start)
        files.append({'path':role+'/START_HERE.md','sha256':hashlib.sha256(start.encode()).hexdigest()})
    manifest[role]={'archive':archive.name,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest(),'files':files}
meta=json.loads((ROOT/'intent/metadata.json').read_text())
version=int(re.search(r'VERSION=(\d+)',(ROOT/'firmware/common/protocol.h').read_text())[1])
(out/'manifest.json').write_text(json.dumps({'model_id':meta['model_id'],'protocol_version':version,'labels_version':meta['labels_version'],'boards':manifest},indent=2)+'\n')
print('Packaged 4 standalone role folders; unzip whole folder before opening .ino')
