# MAIN 팀 업로드 안내

MAIN은 PC 문장을 토큰화하고 두 계산 보드의 10개 부분 점수를 더한 다음 액추에이터에 명령을 보냅니다. MAIN에서 신경망 `Intent::logits`를 호출하지 않습니다. 바이어스 `IntentMain::C`는 한 번만 더합니다. 모델 ID `0x30918170`, 어휘 247, 임베딩 16, 은닉 64 (각 계산 보드 32), 라벨 버전 2, 라벨 10을 모든 보드에서 일치시켜야 합니다.

`main` 폴더 전체를 압축해 전달하세요. `main.ino`, `src` 및 `src/model/generated`가 모두 필요합니다. Arduino IDE에서 `main.ino`를 열고 Arduino Uno / ATmega328P / 16 MHz를 선택하세요. 자기 보드에 **MAIN** 라벨을 붙이고 자기 USB 포트를 확인한 뒤 업로드하세요. 업로드 중 보드 사이 배선을 분리하고 USB만 연결하세요. 현재 구현은 물리 보드에 업로드하거나 시리얼 포트를 열어 검증하지 않았습니다.

MAIN D0/D1은 USB 시리얼 모니터용입니다. 외부 보드 또는 모터를 연결하지 마세요. MAIN에는 모터 드라이버를 연결하지 않습니다.

| MAIN 핀 | 상대 보드 핀 | 방향 / 기능 |
|---|---|---|
| D2 | ACTUATOR D1 TX | ACTUATOR → MAIN 소프트 UART RX |
| D3 | ACTUATOR D0 RX | MAIN → ACTUATOR 소프트 UART TX |
| A4 | COMPUTE_I2C A4 | SDA, 주소 0x12, 100 kHz |
| A5 | COMPUTE_I2C A5 | SCL, 100 kHz |
| D10 | COMPUTE_SPI D10 | MAIN SS → 슬레이브 SS |
| D11 | COMPUTE_SPI D11 | MAIN MOSI → 슬레이브 MOSI |
| D12 | COMPUTE_SPI D12 | 슬레이브 MISO → MAIN MISO |
| D13 | COMPUTE_SPI D13 | MAIN SCK → 슬레이브 SCK |
| GND | 각 상대 보드 GND | 모든 보드 공통 접지 |
| 5V | 각 상대 보드 5V 전원 기준 | 모두 5 V 로직. 전원은 팀의 단일 공급 구성을 확인하고 연결 |

여러 USB 전원을 사용할 때 5V 공급원끼리 직접 병렬 연결하지 마세요. I2C SDA/SCL에는 5V 풀업이 필요합니다. ACTUATOR D0/D1과 외부 UART는 업로드 동안 분리해야 합니다.

PC 시리얼 모니터는 **9600 baud, Newline**입니다. 먼저 `!status`를 입력하세요. 계산 보드나 액추에이터가 없으면 `ready=0`이고 차량 동작을 허용하지 않습니다. 나머지 팀원이 자기 펌웨어를 업로드하고 배선한 후 `!retry`로 새 세션을 시작하세요. 모든 메타데이터 핸드셰이크가 통과하면 `READY`가 출력되고 초기 STOP ACK를 기다립니다. `APPLIED`는 세션/명령 번호가 일치하는 액추에이터 ACK의 실제 출력 상태이며 센서 측정값이 아닙니다.

시험 문장: `go forward`, `go backward`, `stop`, `turn on the lights`, `turn off the lights`, `turn on the wipers`, `turn off the wipers`, `tell me a joke` (안전 정지), `turn on the horn`, `turn off the horn`. 동작 문장은 이전 `APPLIED` 뒤에 입력하세요. `  STOP!?  `도 즉시 STOP으로 처리합니다. STOP은 분류 대기 상태를 취소하고 이전 결과/ACK가 새 동작을 만들지 못하게 합니다. 바쁜 동안 새 일반 문장, OOV, 비 ASCII, 가드 문장, 96자를 넘는 문장은 안전 정지로 처리합니다. 문장은 최대 ASCII 96자, 16토큰입니다.

전진/후진 기본 시간은 500 ms이며 MAIN은 하트비트로 구동 시간을 연장하지 않습니다. 조명/와이퍼/경적 ON은 액추에이터의 래치 출력입니다. STOP은 모두 OFF로 요청합니다. 링크 오류/타임아웃도 STOP을 요청하며 액추에이터 ACK가 없으면 `ALL_STOP_unconfirmed`라고 표시합니다. 액추에이터 자체의 로컬 안전 정지와 구동 시간 제한도 필요합니다.

`!retry`는 진행 중 작업을 취소하고 EEPROM 세션을 증가시킵니다. MAIN EEPROM 마지막 4바이트(1020–1023)는 부팅 세션 카운터로 예약되어 있습니다. 지우거나 다른 코드에서 사용하지 마세요. 16비트 명령 번호의 랩 직전에 새 세션/핸드셰이크를 시작합니다. 세션 범위를 소진하면 동작을 거부합니다. 액추에이터 재부팅으로 세션 거절을 받으면 새 세션으로 다시 핸드셰이크합니다.

Timer0 COMPA 1 ms 시계, Timer1 COMPA/COMPB 소프트 UART 약 9615 baud, INT0(D2) 시작 비트를 사용합니다. Arduino Timer0 overflow를 비활성화합니다. USART0, TWI, SPI, EEPROM과 GPIO는 AVR 레지스터만 사용합니다. Arduino 코어의 setup/loop 시작 코드는 허용하지만 Serial/Wire/SPI/millis/delay/GPIO 래퍼는 사용하지 않습니다. SPI는 Mode 0, MSB first, 125 kHz입니다. 프레임은 32바이트, CRC8/길이/패딩/작업 번호를 검증합니다. 계산 결과는 워커별 두 청크(24/16바이트)를 각 한 번만 합칩니다.

`!trace`는 스케줄러의 순서를 표시합니다. 분류마다 액추에이터 UART STATUS를 먼저 예약하고 TWI와 SPI 비동기 전송을 곧바로 시작합니다. 물리 동시 통신은 코드/호스트 테스트로 증명되지 않습니다. 팀 통합 때 로직 분석기 6채널을 MAIN D3(TX), D2(RX), A4(SDA), A5(SCL), D10(SS), D13(SCK)에 연결하고 공통 GND를 연결하세요. 2 MHz 이상 샘플링으로 한 문장 전체를 캡처해 UART STATUS 프레임이 진행 중일 때 TWI 트래픽과 SPI SS/SCK가 함께 존재하는 구간을 확인하세요. 별도 캡처에서 D11/D12도 디코딩해 실제 32바이트 요청/응답, 작업 ID와 CRC를 확인하세요. 네 보드 전체의 속도 향상이나 물리 동작을 아직 측정했다고 주장하지 않습니다.

호스트 검증: `c++ -std=c++11 -Wall -Wextra -pedantic firmware/tests/main_policy_test.cpp -o /tmp/main_policy_test && /tmp/main_policy_test`. 실제 구현의 STOP 정규화, 취소 세대, 중복 청크 거절, 작업 ID, 비정상 float/CRC, 바이어스 단일 합산을 검증합니다. AVR 빌드: Arduino CLI `compile --warnings all --fqbn arduino:avr:uno firmware/main`. 실제 하드웨어, 전기적 노이즈, 타이밍 여유와 최종 영어 문장 예측은 팀 통합 시험이 필요합니다.
