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

## UART/SPI 통신 진단 업데이트 — 2026-10-13

PC의 SLM 콘솔에서 `!status`는 STATUS와 LINK를 출력하고, `!diag`는 MAIN의 상세 카운터를 출력합니다. 먼저 MAIN/SLM_UART/PERIPHERAL_SPI 세 역할을 이 수정본으로 업데이트합니다. 모터 외부 전원을 끄고 역할별 포트를 확인해 업로드한 뒤 연결을 복원하세요. ADAS의 센서/배선 규칙은 그대로입니다.

```text
!status
!diag
```

LINK uart는 MAIN이 연산 보드 응답을 확인했는지, spi는 주변 보드 메타데이터를 확인했는지, spi_safe는 STOP 동기화됐는지, adas는 센서 상태 프레임이 유효한지, clear는 정상 거리3회/freshness 조건인지, alert는 물리LOW인지입니다. uart=1/spi=0이면 UART 전체 단절로 판단하지 말고 SPI D8→D10/11/12/13와 주변 보드 역할/버전을 확인합니다. uart=0이면서 rx_errors가 증가하면 교차D2/D3/GND와 UART 수신 타이밍을 확인합니다.

DIAG cause:1 UART수신오류,2 UART2s watchdog,3 SPI응답 timeout,4 SPI peer 오류,5 SPI안전동기화미완료,6 잘못된적용ACK(예약),7 ADAS freshness,8 ALERT,9 잘못된UART명령(예약),10 SPI 세션복구입니다. peer_error:6모델/라벨불일치,7세션거부,13오래된epoch,14안전차단입니다. rx_bytes/rx_frames/rx_errors는 MAIN UART 수신, spi_bad는 CRC/전송확인 실패 수입니다. 원인은 마지막 기록이며 정상 복구 후에도 남을 수 있습니다.

수정은 송신32바이트 복사 중의 인터럽트 차단 제거(완성 후8bit head 공개), SLM RX half-bit 계산의 나눗셈 제거, SPI 세션거부 시 motorsOFF→새세션EEPROM저장→양쪽재핸드셰이크(boot당최대3회/1s간격), 느린SPI에서도 상태snapshot우선 전달입니다. SPI ERROR_REPLY의 HELLO/BAD_SESSION은 선택적으로 EEPROM의최고세션을 포함합니다. 오래된 낮은hint는 무시하며 replay 보호를 해제하지 않습니다.

기존 STOP/ADAS/epoch/READY 차단은 유지됩니다. `light` 대신 `lights on/off`를 사용하고 READY 전 일반 명령을 보내지 않습니다. 이 수정은 코드의 취약점과 진단부족을 보완한 것이며 현재 실물 실패 원인이 단 하나로 확정됐다는 뜻은 아닙니다. 새펌웨어의 LINK/DIAG 출력으로 실물을 재확인해야 합니다.

## I2C 개별 진단 — 2026-10-13

MAIN·SLM_UART 수정본을 재업로드한 뒤 SLM 콘솔에 `!i2c`를 보냅니다. 출력은 MAIN의 마지막 완료된 I2C 시도이며 자동재시도 중에도 유지됩니다. 새로운 sideband 정보는 READY/명령/epoch를 변경하지 않습니다.

```text
!i2c
I2C twsr=... error=... req=... reply=... version=... len=... ok=... fail=... meta=... flags=... since_ms=...
```

TWSR은 십진수입니다:32(0x20)은 쓰기주소 NACK,72(0x48)은 읽기주소 NACK,88(0x58)은 마지막 읽기바이트 완료입니다. error1=쓰기주소응답없음,2=데이터NACK,3=읽기주소응답없음,4=예상밖하드웨어상태,5=응답CRC/프레임/버전불일치,6=진행timeout,7=STOPtimeout,8=metadata/status정책불일치입니다. error0은드라이버성공이고 version4/reply15/len8은ADAS HELLO metadata 형태입니다. meta1은MAIN metadata검증통과입니다. version2/3이면이전펌웨어일수있습니다. since_ms가2000보다크면새로운로그를기다립니다.

코드수정: 이미완료된read STOP뒤의불필요한추가STOP을제거하고CRC결과를바로완료로게시합니다. 오류·NACK·시간초과·취소는기존유한STOP복구를유지합니다. timeout의32비트시간값은안전하게읽은service now를사용합니다. 실물실패의원인이중복STOP이라고확정하지는않습니다.

USB전원을각보드에넣는경우보드끼리5V선을묶지않고GND만공통으로연결합니다. MAIN A4→ADAS A4/A5→A5 및SPI보드의로컬LCD I2C와분리된배선을확인합니다. 현재내부풀업/약30.4kHz설정은유지합니다.
