# ADAS HC-SR04 I2C Uno R3 업로드 안내

ZIP을 풀어 `adas_i2c/adas_i2c.ino`, `config.h`, `src/`를 함께 보존합니다. Arduino IDE에서 이 `.ino`를 열고 **Arduino Uno**, ADAS용 Uno R3의 USB 포트를 선택해 업로드합니다. 이 작업에서는 물리 보드 업로드·시리얼 모니터를 실행하지 않았습니다. 이 노드는 센서 전용이며 신경망 모델이 없습니다.

| 장치 | 연결 |
|---|---|
| HC-SR04 Vcc | ADAS 5V |
| HC-SR04 GND | ADAS GND |
| HC-SR04 Trig | ADAS D7 / PD7 |
| HC-SR04 Echo | ADAS D8 / PB0 / ICP1 |
| ADAS A4 / SDA | MAIN A4 / SDA |
| ADAS A5 / SCL | MAIN A5 / SCL |
| ADAS D4 / PD4 ALERT | MAIN D4 / PCINT20 및 ACTUATOR D3 / INT1 |
| ADAS GND | MAIN GND 및 ACTUATOR GND |

HC-SR04 모듈의 실제 Vcc/Trig/Echo/GND 인쇄 순서를 확인하세요. 이 ADAS에는 로컬 정지 버튼이 없습니다. ALERT는 하나의 공통 신호선입니다. MAIN 5V에서 ALERT까지 외부 4.7kΩ 풀업 **한 개**를 연결합니다. ADAS는 PORTD4 latch를 LOW로 유지하고 DDRD4 출력으로 LOW를 주장하며 입력으로 풀어 줍니다. HIGH를 push하지 않습니다. ACTUATOR의 약한 내부 풀업은 선택적 보조입니다. 이 선은 전원 레일 점퍼가 아닙니다. 별도 USB를 사용하는 Uno 사이의 5V 레일은 연결하지 않고 GND만 공통으로 만듭니다.

SDA와 SCL에는 각각 별도의 4.7kΩ 풀업을 MAIN 5V에 연결합니다. I2C 주소는 `0x12`, MAIN clock은 100kHz입니다. 송신 cache를 읽기 전에 요청 32바이트를 쓰고 STOP으로 transaction을 끝냅니다. 응답 역시 32바이트이며 id/op/CRC로 확인합니다. 부팅 기본 cache는 유효 ADAS_STATUS_REPLY입니다.

[HC-SR04 데이터시트](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf)에 따른 최소 10µs trigger와 60ms 초과 주기, `echo µs / 58 = cm`를 사용합니다. 기본 주기는 70ms, 총 echo timeout은 30ms입니다. Timer1 prescaler 8의 0.5µs tick과 하드웨어 ICP1 capture를 사용하고 ISR은 edge timestamp/width만 저장합니다. Timer1 COMPA가 20 tick 뒤 trigger를 내립니다. 인터럽트 지연으로 실제 펄스는 10µs보다 길 수 있습니다. valid echo는 232..46,400 tick(2..400cm)입니다. 너무 짧거나 범위 밖 pulse, stuck HIGH, 누락 rising/falling, 200ms 초과 유효 sample 부재는 fault로 정지합니다. Timer0 COMPA 1ms clock을 쓰고 Arduino core overflow interrupt를 비활성화합니다. blocking pulseIn/delay는 없습니다.

부팅부터 ALERT는 LOW입니다. 25cm 이상 유효 측정이 세 번 연속 있어야 초기 정지가 풀립니다. 20cm 이하는 첫 sample부터 장애물 정지, 21..24cm는 clear count를 초기화하며 기존 정지를 해제하지 않습니다. fault 뒤에도 같은 세 번 확인을 거칩니다. 정지 해제는 차량 자동 재출발을 뜻하지 않습니다. MAIN/ACTUATOR는 새 SLM 명령이 있을 때까지 STOP을 유지합니다. 앞 센서 하나의 보수적 기본 정책으로 후진까지 모든 주행을 정지합니다.

MAIN은 현재 metadata를20/25/3/70ms로 고정 확인하므로 임계값·주기를 변경할 때 MAIN의 metadata/status 검증과 테스트도 함께 수정하고 네 역할을 재배포해야 합니다. `config.h`에서 STOP_CM=20, RELEASE_CM=25, CLEAR_REQUIRED=3, PERIOD_MS=70, TIMEOUT_MS=30, STALE_MS=200을 빌드 시 변경할 수 있습니다. ADAS_CONFIG는 예약되어 BAD_OP입니다. 정책 임계값은 `0 < STOP_CM < RELEASE_CM`, trigger 주기는 60ms 초과로 유지하세요.

센서 추가는 `SourceRegistry::registerSource(index)`로 독립 source를 활성화하고 foreground에서 `update(index, STARTUP/OBSTACLE/SENSOR_FAULT)`를 호출합니다. 현재 HC-SR04는 index 0이며 총 네 source 공간이 있습니다. 활성 source는 최초 STARTUP hazard로 시작합니다. 다른 source의 clear는 기존 hazard를 지우지 않으며 어떤 source라도 hazard면 공통 ALERT가 LOW입니다. 사용하지 않는 source는 `registerSource(index, false)`로 명시적으로 비활성화합니다. 현재 장애물 판단은 desired motion과 무관합니다. STATE_SYNC에 저장된 desired/applied snapshot은 향후 플러그인 참고용입니다.

HELLO session32는 공유 snapshot만 재설정하고 실제 센서 hazard는 지우지 않습니다. STATE_SYNC는 22바이트 session/revision 검증 후 ACK하며 출력이나 센서 상태를 바꾸지 않습니다. 이전 revision은 BAD_JOB, 다른 session은 BAD_SESSION이고 동일 revision/동일 내용 재전송은 허용합니다. STATUS_REPLY payload 18바이트 flags는 startup bit0, obstacle bit1, fault bit2, valid bit3, ALERT bit4입니다. range가 invalid면 FFFF, 유효 sample이 없으면 age FFFF입니다. shared snapshot revision은 offset 14입니다. HELLO metadata payload는 8바이트 role AD/sensor HC/20/25/3/protocol3/period16입니다.

검증은 `adas_nodes_policy_test.cpp`와 `adas_nodes_capture_test.cpp`에서 threshold, hysteresis, noise/fault, startup, stale/wrap, 여러 source, snapshot, frame cache, capture overflow race, trigger 종료, 누락 edge 취소를 확인합니다. Uno core 1.8.6 빌드: flash 3,984 / 32,256, static SRAM 291 / 2,048바이트. 실제 clone 센서의 거리 정확도·잘못된 반사·ALERT 지연과 네 보드 배선은 물리적으로 검증되지 않았습니다.
