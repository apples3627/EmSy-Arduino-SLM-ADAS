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

프로토콜은 `firmware/common/protocol.h`의 v4를 `src/protocol.h`로 복사한 독립 스케치다. HELLO는 len9(session32, model32, labelsVersion), 응답은 PERIPHERAL_META(20) len10(session32, model32, labelsVersion, capabilities=7)이다. SID는 0과 0xffffffff를 제외한 증가하는 값이며 마지막 수락 SID를 EEPROM 1020..1023에 기록한다. 같은 SID의 HELLO 재전송은 활성 상태·epoch·명령 번호를 초기화하지 않는다. 재부팅은 출력 OFF와 safetySyncRequired로 시작한다. EEPROM은 루프마다 한 바이트씩 기록하므로 새 SID 직후 전원이 끊기는 경우 기록 완료를 보장하지 않는다. MAIN의 지속 SID 할당이 함께 필요하며 EEPROM 네 바이트는 원자적 저널이 아니다.

SPI 한 트랜잭션은 SS LOW 동안 정확히 32바이트다. MAIN은 요청을 전송한 뒤 주변장치 루프가 처리할 시간을 두고 더미 32바이트를 읽어 요청 id가 일치하는 응답을 확인한다. 더미 첫 바이트는 0xa5를 피한다. 응답은 현재 읽는 동안 바뀌지 않는 이중 버퍼이며 최대 두 요청이 대기할 수 있다. 큐 오버플로는 OFF와 재동기화 요구로 처리한다. 한 번 전송한 명령의 같은 id 재전송은 내용까지 같아야 한다.

ACTUATE_BATCH는 len11+count(session32, epoch32, count8, intents[count], duration16)이고 MAIN은 최대 4개 intent의 전체 계획을 보낸다. 0/1은 표시용, 3/4는 LED, 8/9는 부저다. 정상 입력에서 2(ALL_STOP)가 있으면 알려진 라벨 0..9와 그 충돌보다 우선하여 전체 OFF로 ACK한다. 5/6(와이퍼), 7(UNKNOWN), 충돌은 STOP이 없는 전체 배치를 거부하고 OFF한다. 9보다 큰 ID는 STOP이 있어도 거부한다. duration은 파싱하지만 이 보드의 모터 lease를 만들지 않는다.

STATUS와 HEARTBEAT는 len4 SID다. ACK와 STATUS_REPLY는 len18이고 drive=0, wiper=0, servo=0이다. LED·horn은 이 보드의 적용 상태다. flags bit2는 현재 ALERT LOW, bit3는 재동기화 필요, bit4는 LCD fault다. 마지막 적용 command16과 stopSources16이 이어진다. LCD fault는 진단이며 안전한 명령의 LED·부저 적용을 막지 않는다.

ALERT의 INT0 하강 ISR은 즉시 D3/D4와 부저 타이머를 끄고 sticky 이벤트를 남긴다. LOW→HIGH 짧은 펄스 이후에도 OFF가 유지되며, **현재 ALERT가 HIGH이고 STOP_ALL len10에 엄격하게 증가한 epoch가 있어야** 재동기화를 해제한다. 같거나 오래된 STOP으로 해제하지 않는다. 오래된 ACTUATE epoch는 거부한다. MAIN과 마지막 정상 연락 이후 2000 ms가 지나면 OFF와 재동기화 요구로 전환하고 SID는 유지한다. ALERT 해제·HELLO·HEARTBEAT만으로 출력이 복구되지 않는다.

STATE_SYNC len22는 MAIN의 상태 스냅샷을 LCD용으로 저장하고 ACK len2를 반환한다. 자체 출력·안전 epoch를 바꾸지 않는다. 이전 revision, 같은 revision에 다른 내용, 현재 안전 epoch 또는 이전 스냅샷보다 오래된 안전 epoch는 거부한다. 완전히 동일한 스냅샷 재전송은 ACK한다. LCD 첫 줄은 MAIN의 적용 drive/LED/horn 상태, 두 번째 줄은 epoch와 stopSources를 5자리로 표시한다. MAIN의 appliedKnown이 false이면 drive는 `?`, 자체 안전 억제가 있으면 마지막 칸에 `!`를 표시한다. STATE_SYNC 수신 전에는 주변장치 자체 LED/horn/epoch를 표시한다.

LCD는 시작 후 최소 40 ms를 기다리고 4비트 3,3,3,2 웨이크 순서를 수행한다. 모든 데이터/일반 명령 사이 최소 1 ms, clear 이후 최소 2 ms를 둔다. 루프마다 TWI 상태 전이 하나만 진행한다. 한 전송이 5 ms 내에 끝나지 않거나 NACK이면 LCD fault를 기록하고 1000 ms 후 초기화부터 재시도한다. TWI 대기·LCD 지연 중 인터럽트를 막거나 반복 대기하지 않으므로 SPI/ALERT 처리를 계속한다. 실물 LCD 명암은 백팩 가변저항으로 조절한다.

빌드와 호스트 회귀 테스트만 수행했다. 하드웨어 업로드·모듈 전류/극성·배선·노이즈·실제 LCD 화면은 검증하지 않았다.

```sh
arduino-cli compile --warnings all --fqbn arduino:avr:uno firmware/peripheral_spi
arduino-cli compile --warnings all --fqbn arduino:avr:uno --build-property compiler.cpp.extra_flags=-DPERIPHERAL_ACTIVE_BUZZER=0 firmware/peripheral_spi
c++ -std=c++17 -Wall -Wextra -Werror firmware/tests/topology_peripheral_engine_test.cpp -o /tmp/peripheral-engine
/tmp/peripheral-engine
c++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/topology_peripheral_register_test.cpp -o /tmp/peripheral-register
/tmp/peripheral-register
c++ -std=c++17 -Wall -Wextra -Werror -DPERIPHERAL_ACTIVE_BUZZER=0 -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/topology_peripheral_register_test.cpp -o /tmp/peripheral-passive
/tmp/peripheral-passive
c++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/tests/topology_peripheral_fake_avr firmware/tests/topology_peripheral_lcd_test.cpp -o /tmp/peripheral-lcd
/tmp/peripheral-lcd
```

검증 시 활성 부저: flash 6260 bytes / static SRAM 448 bytes, 수동 부저: flash 6264 bytes / static SRAM 448 bytes. 런타임 스택 여유는 측정값이 아니다.
