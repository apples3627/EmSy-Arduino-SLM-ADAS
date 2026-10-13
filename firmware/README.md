# 네 Uno R3: PC → 연산 UART → MAIN + ADAS + SPI 주변장치

2026-10-13 현재 정본은 **MAIN + SLM_UART + ADAS_I2C + PERIPHERAL_SPI**입니다. 프로토콜4, 모델95616d56, 라벨 버전3입니다. 네 보드를 함께 업데이트해야 하며 이전 SPI 연산/UART 작동 구조와 섞지 않습니다. I/O는 AVR 레지스터 방식입니다.

|담당 보드|역할|업로드 지침|배포 ZIP|
|---|---|---|---|
|SLM_UART|PC USB UART로 영어 입력, 전체 NN 분류·다중 절 계획, D2/D3 soft UART로 MAIN 전달|[연산 보드](slm_uart/GUIDE.md)|[slm_uart ZIP](packages/slm_uart_uno_r3.zip)|
|MAIN|UART 결과 검증, ADAS 우선 중재, L298N 두 모터 구동, SPI 주변장치 명령 및 상태 공유|[MAIN](main/GUIDE.md)|[main ZIP](packages/main_uno_r3.zip)|
|ADAS_I2C|HC-SR04 거리·위험 판단, I2C 보고, 별도 ALERT 출력|[초음파 보드](adas_i2c/GUIDE.md)|[adas_i2c ZIP](packages/adas_i2c_uno_r3.zip)|
|PERIPHERAL_SPI|MAIN SPI 요청 처리, LED·3핀 부저, 4핀 I2C LCD|[SPI 보드](peripheral_spi/GUIDE.md)|[peripheral_spi ZIP](packages/peripheral_spi_uno_r3.zip)|

ZIP 전체를 풀어 역할 폴더의 `.ino`를 엽니다. `src/`와 config를 함께 보존합니다. 외부 배선·모터 전원을 분리하고 Arduino Uno R3/ATmega328P/16MHz와 해당 포트를 선택해 업로드합니다. **PC 시리얼 모니터는 SLM_UART USB 포트에서9600 baud/Newline**으로 엽니다. MAIN USB는 전원/업로드용이며 영어 입력 콘솔이 아닙니다.

## 명령 경로

PC → SLM USART0(D0/D1 USB) → 학습된 full H64 NN·계획 검증 → SLM D3 TX → MAIN D2 RX입니다. MAIN D3 TX → SLM D2 RX로 적용 ACK와 상태를 돌려보냅니다. MAIN 하나가 software UART·I2C master·SPI master를 동시에 운용합니다.

- 전진: `go forward`, `please drive ahead`; 후진: `go backward`, `reverse the car`.
- 조명: `lights on`, `lights off`; 경적: `horn on`, `horn off`.
- 다중 명령: `go forward and lights on`, `go forward, lights on, horn on`.
- 전체 정지: `stop`, `stop everything`. 단독 `stop`은 NN/READY를 우회하는 우선 경로입니다.
- 와이퍼는 보류입니다. `wipers on/off`가 포함된 계획은 **전체 거절/STOP**, 앞부분도 실행하지 않습니다. 유효 STOP이 있는 계획은 STOP 우선입니다.
- 방향·ON/OFF 충돌, 어휘 밖 단어, 부정/조건/지원 밖 문법도 전체 거절합니다.

전체192 ASCII 바이트/4절, 절마다96바이트/16단어입니다. 구분자는 `and/also/plus`, 쉼표, 세미콜론입니다. 모든 절이 검증된 뒤에만 실행하므로 일부만 먼저 작동하지 않습니다. APPLIED/READY를 기다린 후 다음 일반 명령을 보냅니다. `!status`는 SLM에서 MAIN의 마지막 상태를 보여줍니다. 저장된 모델은 여전히10개 라벨이지만 와이퍼5/6을 운용 정책에서 차단합니다. 모델 재학습은 필요하지 않았습니다.

## 안전과 상태

HC-SR04는20cm 이하 정지,25cm 이상3회 연속 정상 측정 후 해제,70ms 주기/30ms echo timeout/200ms 초과 stale 정지입니다. ALERT는 ADAS D4 → **MAIN D6 PCINT22 + SPI 보드 D2 INT0** 공통 active LOW 선입니다. MAIN ISR이 두 모터 PWM/방향 출력을 직접 끄고, SPI 보드 ISR이 LED·부저를 직접 끕니다. MAIN의 SPI 처리 완료를 기다리지 않습니다.

해제 뒤 새 STOP epoch의 OFF ACK를 확인하며 이전 명령은 자동 재개하지 않습니다. SLM의 오래된 epoch 계획, MAIN의 오래된 SPI ACK가 주행을 되살리지 못합니다. MAIN은 요청 상태와 적용 확인 상태를 분리하고 변경 시/주기적으로 snapshot을 SLM·ADAS·SPI 보드에 공유합니다. 통신이 끊기거나 센서 상태가 미확인인 경우 주행을 허용하지 않습니다. 와이퍼 상태는 항상0입니다.

모터는 SPI 보드의 같은 session/epoch/명령 ACK를 확인한 뒤 켭니다. 서로 다른 칩의 출력은 전기적으로 원자적인 동시 변경이 아니며 통신 단계가 있습니다. 방향 반전은 먼저 두 모터를 끄고100ms 대기한 다음 부가 출력을 요청합니다. 기본 주행 lease500ms가 끝나면 **두 모터만** 끄고 LED/부저는 유지합니다. STOP/ADAS는 모든 동작 출력을 끕니다. LCD는 안전 상태를 표시하는 정보 출력이며 STOP에서 화면을 소등하지 않습니다.

## LCD와 부저

확인된 LCD 핀은 GND/VCC/SDA/SCL입니다. LCD를 MAIN의 SPI 선에 직접 연결하지 않습니다. SPI 보드가 MAIN과 SPI로 통신하고, 자기 A4/A5에서 LCD를 로컬 I2C로 구동합니다. MAIN↔ADAS I2C와 **서로 다른 버스**입니다. 기본16×2 HD44780/PCF8574, 주소0x27/표준 backpack 배선을 가정하며 주소0x3F·backpack 핀 매핑은 config에서 변경합니다. LCD의 행/칩/주소는 실제 부품에서 확인해야 합니다.

3핀 부저는 SPI 보드 D3 signal/5V/GND입니다. 능동 부저 기본 설정이고 수동 피에조면 `PERIPHERAL_ACTIVE_BUZZER=0`으로 바꿔1kHz 신호를 사용합니다. S/+/- 인쇄 순서와 모듈 정격을 확인합니다. LED는 D4에 직렬저항을 사용합니다. LCD 문제는 상태에 보고하되 SPI·ALERT 처리를 막지 않도록 timeout과 비차단 갱신을 구현했습니다.

[전체 배선표](PIN_MAP.md) · [통합 시험](INTEGRATION_CHECKLIST.md) · [독립 검증](reports/TOPOLOGY4_INDEPENDENT.md)

```sh
python3 firmware/tools/test.py
python3 firmware/tools/build.py
python3 firmware/tools/package.py
```

컴파일·호스트 모델/통신/레지스터 검증을 완료한 코드입니다. 실제 네 보드 연결·LCD/부저 모델 확인·전기 파형·모터 구동 및 실행 스택 최대값은 팀 실물 시험이 필요합니다. 자동 업로드는 하지 않습니다. build_sizes.json의 SRAM 여유는 정적 linker 값입니다.

이전 `slm_spi/actuator_uart/compute_i2c/compute_spi`와 이전 이름 ZIP은 재현용 기록입니다. 현재 manifest에는 위 네 역할만 있습니다. `0_web`은 격리된2026-10-12 이전 배치의 설명 snapshot이며 새 핀 배선 기준으로 사용하지 않습니다.

현재 PC UART 터미널은 `python3 firmware/tools/uart_cli.py <SLM_USB포트>`로 실행합니다. `!status`/`!diag` 진단은 [통신 점검](reports/UART_TROUBLESHOOT.md)을 참고합니다.
