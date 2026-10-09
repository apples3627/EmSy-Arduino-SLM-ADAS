# MAIN 팀 업로드 안내

MAIN은 PC 문장을 토큰화하고 두 계산 보드의 10개 부분 점수를 더한 다음 액추에이터에 명령을 보냅니다. MAIN에서 신경망 `Intent::logits`를 호출하지 않습니다. 바이어스 `IntentMain::C`는 한 번만 더합니다. 모델 ID/어휘 태그는 배포된 `src/model/generated/config.h`와 공통 프로토콜 값이 같아야 하며, 임베딩 16, 은닉 64 (각 계산 보드 32), 라벨 버전 3, 라벨 10을 모든 보드에서 일치시켜야 합니다.

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
| 5V | MAIN 쪽 I2C 풀업 기준만 | MAIN 5V에 4.7 kΩ 풀업 한 쌍(SDA/SCL). USB로 각각 공급되는 상대 보드의 5V와 점퍼하지 않음 |

여러 USB 전원을 사용할 때 5V 공급원끼리 직접 병렬 연결하지 마세요. I2C SDA/SCL에는 MAIN의 5V를 기준으로 한 4.7 kΩ 풀업 한 쌍만 연결하세요. 모든 보드의 GND 연결은 필수입니다. ACTUATOR D0/D1과 외부 UART는 업로드 동안 분리해야 합니다.

PC 시리얼 모니터는 **9600 baud, Newline**입니다. 먼저 `!status`를 입력하세요. 계산 보드나 액추에이터가 없으면 `ready=0`이고 차량 동작을 허용하지 않습니다. 나머지 팀원이 자기 펌웨어를 업로드하고 배선한 후 `!retry`로 새 세션을 시작하세요. 모든 메타데이터 핸드셰이크와 초기 STOP의 일치하는 ACK가 완료된 뒤 `READY`가 출력됩니다. `READY` 직후 일반 문장을 입력할 수 있습니다. `APPLIED`는 세션/명령 번호가 일치하는 액추에이터 ACK의 실제 출력 상태이며 센서 측정값이 아닙니다.

시험 문장: `go forward`, `go backward`, `stop`, `turn on the lights`, `turn off the lights`, `turn on the wipers`, `turn off the wipers`, `tell me a joke` (안전 정지), `turn on the horn`, `turn off the horn`. 동작 문장은 이전 `APPLIED` 뒤에 입력하세요. `  STOP!?  `도 즉시 STOP으로 처리합니다. STOP은 분류 대기 상태를 취소하고 이전 결과/ACK가 새 동작을 만들지 못하게 합니다. 분류 중 새 일반 문자의 첫 바이트는 진행 중 계획을 취소하고 안전 정지를 요청합니다. `!status`/`!trace`는 별도 12바이트 버퍼로 처리하여 진행 중 계획을 보존합니다. 바쁜 동안 새 일반 문장, OOV, 비 ASCII, 가드 문장, 192자를 넘는 문장은 안전 정지로 처리합니다. 전체 문장은 최대 ASCII 192자, 각 절은 최대 96자/16토큰이며 최대 네 절입니다.

전진/후진 기본 시간은 500 ms이며 MAIN은 하트비트로 구동 시간을 연장하지 않습니다. 조명/와이퍼/경적 ON은 액추에이터의 래치 출력입니다. STOP은 모두 OFF로 요청합니다. 링크 오류/타임아웃도 STOP을 요청하며 액추에이터 ACK가 없으면 `ALL_STOP_unconfirmed`라고 표시합니다. 액추에이터 자체의 로컬 안전 정지와 구동 시간 제한도 필요합니다.

`!retry`는 진행 중 작업을 취소하고 EEPROM 세션을 증가시킵니다. MAIN EEPROM 마지막 4바이트(1020–1023)는 부팅 세션 카운터로 예약되어 있습니다. 지우거나 다른 코드에서 사용하지 마세요. 16비트 명령 번호의 랩 직전에 새 세션/핸드셰이크를 시작합니다. 세션 범위를 소진하면 동작을 거부합니다. 액추에이터 재부팅으로 세션 거절을 받으면 새 세션으로 다시 핸드셰이크합니다.

Timer0 COMPA 1 ms 시계, Timer1 COMPA/COMPB 소프트 UART 약 9615 baud, INT0(D2) 시작 비트를 사용합니다. Arduino Timer0 overflow를 비활성화합니다. USART0, TWI, SPI, EEPROM과 GPIO는 AVR 레지스터만 사용합니다. Arduino 코어의 setup/loop 시작 코드는 허용하지만 Serial/Wire/SPI/millis/delay/GPIO 래퍼는 사용하지 않습니다. SPI는 Mode 0, MSB first, 125 kHz입니다. 프레임은 32바이트, CRC8/길이/패딩/작업 번호를 검증합니다. 계산 결과는 워커별 두 청크(24/16바이트)를 각 한 번만 합칩니다.

`!trace`는 스케줄러의 순서를 표시합니다. 분류마다 액추에이터 UART STATUS를 먼저 예약하고 TWI와 SPI 비동기 전송을 곧바로 시작합니다. 물리 동시 통신은 코드/호스트 테스트로 증명되지 않습니다. 팀 통합 때 로직 분석기 6채널을 MAIN D3(TX), D2(RX), A4(SDA), A5(SCL), D10(SS), D13(SCK)에 연결하고 공통 GND를 연결하세요. 2 MHz 이상 샘플링으로 한 문장 전체를 캡처해 UART STATUS 프레임이 진행 중일 때 TWI 트래픽과 SPI SS/SCK가 함께 존재하는 구간을 확인하세요. 별도 캡처에서 D11/D12도 디코딩해 실제 32바이트 요청/응답, 작업 ID와 CRC를 확인하세요. 네 보드 전체의 속도 향상이나 물리 동작을 아직 측정했다고 주장하지 않습니다.

호스트 검증: `c++ -std=c++11 -Wall -Wextra -pedantic firmware/tests/main_policy_test.cpp -o /tmp/main_policy_test && /tmp/main_policy_test`. 실제 구현의 STOP 정규화, 취소 세대, 중복 청크 거절, 작업 ID, 비정상 float/CRC, 바이어스 단일 합산을 검증합니다. AVR 빌드: Arduino CLI `compile --warnings all --fqbn arduino:avr:uno firmware/main`. 실제 하드웨어, 전기적 노이즈, 타이밍 여유와 최종 영어 문장 예측은 팀 통합 시험이 필요합니다.

통합 호스트 시험: `firmware/tests/main_integration_test.sh`는 실제 MAIN 헤더와 가짜 AVR 레지스터를 실행하고, 실제 액추에이터 엔진 및 I2C/SPI 워커를 각각 별도 번역 단위로 연결합니다. 초기 STOP ACK 뒤 READY, 두 토큰 청크/점수/바이어스 단일 합산, 반전 ACK 지연, 오래된 결과/ACK 취소, CRC/NaN/작업 ID 오류, 3초 계산 대기 중 같은 작업 ID의 STATUS 유지, 32,769번 유휴 STATUS 조회 동안 명령 번호 유지, USART FE/DOR/UPE, TWI 무인터럽트 및 STOP 비트 고착 타임아웃, UART TX 비트와 RX 반비트 샘플을 검증합니다. 유휴 상태 조회는 현재 명령 번호를 재사용하며 명령 번호를 소비하지 않습니다. 계산 대기 중에도 500 ms마다 STATUS를 보내지만 구동 시간을 갱신하지 않습니다. 이 시뮬레이션은 실제 4대 Uno의 전기적/인터럽트 타이밍 측정을 대체하지 않습니다. 별도 작업 트리에서 시험할 때는 `MAIN_PEER_ROOT=/path/to/integrated/firmware`를 지정할 수 있습니다.

## 여러 동작을 한 번에 요청

`go forward and turn on the lights`, `go forward; turn on the lights; turn on the wipers`, `go forward, turn on the lights, turn on the wipers, turn on the horn`처럼 각 절에 완전한 동작을 적으세요. `and`, `also`, `plus`, 쉼표, 세미콜론이 경계입니다. 모든 절은 두 계산 보드에서 순서대로 추론하며, 마지막 절까지 검증한 후 하나의 `ACTUATE_BATCH`만 보냅니다. 첫 절이 유효해도 다음 절의 오류/타임아웃 전에 실행하지 않습니다. `PLAN actions=... reason=...`은 결정이고, 실제 적용은 일치하는 `APPLIED` ACK로 확인합니다.

STOP이 한 절이라도 있으면 전체 OFF가 우선합니다. UNKNOWN은 전체 거부/정지이며, 전진+후진 또는 같은 장치 ON+OFF도 전체 거부/정지입니다. 같은 동작의 반복은 하나로 합칩니다. `turn off the lights and turn on the horn`은 서로 다른 장치에 각각 적용됩니다. 모든 네 보드는 프로토콜 버전 2/라벨 버전 3/같은 모델로 함께 업데이트해야 READY가 됩니다.

시간 조건, `then` 순차 실행, `or` 대안, `turn on lights and horn` 같은 공통 동사/목적어 생략은 지원 범위 밖입니다. 자연어 문장 전체를 이해하는 파서는 아니며, 각 절의 신경망 분류가 거부되면 전체 정지합니다.
