# 네 Uno: 영어 명령, 선택 연결, 실시간 거리 LCD

현재 정본은 **MAIN + SLM_UART + ADAS_I2C + PERIPHERAL_SPI, protocol5**입니다. 모델95616d56/라벨3은 그대로입니다. 통신 버전이 바뀌었으므로 사용하는 네 보드에 각각 새 코드를 업로드하세요. 구버전4와 섞지 않습니다. I/O는 AVR 레지스터로 구현합니다.

|보드|역할|코드·업로드 안내|담당자 배포|
|---|---|---|---|
|SLM_UART|PC USB 입력, 전체 H64 신경망과 다중 명령 검증, D2/D3 UART|[SLM 지침](slm_uart/GUIDE.md)|[ZIP](packages/slm_uart_uno_r3.zip)|
|MAIN|UART 결과 수신, ADAS 중재, L298N 두 모터, SPI/I2C 동시 통신|[MAIN 지침](main/GUIDE.md)|[ZIP](packages/main_uno_r3.zip)|
|ADAS_I2C|HC-SR04 거리 측정, I2C 보고, ALERT|[ADAS 지침](adas_i2c/GUIDE.md)|[ZIP](packages/adas_i2c_uno_r3.zip)|
|PERIPHERAL_SPI|LED·부저, 로컬 I2C LCD 거리·상태 표시|[SPI 지침](peripheral_spi/GUIDE.md)|[ZIP](packages/peripheral_spi_uno_r3.zip)|

ZIP 전체를 풀고 역할 폴더의 .ino를 엽니다. src/와 config를 유지합니다. 외부 배선·모터 전원을 분리한 뒤 Uno R3/ATmega328P/16MHz와 해당 USB 포트로 업로드합니다. MAIN USB는 전원/업로드용이며 영어 입력은 SLM USB입니다.

## 부분 연결 동작

기본값 `main/src/board_config.h`의 `REQUIRE_ADAS_FOR_MOTION=false`는 사용자가 요청한 시연 모드입니다. **PC+SLM+MAIN만으로 두 모터 명령을 실행합니다.** ADAS가 미연결·센서 오류·오래된 측정이면 `adas_mode=BYPASS`이고 물리 ALERT LOW도 무시합니다. 센서 없이도 차량이 움직이는 설정입니다. ADAS를 필수로 쓰려면 이 상수를 true로 바꿔 MAIN을 재업로드합니다.

유효한 ADAS 측정이 들어오면 자동으로 ACTIVE가 됩니다. 20cm 이하에서 모든 동작 출력을 정지하고, 25cm 이상3회 정상 측정 뒤 해제합니다. 센서 주기는70ms, echo timeout30ms, freshness200ms입니다. 정상 측정에서 ALERT ISR은 모터와 LED·부저를 즉시 끕니다. ACTIVE↔BYPASS 또는 SPI 연결 변경은 실행 중 명령을 취소하고 OFF로 전환합니다. **이전 명령은 자동 재개하지 않으며 READY 뒤 새 명령을 보내야 합니다.**

SPI가 없으면 LED·부저만 건너뛰며 새 모터 명령은 가능합니다. 콘솔에 `SKIPPED auxiliary_outputs_unavailable`, `aux=unavailable`을 표시합니다. 이때 lights/horn=0은 물리 OFF 확인이 아니라 사용 불가 상태입니다. SPI가 있으면 LED·부저 포함 계획은 적용 ACK를 기다리고, LCD 오류 자체는 LED·부저 사용을 막지 않습니다. 전진·후진은 STOP 또는 안전/통신 취소까지 유지하며 기본 PWM은255/255(100%)입니다. 반전100ms 대기는 유지됩니다. UART 단절·오류 및 STOP은 항상 정지합니다.

## LCD 표시

LCD는 SPI 보드의 A4/A5에만 연결하는16×2 HD44780+PCF8574입니다. MAIN↔ADAS 버스와 분리합니다. 기본주소0x27, 필요하면 peripheral_spi/config.h에서0x3F로 변경합니다.

첫 줄 예: `DIST:123.4cm OK ` — 실제 HC-SR04 측정값을 mm에서 cm로 표시합니다. 장애물은 STP, 미연결은 N/A, 측정 오류는 ERR, 오래된 값은 STA입니다. 잘못된 거리를0cm로 꾸미지 않습니다. MAIN은 약100ms 목표로 거리 메시지를 순환 전송하며 버스 지연에 따라 실제 화면 갱신은 늦어질 수 있습니다.

둘째 줄 예: `M:F L:1 H:0 A:B ` — M은 MAIN의 마지막 확인 상태(F/R/0 또는 미확인?), L/H는 로컬 LED·경적, A는 Y=ACTIVE/B=BYPASS/?=미설정입니다. 모터 상태는 바퀴를 직접 측정한 값이 아닙니다. STOP은 LCD 정보를 지우거나 백라이트를 끄지 않습니다.

## PC 명령과 진단

```sh
cd /Users/jiseonpark/Desktop/EmSy/LLM
python3 firmware/tools/uart_cli.py /dev/cu.usbmodemXXXX
```

포트를 실제 SLM USB로 바꾸고9600/Newline을 사용합니다. READY 뒤 `go forward`, `go backward`, `lights on`, `horn on`, `go forward and lights on and horn on`, `stop`을 보냅니다. APPLIED/READY 뒤 다음 명령을 입력합니다. 최대192 ASCII바이트/4절이고 unknown·부정/조건·충돌·보류 중인 와이퍼는 계획 전체를 거절합니다. 지원 어휘 안의 영어 유사 문장은 기존 모델로 판단합니다.

`!status`의 ready/aux/adas_mode와 LINK를 확인합니다. `!diag`는 누적 오류와 마지막 원인이며 cause는 복구 후에도 남을 수 있습니다. `!i2c`에서 error0/version5는 새 버스 프레임, flags21은 startup+fault+ALERT로 정상 거리 측정이 없다는 뜻입니다. BYPASS에서는 이것만으로 READY를 막지 않습니다. TWSR32/72는 주소NACK,88은 마지막 읽기 완료입니다. 자세한 오류 번호는 MAIN 지침을 참고하세요.

[핀 연결표](PIN_MAP.md) · [실물 시험 순서](INTEGRATION_CHECKLIST.md) · [독립 검증 보고](reports/PARTIAL5_INDEPENDENT.md) · [지속 주행 수정 검증](reports/CONTINUOUS_MOTION.md)

```sh
python3 firmware/tools/test.py
python3 firmware/tools/build.py
python3 firmware/tools/package.py
python3 firmware/verification/partial5/run.py --build-uno --verify-packages
```

호스트 검증과 Uno 컴파일은 실물 동작 확인과 다릅니다. 현재 변경에서 자동 업로드·모터/센서/배선 파형 측정은 수행하지 않습니다. SRAM 여유는 정적 linker 값이며 실행 스택 최대값은 측정하지 않았습니다. 0_web 및 이전 보고서는 격리된 이전 구성 기록입니다.
