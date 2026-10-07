# COMPUTE I2C Uno 담당자 안내

이 폴더는 **I2C COMPUTE 전용 Arduino Uno R3** 펌웨어입니다. 다른 COMPUTE 역할 또는 Main/액추에이터 펌웨어로 바꾸지 마세요. 이 보드는 신경망 은닉 뉴런 0–31만 계산하고 10개 float32 부분 점수를 반환합니다. 모터/서보/LED/경적을 제어하지 않습니다.

## Arduino IDE 업로드

1. `compute_i2c` 폴더 전체를 전달받습니다. `compute_i2c.ino`와 `src/`의 모든 파일(모델 및 protocol 포함)을 함께 유지합니다.
2. Arduino IDE에서 `compute_i2c.ino`를 열고 보드를 Arduino Uno로 선택합니다. 담당자 본인의 USB 포트를 확인해 선택합니다.
3. 업로드할 때 버스 배선을 분리하고 USB로 연결해 업로드합니다. 별도 라이브러리 다운로드는 필요 없습니다. 모델은 번들에 포함됩니다.
4. 업로드 후 USB를 분리하고 회로의 5 V/GND 전원을 연결한 다음 역할에 맞게 버스를 연결합니다. USB와 회로 전원을 동시에 연결하지 않습니다.

## 배선과 버스 조건

- Main A4(SDA) ↔ 이 보드 A4, Main A5(SCL) ↔ 이 보드 A5. GND를 공통 연결합니다.
- SDA/SCL 각각 4.7 kΩ 외부 풀업을 5 V에 연결합니다. 전체 버스에 한 쌍만 사용합니다.
- 슬레이브 주소는 7비트 `0x12`입니다. 레지스터 TWAR 값은 `0x24`입니다.
- Main은 32바이트 요청을 쓰고 STOP 후 최소 1 ms를 기다린 뒤 32바이트 응답을 읽습니다. 짧거나 긴 쓰기는 버립니다. 계산 중 폴링은 25–50 ms 간격입니다.

D0/D1 UART 연결은 없습니다. PC 시리얼 모니터로 입력하거나 출력하지 않습니다. D4는 계산 중 HIGH, 그 외 LOW인 로직 분석기 관찰 핀입니다. LED가 깜박인다는 확인 절차는 없습니다.

## 통신 계약

- 고정 32바이트 프레임: SOF `0xa5`, version 1, opcode, job16 LE, seq, payload 길이, 최대 24바이트 payload, zero padding, CRC8(poly `0x07`, 처음 31바이트).
- HELLO는 payload 길이 0 또는 4를 허용하며 작업/입력/대기 큐를 초기화합니다. HELLO_ACK는 모델 `0x30918170`, vocab tag `0x766b5682`, labels version 2, 클래스 10, embedding 16, hidden 64, 담당 범위 및 vocab 247을 알립니다.
- LOAD_INPUT seq0/길이24는 토큰 0–11, seq1/길이8은 토큰 12–15입니다. 토큰은 uint16 LE, 정상 ID 0–246, PAD `0xffff`입니다. seq0부터 같은 job ID로 전송하고 START합니다. 같은 chunk/내용 재전송은 ACK, 다른 내용은 BAD_JOB입니다. 전부 PAD 입력과 잘못된 토큰은 거절합니다.
- START ACK를 준비한 다음 foreground에서 계산합니다. ISR에서는 부동소수점 계산을 하지 않습니다. 동일 job START 재전송은 재계산하지 않습니다.
- STATUS_REPLY payload 6바이트: state, error, 입력 mask, role(1), activeJob16 LE. state 0 IDLE/1 RECEIVING/2 COMPUTING/3 READY/4 FAULT입니다.
- GET_RESULT seq0은 처음 6개 float(24바이트), seq1은 마지막 4개 float(16바이트)를 반환합니다. global bias는 포함하지 않습니다. Main에서 두 보드 점수와 global bias를 한 번 더합니다. 승자 intent 결정은 Main에서 합니다.
- 수신 큐는 완료된 요청 2개까지 보관하며 추가 요청은 버립니다. 응답이 없어지거나 이전 응답이면 Main은 동일 요청을 재시도합니다. CRC 오류는 작업을 시작하거나 변경하지 않습니다.

## 검증 범위

AVR core 1.8.6 `arduino:avr:uno` 컴파일과 실제 모델 헤더를 이용한 host 검사를 수행했습니다. USB 업로드 및 실제 배선/파형 확인은 수행하지 않았습니다. 담당자가 회로에서 HELLO metadata, START→STATUS READY, RESULT 두 조각 및 재시도를 확인해야 합니다.
