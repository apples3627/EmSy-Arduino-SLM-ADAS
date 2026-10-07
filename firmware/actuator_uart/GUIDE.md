# ACTUATOR 팀 업로드·배선 안내 (Uno R3 / ATmega328P 16 MHz)

이 보드는 MAIN이 전달한 허용 intent만 실행합니다. 모델 추론은 하지 않습니다. Arduino IDE에서 `actuator_uart.ino`를 열고 보드 **Arduino Uno**, 본인 ACTUATOR USB 포트를 선택하여 업로드합니다. 각 팀은 자기 Uno에 자기 스케치를 올립니다. MAIN의 D2/D3과 이 보드의 D2/D3은 서로 다른 핀입니다.

**업로드 전 ACTUATOR D0/D1 외부 UART 선을 분리하세요.** 업로드 후 USB 연결을 유지할 수 있으나 MAIN과 연결된 동안 PC Serial Monitor를 열지 마세요. UART는 9600 bps, 8N1 바이너리 전용입니다. 출력 banner/ASCII는 없습니다. USB 자동 리셋이 발생하면 MAIN은 EEPROM session을 증가시켜 다시 HELLO 해야 합니다.

## 배선표

| ACTUATOR 핀 | 방향 / 연결 |
|---|---|
| D0 / PD0 RX | 입력 ← MAIN D3 TX (소프트웨어 UART) |
| D1 / PD1 TX | 출력 → MAIN D2 RX |
| GND | MAIN GND 및 각 외부 전원/driver GND 공통 |
| D4 / PD4 | 출력 → 외부 H-bridge IN1 |
| D7 / PD7 | 출력 → 외부 H-bridge IN2 |
| D5 / PD5 OC0B | PWM 출력 → bridge PWM/ENA |
| D12 / PB4 | 기본 출력 → TB6612 STBY, boot/STOP LOW |
| D8 / PB0 | 출력 → 220–330 Ω 저항 → 소형 LED → GND |
| D9 / PB1 OC1A | 서보 signal 출력, 서보 전원은 별도 안정화 공급 |
| D6 / PD6 | 수동 piezo signal 출력, 다른 단자 GND |
| D2 / PD2 | 입력 pull-up, 로컬 비상정지 버튼 반대 단자 GND |

버튼 미설치 시 D2를 비워두면 내부 pull-up으로 미눌림입니다. 이 D2를 MAIN D2에 연결하지 마세요. 다른 MAIN/compute 배선은 그 보드 안내를 따릅니다.

## 모터·전원

두 가닥 +/− DC 모터는 외부 **양방향 H-bridge**가 필수입니다. 아직 driver를 선택하지 않았다면 실제 모터 연결을 보류하고 필요한 모터 전압, stall 전류와 driver 허용 전류를 먼저 확인하세요. 모터 두 선은 driver OUT1/OUT2에 연결합니다. GPIO 두 개로 모터에 직접 전압을 걸거나 역전하지 마세요. Uno 5V를 모터 전원으로 사용하지 마세요. 여러 보드의 USB 5V를 서로 연결하지 마세요. 공통 GND만 연결하고 외부 모터 PSU를 driver 모터 전원에 연결합니다. 하드웨어마다 보호 다이오드, bypass capacitor, 전류 제한 등은 driver 자료를 따라야 합니다.

기본 TB6612 설정은 `MOTOR_HAS_STBY=1`, D12→STBY입니다. 사용하지 않는 두 번째 channel 입력은 driver 자료에 따라 안전 상태로 묶습니다. L298 모듈은 `MOTOR_HAS_STBY=0`으로 변경하고 D12는 NC, D5→ENA, D4→IN1, D7→IN2입니다. ENA jumper를 제거해야 PWM 입력이 적용되는 모듈이 있습니다. direction은 FORWARD IN1 HIGH/IN2 LOW, REVERSE LOW/HIGH, neutral LOW/LOW입니다. 실제 forward 방향은 `Config::invertDirection` 또는 모터 선 방향으로 정합니다. 기본 PWM은 75/249 (약 30%)이며 실제 속도나 회전 방향은 ACK만으로 검증되지 않습니다.

서보 D9는 signal만 연결합니다. 별도 regulated 전원은 서보 정격에 맞추고 GND를 공통으로 합니다. 초기 sweep 45°–135°, 3°/25 ms이며 기계 한계에 맞춰 `board_config.h`의 `Config` 기본값을 조정하세요. ON 동안만 20 ms 주기 1–2 ms pulse를 출력합니다. OFF/STOP 시 즉시 signal LOW, parking 없음입니다. 서보 전원을 끊는 기능은 없으며 signal 중단 시 실제 토크/위치는 서보 종류에 따릅니다.

D6 기본은 소전류 **수동 piezo**용 1 kHz square wave입니다. 정격 전류를 확인하고 전류가 큰 buzzer에는 transistor/driver를 사용하세요. 능동 buzzer는 `ACTIVE_BUZZER=1`로 바꾸면 단순 HIGH/LOW입니다. lamp도 소형 LED 외에는 별도 driver가 필요합니다. D8에 전력 lamp를 직접 연결하지 마세요.

## 동작·안전 계약

| 코드 / intent | 동작 |
|---|---|
| 0 FORWARD / 1 REVERSE | 모터만 유한 시간 회전 (기본 500 ms, 최대 1000 ms) |
| 2 ALL_STOP / 7 UNKNOWN | 모터·LED·서보 signal·horn 모두 OFF |
| 3 LIGHT_ON / 4 LIGHT_OFF | LED만 변경 |
| 5 WIPER_ON / 6 WIPER_OFF | 연속 sweep 시작 / signal 즉시 OFF |
| 8 HORN_ON / 9 HORN_OFF | horn latch ON / OFF |

일반 intent는 다른 부품의 상태를 보존합니다. 모터 lease 만료는 모터만 neutral입니다. 반대 방향 요청은 PWM=0/neutral 후 100 ms 기다려 적용하고 ACK합니다. duration은 적용 시점부터 시작합니다. 같은 id retry는 lease를 다시 시작하지 않으며 현재 실제 명령 상태를 ACK합니다. 대기 중 같은 id 또는 독립 action은 BUSY; MAIN은 ACK 후 재시도합니다. 더 새 STOP/UNKNOWN은 대기 동작을 취소하며 늦은 ACK/복원이 없습니다. 오래된 command id는 OLD_COMMAND입니다.

2초간 유효 통신이 없으면 전부 OFF 및 session 무효화됩니다. STATUS/HEARTBEAT는 통신 생존만 갱신하며 모터 lease를 연장하지 않습니다. 버튼 LOW가 10 ms 지속되면 전부 OFF와 stop latch. 버튼을 놓은 후 **새 session HELLO**와 새 ACTUATE가 필요하며 예전 동작은 재개되지 않습니다. 이 소프트웨어 버튼은 인증된 하드웨어 비상정지 장치가 아닙니다.

완전한 32-byte 프레임에서 CRC/padding/version 오류, RX ring overflow 또는 USART 오류는 global OFF 및 session 무효화입니다. leading noise byte만으로는 OFF 하지 않습니다. 알 수 없는 op/intent와 잘못된 길이는 global OFF; 잘못된 session/오래된 command는 실행하지 않습니다. 이는 암호 인증이 아닌 우발 재전송/재생 방어입니다.

EEPROM **1020–1023**은 마지막 새 session uint32 little-endian 저장에 예약됩니다. 새 HELLO마다만 기록하고 ACK 전에 쓰기 완료를 기다립니다. session 0과 0xffffffff는 금지, wrap 금지입니다. 부팅 후 같은 EEPROM session은 거절되므로 MAIN은 더 큰 session을 사용해야 합니다. 실행 중 같은 활성 HELLO retry는 출력/cache를 리셋하지 않습니다. EEPROM 삭제 후에는 두 보드의 session 관리도 다시 맞춰야 합니다.

## 레지스터 설정과 확인 순서

Arduino core의 startup을 사용하는 스케치지만 모든 I/O와 시간 제어는 AVR register 코드입니다. Serial/Wire/SPI/SoftwareSerial/Servo/tone/millis/delay/digitalWrite/analogWrite는 사용하지 않습니다. Timer0 mode7 TOP249 /64는 1 ms COMPA ISR, D5 PWM; core overflow interrupt는 꺼집니다. Timer1 mode14 TOP39999 /8는 D9 50 Hz hardware PWM. Timer2 CTC TOP124 /64는 2 kHz ISR로 D6 toggle (1 kHz). D3/D11 Timer2 PWM은 사용하지 마세요.

1. 실제 motor/servo 전원을 끄고 업로드합니다. boot에서 모든 출력 OFF를 확인합니다.
2. motor 대신 D4/D7에 각각 저항 포함 LED를 달아 방향 논리를 먼저 확인할 수 있습니다. 모터 출력 driver 연결 전 ACK 상태도 확인합니다.
3. MAIN의 FORWARD, REVERSE, ALL_STOP, LIGHT ON/OFF, WIPER ON/OFF, HORN ON/OFF, UNKNOWN을 각각 시험합니다. reversal delay와 500 ms motor 정지를 확인합니다.
4. button, MAIN UART disconnect, controller reset을 시험하여 전부 OFF와 새 session 요구를 확인합니다.
5. 전류 제한 bench PSU와 선택한 driver로 motor를 시험하고 mechanical 범위를 낮게 설정해 servo를 시험합니다.

ACK의 drive/lights/wiper/horn/angle은 controller의 명령 출력 상태입니다. 물리 센서 피드백이나 실제 장치 작동 증거가 아닙니다. host test는 `c++ -std=c++11 firmware/tests/actuator_engine_test.cpp -o /tmp/actuator_test && /tmp/actuator_test`; 실제 AVR build도 별도로 수행합니다. USB 업로드/실제 장치 시험은 팀이 각 보드에서 수행해야 합니다.
