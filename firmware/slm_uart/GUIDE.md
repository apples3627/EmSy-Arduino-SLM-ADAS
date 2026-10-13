# SLM_UART Uno 업로드 및 콘솔

SLM_UART는 PC의 영어 문장을 USB USART0으로 받고, 변경하지 않은 `MODEL_ID=0x95616d56` 전체 H64 신경망으로 판단합니다. 최대 4개 절을 모두 검사한 뒤 MAIN에 하나의 계획을 보냅니다. 모터 출력은 MAIN, LED·혼·LCD는 MAIN에 연결된 SPI peripheral, ADAS는 MAIN의 I2C에서 담당합니다. 이 보드는 액추에이터를 직접 제어하지 않습니다.

## 배선

| SLM_UART Uno | MAIN Uno | 용도 |
|---|---|---|
| D3 TX | D2 RX | SLM → MAIN 소프트웨어 UART |
| D2 RX | D3 TX | MAIN → SLM 소프트웨어 UART |
| GND | GND | 공통 접지 |
| USB | PC | 업로드와 9600 baud 텍스트 콘솔 |

D0/D1은 USB USART0 전용입니다. D2/D3에 USB 직렬 어댑터나 모터를 함께 연결하지 않습니다. 각 Uno는 적절한 전원으로 공급하고 GND를 공유합니다.

## 빌드와 업로드

Arduino IDE에서 `firmware/slm_uart/slm_uart.ino`를 열고 **Arduino Uno**, SLM_UART 보드의 USB 포트를 선택해 업로드합니다. 기존 `prepare` 도구는 이전 역할을 준비하므로 이 스케치에 실행하지 않습니다.

CLI 빌드 예:

```sh
arduino-cli compile --fqbn arduino:avr:uno firmware/slm_uart
arduino-cli upload --fqbn arduino:avr:uno --port /dev/cu.usbmodemXXXX firmware/slm_uart
```

업로드 명령의 포트는 실제 SLM_UART USB 포트로 바꿉니다. 이 구현의 검증은 빌드만 수행하며 실제 업로드·하드웨어 동작 확인은 별도로 수행합니다.

## PC 콘솔

직렬 모니터를 **9600 baud**, 줄 끝을 **Newline**으로 설정합니다. CRLF도 허용합니다. 부팅 후 `WAIT MAIN`을 표시하며 MAIN의 HELLO와 신선한 적용 상태가 확인되면 `READY model=95616d56`을 표시합니다.

```text
move forward and turn lights on
honk horn
turn lights off and silence horn
stop
!status
```

`RESULT`는 각 절의 신경망 raw/effective 클래스와 사유 코드, `PLAN`은 검증된 계획의 클래스 ID, `APPLIED`는 MAIN의 ACK가 확인한 실제 drive/lights/horn/epoch입니다. `!status`는 마지막 MAIN 스냅샷을 표시합니다. `WAIT` 또는 `ERROR … ALL_STOP` 상태에서는 READY가 다시 표시된 뒤 명령을 입력합니다.

클래스는 forward 0, reverse 1, stop 2, lights on/off 3/4, wipers on/off 5/6, unknown 7, horn on/off 8/9입니다. **와이퍼 5/6은 현재 지원하지 않으며 포함된 전체 계획을 거부합니다.** 모델 재학습이나 클래스 삭제는 하지 않았습니다. 같은 계획에서 STOP이 나오면 앞선 거부나 충돌을 덮어쓰고 뒤의 절은 실행하지 않습니다. `stop` 한 줄은 대소문자·양끝 공백·끝의 `.!?`를 허용하며 신경망을 거치지 않습니다. 부정·조건 등의 parse guard는 신경망을 실행하지 않고 거부합니다.

입력은 ASCII 192바이트, 절마다 96바이트, 최대 4개입니다. `and`, `also`, `plus`, 쉼표, 세미콜론으로 절을 나눕니다. 서로 반대인 의도, 알 수 없는 절, 잘못된 문법은 부분 적용 없이 STOP을 요청합니다. MAIN 상태를 모르는 경우, busy/safety inhibit, 2초 이상 상태 또는 링크가 오래된 경우, 수신 FIFO overflow도 STOP을 요청합니다.

## 구현과 재현 검증

모든 I/O는 ATmega328P 레지스터를 사용합니다. Timer0 CTC는 1ms, Timer1 CTC는 208 tick/bit 및 중간 RX 위상으로 D2/D3 9600 baud를 구현합니다. `Serial`, `Wire`, `SPI`, `SoftwareSerial`, `Servo`, `millis`, `delay`, `tone`을 사용하지 않습니다.

```sh
c++ -std=c++11 -O2 -ffp-contract=off firmware/tests/topology_slm_uart_test.cpp -o /tmp/topology-slm-uart-test
/tmp/topology-slm-uart-test
python3 firmware/tests/topology_slm_uart_host_test.py
```

Python 검증은 numpy와 저장된 `intent/model_int8.npz`를 사용합니다. 전체 H64 + global bias 점수를 독립 Python scalar 구현과 33개 문장에서 비교합니다. Session 검증은 새/반복 HELLO, session/epoch 검사, busy 스냅샷 이후 ACK, 최대 2회 동일 프레임 재전송, 링크 watchdog, STOP 우선순위, 와이퍼 거부 및 부분 적용 방지를 확인합니다.
