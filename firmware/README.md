# 네 대 Uno R3: 레지스터 기반 영어 명령 자동차 제어

학습 모델 `95616d56`을 그대로 사용합니다. **MAIN 1대 + I2C 계산 1대 + SPI 계산 1대 + UART 제어 1대**입니다. MAIN은 영어 문장을 최대4개 절로 분리하고 절별 토큰을 두 계산 보드로 보내고, H32씩 계산한 10개 점수를 합한 다음 제어 보드의 출력 적용 ACK를 확인합니다.

## 팀에 전달할 파일

각 담당자는 자기 역할의 ZIP **전체**를 풀고 `.ino`를 여세요. `.ino`만 보내면 `src/`의 통신·모델 파일이 없어 컴파일되지 않습니다.

|담당 보드|스케치|담당자 지침|독립 배포 ZIP|
|---|---|---|---|
|MAIN|`main/main.ino`|[MAIN GUIDE](main/GUIDE.md)|`packages/main_uno_r3.zip`|
|COMPUTE_I2C|`compute_i2c/compute_i2c.ino`|[I2C GUIDE](compute_i2c/GUIDE.md)|`packages/compute_i2c_uno_r3.zip`|
|COMPUTE_SPI|`compute_spi/compute_spi.ino`|[SPI GUIDE](compute_spi/GUIDE.md)|`packages/compute_spi_uno_r3.zip`|
|ACTUATOR|`actuator_uart/actuator_uart.ino`|[제어 GUIDE](actuator_uart/GUIDE.md)|`packages/actuator_uart_uno_r3.zip`|

[전체 핀 연결표](PIN_MAP.md)와 [팀 통합 시험 순서](INTEGRATION_CHECKLIST.md)를 함께 읽습니다. ZIP에도 공통 지침을 동봉합니다. 보드마다 **역할 이름**을 붙이고 자기 USB 포트를 확인하세요. 팀원의 보드에 잘못된 역할을 업로드하지 않도록 합니다.

## 업로드 공통 절차

1. 모든 보드는 **Arduino Uno R3 / ATmega328P / 16 MHz**입니다. Uno R4·Mega용 코드가 아닙니다.
2. 업로드할 보드의 외부 배선과 모터·서보 전원을 분리하고 USB로 PC에 연결합니다.
3. Arduino IDE에서 해당 역할 `.ino`를 열고 `Arduino Uno`와 **그 보드의 포트**를 선택하여 검증/업로드합니다. 추가 Wire/SPI/Servo/SoftwareSerial 라이브러리는 필요 없습니다.
4. 특히 ACTUATOR의 D0/D1 UART 배선을 분리한 상태에서 업로드합니다. 운용 중에는 ACTUATOR의 PC 시리얼 모니터를 열지 않습니다.
5. 네 담당자가 업로드를 마치고 전원을 끈 뒤 [핀 연결표](PIN_MAP.md)대로 조립합니다. 처음에는 모터·서보 전원을 끄고 통신만 확인합니다.
6. MAIN USB 시리얼 모니터만 **9600 baud / Newline**으로 엽니다. `!status`, 필요하면 `!retry`를 보냅니다.
7. 모든 모델/세션 핸드셰이크와 초기 ALL_STOP ACK가 끝난 뒤 `READY model=95616d56 labels=3`가 나와야 합니다. 이후 명령을 하나씩 보내고 `APPLIED`를 기다립니다.

## PC 명령과 응답

|입력|의도|
|---|---|
|`go forward` / `please drive ahead`|전진|
|`go backward` / `reverse the car`|후진|
|`stop` / `stop everything`|주행·조명·와이퍼·경적 모두 OFF|
|`turn on the lights` / `turn off the lights`|조명 ON/OFF|
|`turn on the wipers` / `stop the wipers`|와이퍼만 ON/OFF|
|`turn on the horn` / `silence the horn`|피에조 경적만 ON/OFF|
|`do not move forward` / 지원 밖 문장|거절 및 ALL_STOP|
|`!status`|MAIN 준비/링크/작업 상태|
|`!retry`|기존 작업을 취소하고 새 세션으로 재연결|
|`!trace`|통신 스케줄 설명 출력|

입력은 전체 영어 ASCII 최대 192바이트·4개 명령절입니다. 명령절은 앞뒤 공백 제거 후 최대96바이트·16단어입니다. 어휘 밖 단어는 거절됩니다. 새 일반 문장을 연속 전송하지 말고 이전 `APPLIED`를 기다리세요. `stop`은 대기 분류를 취소하고 우선 전송합니다. UART 프레임 중간을 끊지 않고 현재 프레임을 마친 뒤 STOP을 보냅니다.

`RESULT`는 분류 결과, `APPLIED`는 제어 보드가 GPIO/PWM/서보 신호/경적 상태를 적용했다는 ACK입니다. 실제 바퀴 회전이나 서보 이동을 센서로 측정했다는 뜻은 아닙니다. `ALL_STOP_unconfirmed`는 STOP ACK를 받지 못했다는 뜻으로 통신을 확인해야 합니다.

## 레지스터 구현 범위

`.ino`의 `setup/loop` 진입점은 Arduino IDE가 제공하지만 I/O는 AVR 레지스터를 직접 사용합니다. Serial/Wire/SPI/SoftwareSerial/Servo/tone/millis/delay/digitalWrite/analogWrite API를 호출하지 않습니다.

|보드|통신/타이머|
|---|---|
|MAIN|USART0 PC, Timer1 COMPA/COMPB+INT0 D2/D3 UART, TWI master, SPI master, Timer0 COMPA 1ms|
|I2C 계산|TWI slave 0x12, 짧은 ISR 수신/송신, foreground H32 추론, Timer0 COMPA|
|SPI 계산|SPI slave+SS PCINT0, 캐시된 SPDR 응답, foreground H32 추론, Timer0 COMPA|
|제어|USART0 UART, Timer0 mode7 D5 PWM+1ms, Timer1 mode14 D9 서보, Timer2 COMPA D6 피에조|

모든 버퍼는 고정 크기이며 MCU 코드에 동적 할당은 없습니다. MAIN/ACTUATOR EEPROM 1020–1023은 세션 카운터에 예약합니다. ACTUATOR가 재부팅하면 MAIN은 더 큰 새 세션으로 다시 연결합니다. STM32 등 다른 MCU용 핀/레지스터로 옮길 때는 그대로 사용할 수 없습니다.

## 개발자 빌드·재현

저장소 루트 `LLM`에서 실행합니다. 기본 경로는 macOS Arduino IDE 내장 CLI이며 다른 환경은 `ARDUINO_CLI=/path/to/arduino-cli`로 지정합니다.

```sh
python3 firmware/tools/test.py
python3 firmware/tools/build.py
python3 firmware/tools/package.py
```

`prepare.py`는 학습 모델/프로토콜을 각 스케치 `src/`로 복사합니다. 원본은 `intent/uno_probe/src/`와 `firmware/common/protocol.h`입니다. 모델을 재학습했다면 임의로 헤더 일부만 바꾸지 말고 model ID/어휘/라벨/메타데이터를 함께 맞춘 후 전부 재검증하세요.

검증 범위와 실제 빌드 크기는 [검증 기록](reports/VERIFICATION.md)에 있습니다. 호스트 테스트는 실제 메인 레지스터 로직과 두 실제 모델 워커/제어 엔진을 연결하여 검사합니다. **네 보드의 물리 통신·전기 타이밍·자동차 구동은 아직 팀 통합 시험이 필요합니다.** 현재 연결된 한 대의 예비 모델 추론 스케치는 이 작업에서 자동으로 덮어쓰지 않았습니다.

## V4 다중 명령 업데이트 - 네 보드 모두 업데이트

모델 ID95616d56, 프로토콜 버전2, 라벨 버전3입니다. 기존 배포본과 섞지 말고 네 역할 ZIP을 모두 다시 풀어 업로드합니다. 핀 배선은 그대로입니다. MCU I/O는 계속 레지스터 방식입니다.

- `go forward and turn on the lights`: 전진과 조명을 한 묶음으로 적용.
- `lights off and horn on`: 서로 다른 부품의 OFF/ON을 따로 해석.
- `go forward, lights on, wipers on, horn on`: 최대4개 명령.
- `horn on and horn on`: 중복 제거, 경적 ON 한 번.
- `lights on and stop and horn on`: 전체 STOP만 적용.
- `go forward and reverse`, `lights on and lights off`: 충돌로 전체 거절/ALL_STOP.
- `lights on and unknown text`: 앞부분 조명도 실행하지 않고 전체 거절/ALL_STOP.

구분자는 `and`, `also`, `plus`, 쉼표, 세미콜론입니다. 각 절에 명시적인 동작/상태가 있어야 합니다. `turn on lights and horn` 같은 생략 구문, `then`의 시간 순서, `or`의 선택, 조건·부정 문장은 지원하지 않습니다.

메인은 모든 절의 두 분산 결과를 받고 전체 계획을 검증한 뒤 ACTUATE_BATCH 한 패킷을 보냅니다. 제어 보드도 목록 전체를 검증해 한 번에 적용합니다. 방향 반전의100ms 대기 중에는 부가 출력 변경도 보류하고, STOP·새 세션·watchdog가 묶음 전체를 취소합니다.

새 독립 검증: [다중 명령 검증 보고서](../intent/MULTI_VERIFICATION.md). 모델의 학습과 명령 분류, 문장 분리 문법 및 거절 정책의 역할을 구분합니다. 기존 한 대 실물 시험은 V3 기록이고 V4 실물 시험을 뜻하지 않습니다.
