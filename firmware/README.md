# 영어 SLM + ADAS 자동차 제어: 네 대 Uno R3

현재 구성은 **MAIN + SLM_SPI + ADAS_I2C + ACTUATOR_UART**입니다. MAIN 하나가 UART·SPI·I2C를 동시에 운용합니다. 프로토콜3, 모델 `95616d56`, 라벨 버전3을 네 역할에서 맞춰 사용합니다.

|보드|역할|담당자 지침|팀 배포 ZIP|
|---|---|---|---|
|MAIN|문장 분리·명령 계획·ADAS 우선 중재·상태 공유|[MAIN](main/GUIDE.md)|[main ZIP](packages/main_uno_r3.zip)|
|SLM_SPI|학습된 전체 H64 모델 추론|[SLM](slm_spi/GUIDE.md)|[slm_spi ZIP](packages/slm_spi_uno_r3.zip)|
|ADAS_I2C|HC-SR04 측정·정지 판단·인터럽트 신호|[ADAS](adas_i2c/GUIDE.md)|[adas_i2c ZIP](packages/adas_i2c_uno_r3.zip)|
|ACTUATOR_UART|모터·서보·LED·피에조 출력과 독립 정지|[작동 보드](actuator_uart/GUIDE.md)|[actuator ZIP](packages/actuator_uart_uno_r3.zip)|

ZIP 전체를 풀어 역할 이름과 같은 폴더의 `.ino`를 엽니다. `src/`와 `config.h`도 함께 유지합니다. Arduino IDE에서 Arduino Uno와 해당 USB 포트를 선택하여 검증·업로드합니다. 외부 배선과 구동 전원을 분리하고, ACTUATOR D0/D1도 분리한 상태에서 업로드합니다. 다른 팀원에게 해당 역할 ZIP 전체를 전달하세요.

[전체 핀 연결표](PIN_MAP.md)와 [통합 시험 지침](INTEGRATION_CHECKLIST.md)을 따른 뒤 MAIN USB 콘솔만 9600 baud/Newline으로 엽니다. `!status`로 준비와 상태를 확인합니다. 초기 센서 정상 확인 및 출력 OFF ACK가 끝나 READY가 된 뒤 입력합니다. 모터 드라이버는 아직 미정이며 두 선 모터를 Uno 핀에 직접 연결하지 않습니다.

## 명령과 모델

모델은 SLM_SPI의 `src/model/`에 있고 `src/ownengine/adapters/worker.h`가 추론합니다. MAIN에도 토큰화·계획 검증용 모델 자산이 있으나 신경망 점수는 계산 보드에 요청합니다. ADAS에는 모델이 없습니다. 모델은 V245/E16/H64/10 출력의 작은 학습 명령 분류기입니다. 일반 문장 생성 모델은 아닙니다.

- `please drive ahead`, `go backward`: 전진/후진.
- `lights on`, `lights off`, `wipers on`, `wipers off`, `horn on`, `horn off`: 각 출력.
- `go forward and turn on the lights`: 서로 다른 동작을 한 묶음으로 적용.
- `go forward, lights on, wipers on, horn on`: 최대4개 명시적 명령절.
- `stop`, `lights on and stop and horn on`: 모든 출력 OFF. STOP이 우선합니다.
- 충돌하는 방향·ON/OFF, 알 수 없는 단어·문장, 지원 밖 문법은 전체 거절/ALL_STOP합니다.

영어 ASCII 전체192바이트·최대4절, 절마다96바이트·16단어입니다. 구분자는 `and`, `also`, `plus`, 쉼표, 세미콜론입니다. `turn on lights and horn`처럼 동사를 생략한 절, 조건·부정·순차 실행은 지원하지 않습니다. 유사 문장 지원은 학습 어휘와 검증된 표현 범위입니다. 이전 APPLIED를 기다린 뒤 다음 일반 명령을 보내세요.

## 우선순위와 상태

ADAS는 부팅·센서 오류·장애물에서 ALERT를 LOW로 만듭니다. MAIN D4의 PCINT는 추론과 대기 명령을 취소합니다. 동시에 ACTUATOR D3의 INT1이 출력 레지스터를 직접 OFF로 차단하므로 MAIN의 UART 완료를 기다리지 않습니다. 짧은 LOW 펄스도 이벤트로 보존합니다.

HC-SR04 기본은 20cm 이하 정지, 25cm 이상 세 번 연속 정상 측정 후 해제, 70ms 측정 주기, 30ms echo timeout, 200ms 초과 오래된 측정 정지입니다. 해제 뒤에도 이전 동작은 재개하지 않습니다. 새 STOP epoch의 OFF 응답을 확인한 뒤 새 SLM 명령이 필요합니다. 앞 센서 하나를 쓰는 기본 구현은 후진과 부가 출력도 전체 정지합니다.

MAIN은 원하는 상태와 ACTUATOR 응답으로 확인한 출력 상태를 분리합니다. 세션·안전 epoch가 일치하는 ACK/STATUS만 실제 상태 확인으로 인정합니다. 지연 명령·지연 ACK는 이전 동작을 되살리지 못합니다. SLM STOP도 원하는 상태를 전부0으로 변경하고 OFF 응답을 받은 뒤 확인 상태0을 SLM/ADAS에 STATE_SYNC로 공유합니다. 변경 시 및 주기적으로500ms마다 snapshot을 전달합니다. 이것은 네 칩의 순간적인 원자적 공유 메모리가 아니라 통신을 통한 상태 동기화이며 미확인 상태는 known=0으로 표시합니다.

모터 기본 lease는500ms입니다. 만료되면 주행 상태만0으로 맞추고 조명·와이퍼·경적은 유지합니다. ADAS·STOP은 모든 출력을 끕니다. SourceRegistry는 여러 센서의 위험을 OR하여 합칠 수 있고 새 센서마다 독립 freshness와 fault 정책을 추가해야 합니다.

## 레지스터 구현과 재현

Uno R3/ATmega328P/16MHz 전용입니다. Arduino setup/loop를 사용하며 모든 통신·시간·출력·인터럽트는 AVR 레지스터로 구현합니다. Serial/Wire/SPI/SoftwareSerial/Servo/tone/millis/delay/digitalWrite API는 호출하지 않습니다. MAIN은 USART0 PC, Timer1+INT0 software UART, SPI master, TWI master, Timer0 tick, PCINT20을 함께 운용합니다. ADAS는 Timer1 ICP1으로 echo를 측정합니다.

```sh
python3 firmware/tools/test.py
python3 firmware/tools/build.py
python3 firmware/tools/package.py
```

[독립 검증 보고서](reports/ADAS_INDEPENDENT.md), [현재 빌드 크기](reports/build_sizes.json)를 참고하세요. 호스트 검증과 네 스케치 컴파일을 완료했습니다. 네 실물 보드 배선·파형·모터 동작 및 최대 실행 스택은 아직 측정하지 않았습니다. 기존 연결 보드 펌웨어를 자동 업로드하지 않았습니다. MAIN의 정적 SRAM 여유589바이트는 스택 계측 결과가 아닙니다.

`compute_i2c/`, `compute_spi/`와 이름이 같은 이전 ZIP은 V4 H32 분산 계산 재현용 기록입니다. 현재 팀 배포 manifest에는 포함되지 않습니다. 변경 전 전체 상태는 `pre-adas-2026-10-10` 태그에 보관되어 있습니다.
