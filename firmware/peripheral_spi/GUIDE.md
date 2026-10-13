# SPI 주변장치 Uno R3

이 보드는 LED, 3핀 부저 모듈, 4핀 I2C LCD를 담당한다. 모터·와이퍼·서보 출력은 없으며 모터 명령 0/1은 MAIN 상태 표시용으로만 허용한다. LCD는 **16×2 HD44780 호환 + PCF8574 백팩**, 부저는 **5 V, GND, 신호 입력을 가진 3핀 모듈**로 가정했다. 실제 모듈의 종류·핀 순서·입력 극성은 제공되지 않았으므로 아래 설정과 실물 라벨을 확인해야 한다. 두 핀짜리 부저나 다른 LCD 컨트롤러는 이 연결에 해당하지 않는다.

| 주변장치 Uno | 연결 |
|---|---|
| D10 / SS | **MAIN D8** |
| D11 / MOSI | MAIN D11 |
| D12 / MISO | MAIN D12 |
| D13 / SCK | MAIN D13 |
| D2 / INT0 | 공통 active LOW ALERT: ADAS D4 및 MAIN D6 |
| D4 | LED 신호 (직접 LED이면 직렬 저항 사용) |
| D3 | 부저 모듈 S / Signal |
| 5V, GND | 부저 모듈 VCC / +, GND / − |
| A4 / SDA | LCD SDA |
| A5 / SCL | LCD SCL |
| 5V, GND | LCD VCC, GND |
| GND | MAIN·ADAS와 공통 접지 |

**MAIN D10은 모터 PWM이므로 SS에 연결하지 않는다.** LCD의 A4/A5 버스는 주변장치 보드에만 연결한다. MAIN↔ADAS의 I2C 버스와 별도 버스다. LCD 백팩의 풀업 유무를 확인하고 SDA/SCL을 5 V로 풀업한다. ALERT는 ADAS가 LOW를 만들고 해제 시 고임피던스로 놓는 공통 신호이며 주변장치 D2의 내부 풀업을 사용한다.

`config.h`의 기본 LCD 주소는 7비트 `0x27`이다. 실물이 `0x3f`이면 `PERIPHERAL_LCD_ADDRESS`를 변경한다. PCF8574 기본 매핑은 RS=P0, RW=P1, EN=P2, BL=P3, D4..D7=P4..P7이다. 다른 백팩은 각각의 `PERIPHERAL_LCD_*_BIT`를 변경한다. RW는 항상 LOW로 전송한다. 백라이트는 켜진 상태로 유지한다. 주소 변경은 다른 LCD 컨트롤러를 지원한다는 의미가 아니다.

종류를 모르는 부저의 기본값은 `PERIPHERAL_ACTIVE_BUZZER=1`이다. 활성 모듈은 신호 HIGH/LOW로 켜고 끈다. 수동 피에조 모듈이면 `0`으로 바꾼다. 수동 설정은 Timer2 CTC, 분주64, OCR2A=124의 ISR에서 D3을 토글해 1 kHz를 만든다. Timer0은 CTC, 분주64, OCR0A=249의 1 ms 시간 기준이다. `Wire`, `SPI`, `Serial`, `digitalWrite`, `delay`, `tone` 등의 Arduino I/O API를 사용하지 않는다.

프로토콜은 `firmware/common/protocol.h`의 v5를 `src/protocol.h`로 복사한 독립 스케치다. HELLO는 len9(session32, model32, labelsVersion), 응답은 PERIPHERAL_META(20) len10(session32, model32, labelsVersion, capabilities=31(LED1|HORN2|LCD4|SAFETY_POLICY8|SENSOR_TELEMETRY16))이다. SID는 0과 0xffffffff를 제외한 증가하는 값이며 마지막 수락 SID를 EEPROM 1020..1023에 기록한다. 같은 SID의 HELLO 재전송은 활성 상태·epoch·명령 번호를 초기화하지 않는다. 재부팅은 출력 OFF와 safetySyncRequired로 시작한다. EEPROM은 루프마다 한 바이트씩 기록하므로 새 SID 직후 전원이 끊기는 경우 기록 완료를 보장하지 않는다. MAIN의 지속 SID 할당이 함께 필요하며 EEPROM 네 바이트는 원자적 저널이 아니다.

SPI 한 트랜잭션은 SS LOW 동안 정확히 32바이트다. MAIN은 요청을 전송한 뒤 주변장치 루프가 처리할 시간을 두고 더미 32바이트를 읽어 요청 id가 일치하는 응답을 확인한다. 더미 첫 바이트는 0xa5를 피한다. 응답은 현재 읽는 동안 바뀌지 않는 이중 버퍼이며 최대 두 요청이 대기할 수 있다. 큐 오버플로는 OFF와 재동기화 요구로 처리한다. 한 번 전송한 명령의 같은 id 재전송은 내용까지 같아야 한다.

ACTUATE_BATCH는 len11+count(session32, epoch32, count8, intents[count], duration16)이고 MAIN은 최대 4개 intent의 전체 계획을 보낸다. 0/1은 표시용, 3/4는 LED, 8/9는 부저다. 정상 입력에서 2(ALL_STOP)가 있으면 알려진 라벨 0..9와 그 충돌보다 우선하여 전체 OFF로 ACK한다. 5/6(와이퍼), 7(UNKNOWN), 충돌은 STOP이 없는 전체 배치를 거부하고 OFF한다. 9보다 큰 ID는 STOP이 있어도 거부한다. duration은 파싱하지만 이 보드의 모터 lease를 만들지 않는다.

STATUS와 HEARTBEAT는 len4 SID다. ACK와 STATUS_REPLY는 len18이고 drive=0, wiper=0, servo=0이다. LED·horn은 이 보드의 적용 상태다. flags bit2는 유효한 ADAS 차단(정책 미설정 또는 ACTIVE에서 ALERT LOW), bit3는 재동기화 필요, bit4는 LCD fault, bit5는 BYPASS 진단이다. BYPASS에서는 물리 ALERT LOW여도 bit2를 세우지 않는다. 마지막 적용 command16과 stopSources16이 이어진다. LCD fault는 진단이며 안전한 명령의 LED·부저 적용을 막지 않는다.

SAFETY_POLICY(23) len9는 SID32, observedEpoch32, mode8이다. mode0은 ACTIVE(REQUIRE_ALERT), mode1은 BYPASS_ADAS다. 부팅·새 SID의 HELLO 이후 정책은 미설정이며 출력 OFF와 safetySyncRequired를 유지한다. MAIN은 현재 epoch 이상인 정책을 보내고 ACK len2를 받은 뒤, 엄격하게 새 STOP_ALL epoch로 동기화한다. 정책 자체는 epoch를 변경하거나 출력을 복구하지 않는다. 모드 전환에는 현재 epoch 및 이전 정책 epoch보다 큰 epoch가 필요하다. 동일 epoch/모드 재전송은 ACK하지만 오래된 정책이나 같은 epoch의 다른 모드는 거부한다. STOP epoch는 저장된 정책 epoch 이상이어야 한다. 정책 전환은 항상 LED·horn을 OFF하고 새 명령을 요구한다.

ACTIVE와 정책 미설정 상태에서 ALERT의 INT0 하강 ISR은 즉시 D3/D4와 부저 타이머를 끄고 sticky 이벤트를 남긴다. LOW→HIGH 짧은 펄스 이후에도 OFF가 유지되며, **현재 ALERT가 HIGH이고 STOP_ALL len10의 epoch가 엄격하게 증가해야** 재동기화를 해제한다. ISR generation 확인은 STOP 처리 중 새 펄스가 생기는 경우에도 차단을 유지한다. BYPASS 정책은 센서 부재·고장 데모를 위한 MAIN의 명시적 결정이며 물리 ALERT LOW를 무시한다. BYPASS에서도 새 STOP 동기화 전에는 OFF이고, 동기화 뒤에도 새 명령을 받기 전에는 OFF다. LOW 상태에서 BYPASS의 새 STOP은 이전 sticky latch를 해제할 수 있다. ACTIVE로 돌아가면 즉시 OFF와 새 STOP 요구가 적용된다.

같거나 오래된 STOP으로 재동기화를 해제하지 않는다. 오래된 ACTUATE epoch는 거부한다. MAIN과 마지막 정상 연락 이후 2000 ms가 지나면 OFF와 재동기화 요구로 전환하고 SID·정책은 유지한다. ALERT 해제·HELLO·HEARTBEAT·정책 재전송·텔레메트리만으로 출력이 복구되지 않는다.

STATE_SYNC len22는 MAIN의 상태 스냅샷을 LCD용으로 저장하고 ACK len2를 반환한다. 자체 출력·안전 epoch를 바꾸지 않는다. 이전 revision, 같은 revision에 다른 내용, 현재 안전 epoch 또는 이전 스냅샷보다 오래된 안전 epoch는 거부한다. 완전히 동일한 스냅샷 재전송은 ACK한다.

SENSOR_TELEMETRY(24) len16은 SID32, epoch32, sampleId16, rangeMm16, ageMs16, flags8, mode8이다. MAIN은 약 100 ms마다 ADAS의 측정값을 전달한다. flags는 bit0 센서 메타데이터 존재, bit1 fresh 유효 거리, bit2 고장, bit3 stale, bit4 장애물, bit5 startup이다. SID가 일치하고 현재 epoch이며 sampleId가 이전 값보다 오래되지 않은 경우만 저장하고 ACK len2를 반환한다. sampleId는 16비트 wrap을 허용한다. 같은 sample 재전송은 age를 이전 age+로컬 경과 시간보다 작게 만들 수 없다. 거리 텔레메트리는 관찰 전용이며 출력·정책·안전 epoch·동기화를 변경하지 않는다. 다른 SID, 오래된 epoch/sample, 잘못된 길이·필드는 기존 출력 상태를 바꾸지 않고 거부한다.

LCD 첫 줄은 실제 rangeMm를 cm 소수 한 자리로 표시한다. 예: 1234 mm → `DIST:123.4cm OK `, 100 mm → `DIST: 10.0cm OK `. 정상 fresh 장애물은 실제 거리와 `STP`, 센서 고장·startup은 `ERR`, 센서가 없으면 `DIST:---.-cm N/A`다. rangeMm=0xffff 등 유효하지 않은 값은 0으로 꾸미지 않고 `---.-`를 표시한다. MAIN age+로컬 수신 후 경과가 200 ms를 넘거나 이전 epoch 값이면 `STA`다. 유효한 마지막 거리가 남아 있으면 stale에서도 그 거리와 STA를 표시한다.

두 번째 줄은 `M:F L:1 H:0 A:B ` 형식이다. M은 MAIN의 appliedKnown=true이고 snapshot epoch가 자체 epoch와 같으며 자체 안전 억제가 없을 때만 F/R/0, 나머지는 `?`다. 이는 MAIN의 마지막 적용 ACK 스냅샷이며 바퀴나 모터를 직접 측정한 값이 아니다. L/H는 주변 보드 실제 논리 출력 상태, A:B는 BYPASS, A:Y는 ACTIVE, A:?는 정책 미설정이다. 텔레메트리의 mode는 관찰 데이터이며 A 표시는 실제 수락한 SAFETY_POLICY를 따른다. 장애물 STOP 이후 MAIN의 적용 drive=0 스냅샷을 받아야 M:0을 표시하며, 수신 전이나 로컬 차단 중에는 M:?를 표시한다.

LCD는 시작 후 최소 40 ms를 기다리고 4비트 3,3,3,2 웨이크 순서를 수행한다. 모든 데이터/일반 명령 사이 최소 1 ms, clear 이후 최소 2 ms를 둔다. 루프마다 TWI 상태 전이 하나만 진행한다. 한 전송이 5 ms 내에 끝나지 않거나 NACK이면 LCD fault를 기록하고 1000 ms 후 초기화부터 재시도한다. TWI 대기·LCD 지연 중 인터럽트를 막거나 반복 대기하지 않으므로 SPI/ALERT 처리를 계속한다. 실물 LCD 명암은 백팩 가변저항으로 조절한다.

빌드와 호스트 회귀 테스트만 수행했다. 하드웨어 업로드·모듈 전류/극성·배선·노이즈·실제 LCD 화면은 검증하지 않았다.

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:uno firmware/peripheral_spi
arduino-cli compile --warnings all --fqbn arduino:avr:uno --build-property compiler.cpp.extra_flags=-DPERIPHERAL_ACTIVE_BUZZER=0 firmware/peripheral_spi
c++ -std=c++17 -Wall -Wextra -Werror firmware/tests/partial_peripheral_engine_test.cpp -o /tmp/peripheral-engine
/tmp/peripheral-engine
c++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/partial_peripheral_register_test.cpp -o /tmp/peripheral-register
/tmp/peripheral-register
c++ -std=c++17 -Wall -Wextra -Werror -DPERIPHERAL_ACTIVE_BUZZER=0 -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/partial_peripheral_register_test.cpp -o /tmp/peripheral-passive
/tmp/peripheral-passive
c++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/partial_peripheral_lcd_test.cpp -o /tmp/peripheral-lcd
/tmp/peripheral-lcd
```

검증 시 활성 부저: flash 7498 bytes / static SRAM 493 bytes, 수동 부저: flash 7502 bytes / static SRAM 493 bytes. SRAM 1536 bytes 이하 목표를 만족한다. 런타임 스택 여유는 측정값이 아니다.

## UART/SPI 통신 진단 업데이트 — 2026-10-13

PC의 SLM 콘솔에서 `!status`는 STATUS와 LINK를 출력하고, `!diag`는 MAIN의 상세 카운터를 출력합니다. 먼저 MAIN/SLM_UART/PERIPHERAL_SPI/ADAS 네 역할을 이 수정본으로 업데이트합니다. 모터 외부 전원을 끄고 역할별 포트를 확인해 업로드한 뒤 연결을 복원하세요. 네 역할의 프로토콜5 펌웨어를 함께 사용합니다. ADAS의 센서/배선 규칙은 그대로입니다. 센서가 연결되고 정상 fresh이면 기존 ADAS 정지 우선 동작을 사용하며, 부재·고장에는 MAIN이 BYPASS 정책을 명시적으로 전달합니다. LCD는 센서 부재·고장을 거리 0으로 표시하지 않습니다.

```text
!status
!diag
```

LINK uart는 MAIN이 연산 보드 응답을 확인했는지, spi는 주변 보드 메타데이터를 확인했는지, spi_safe는 STOP 동기화됐는지, adas는 센서 상태 프레임이 유효한지, clear는 정상 거리3회/freshness 조건인지, alert는 물리LOW인지입니다. uart=1/spi=0이면 UART 전체 단절로 판단하지 말고 SPI D8→D10/11/12/13와 주변 보드 역할/버전을 확인합니다. uart=0이면서 rx_errors가 증가하면 교차D2/D3/GND와 UART 수신 타이밍을 확인합니다.

DIAG cause:1 UART수신오류,2 UART2s watchdog,3 SPI응답 timeout,4 SPI peer 오류,5 SPI안전동기화미완료,6 잘못된적용ACK(예약),7 ADAS freshness,8 ALERT,9 잘못된UART명령(예약),10 SPI 세션복구입니다. peer_error:6모델/라벨불일치,7세션거부,13오래된epoch,14안전차단입니다. rx_bytes/rx_frames/rx_errors는 MAIN UART 수신, spi_bad는 CRC/전송확인 실패 수입니다. 원인은 마지막 기록이며 정상 복구 후에도 남을 수 있습니다.

수정은 송신32바이트 복사 중의 인터럽트 차단 제거(완성 후8bit head 공개), SLM RX half-bit 계산의 나눗셈 제거, SPI 세션거부 시 motorsOFF→새세션EEPROM저장→양쪽재핸드셰이크(boot당최대3회/1s간격), 느린SPI에서도 상태snapshot우선 전달입니다. SPI ERROR_REPLY의 HELLO/BAD_SESSION은 선택적으로 EEPROM의최고세션을 포함합니다. 오래된 낮은hint는 무시하며 replay 보호를 해제하지 않습니다.

STOP/epoch/UART 시작 동기화 차단을 유지하며, ADAS 차단은 수락한 ACTIVE/BYPASS 정책을 따릅니다. SPI 주변 보드가 없으면 MAIN은 모터 명령을 처리하고 LED·horn 적용 상태를 unavailable로 보고합니다. `light` 대신 `lights on/off`를 사용하고 READY 전 일반 명령을 보내지 않습니다. 이 수정은 코드의 취약점과 진단부족을 보완한 것이며 현재 실물 실패 원인이 단 하나로 확정됐다는 뜻은 아닙니다. 새펌웨어의 LINK/DIAG 출력으로 실물을 재확인해야 합니다.
