# 팀 통합 시험: 업로드부터 발표 증거까지

## 담당자 체크

- [ ] MAIN/I2C/SPI/ACTUATOR 라벨을 각 보드에 붙임.
- [ ] 각 역할 ZIP 전체를 풀고 자기 `.ino`와 `src/`로 컴파일·업로드함.
- [ ] 네 보드가 Uno R3/ATmega328P/16MHz임.
- [ ] 각 계산 역할의 모델 ID30918170·라벨버전2·범위0~31/32~63을 변경하지 않음.
- [ ] 외부 선 분리 상태로 업로드, 특히 제어 D0/D1을 분리함.
- [ ] 전원 OFF 후 PIN_MAP대로 신호·공통 GND 연결, 별도 USB 5V끼리 연결하지 않음.
- [ ] SDA/SCL 풀업 한 쌍, SPI 방향, UART 교차 연결을 확인함.

## 1. 모터·서보 외부 전원 OFF, 통신부터

1. 네 MCU에 전원을 공급하고 MAIN PC 콘솔만 9600/Newline으로 엽니다.
2. `!status`를 보냅니다. `actor=1 i2c=1 spi=1 ready=1`을 목표로 확인합니다.
3. 초기 ALL_STOP `APPLIED` 후 `READY model=30918170 labels=2`를 확인합니다. 오류면 `!retry`로 새 세션을 만듭니다.
4. `go forward`를 보내 `RESULT`와 `APPLIED`를 확인합니다. 실제 모터 없이 출력 상태만 검사합니다.
5. OOV·`do not move forward`·긴 입력은 `ALL_STOP` 또는 유효 의도2로 처리되어야 합니다.

## 2. 부품 개별 시험

선택한 H-bridge·모터·서보·피에조 정격 및 전원 연결을 먼저 확정합니다. 모터 대신 저항 포함 방향 LED로 논리를 확인한 뒤 실제 드라이버와 전류 제한 공급으로 시험할 수 있습니다.

|시험 입력|확인|
|---|---|
|`go forward`|설정 방향 회전, 약500ms 후 모터 정지|
|`go backward`|반대 방향, 반전 시 무구동100ms|
|`turn on the lights` / `turn off the lights`|LED만 ON/OFF|
|`turn on the wipers` / `stop the wipers`|왕복 / signal LOW, 기구 간섭 없음|
|`turn on the horn` / `silence the horn`|피에조 ON/OFF|
|조명·와이퍼·경적 ON 후 `stop`|네 기능 모두 OFF, 서보 park 재개 없음|
|`stop the horn`|경적만 OFF, 다른 상태 유지|

기본 모터 동작은 500ms 뒤 종료됩니다. 조명·와이퍼·경적은 별도 OFF/STOP 또는 통신 watchdog 전까지 유지됩니다. ACK는 출력 상태 확인이며 물리 동작은 눈·영상·필요한 센서로 따로 확인합니다.

## 3. 오류·재시작

- [ ] STOP을 계산 대기 중 보냄: 이전 결과가 새 ACTUATE를 만들지 않음.
- [ ] ACTUATOR 버튼을 누름: 전체 OFF. 해제만으로 재개 안 됨. `!retry` 후 새 명령 필요.
- [ ] UART 선 분리: 제어 보드 2초 watchdog 전체 OFF; MAIN은 확인되지 않은 STOP을 성공이라고 출력하지 않음.
- [ ] I2C/SPI 계산 보드 분리: 새 구동 없음, 명시 오류 및 STOP 요청, 재연결 후 `!retry`.
- [ ] ACTUATOR USB 리셋: 새 세션으로 재연결, 이전 명령/구동 시간 재실행 없음.
- [ ] 방향 전환 대기 중 STOP: PWM/와이퍼/조명/경적 모두 OFF, 늦은 ACK/park 재개 없음.
- [ ] 다음 정상 명령이 복구되는지 확인.

## 4. 과제의 동시 통신 증거

단순히 UART→I2C→SPI를 순서대로 사용한 로그로는 동시 운용을 입증하지 않습니다. MAIN은 추론 중 제어 보드의 UART STATUS 거래를 유지하면서 I2C·SPI 실제 모델 패킷을 보냅니다. 분류 완료 뒤 최종 ACTUATE를 보냅니다.

권장 로직 분석기 연결: MAIN D3(TX), A4(SDA), A5(SCL), D10(SS), D11(MOSI), D13(SCK) + 공통 GND. 8채널이면 D2(RX), D12(MISO)도 추가합니다. 2MHz 이상 샘플링으로 영어 명령 한 건 전체를 캡처하세요. UART 9600 8N1, I2C7비트주소0x12/100kHz, SPI Mode0 MSB/125kHz로 디코딩합니다.

- [ ] UART STATUS 프레임·I2C 트랜잭션·SPI SS/SCK 전송이 같은 추론 작업에서 겹치는 공통 구간 표시.
- [ ] 실제 token/job ID·CRC·응답 종류를 디코딩하고 패킷 로그와 맞춤.
- [ ] 최종 ACTUATE가 두 점수 수신 이후임을 확인.
- [ ] CSV/VCD·스크린샷·배선 사진·각 보드/버전·출력 영상 저장.

현재 호스트 시뮬레이션과 컴파일 결과는 전기적 타이밍·IRQ 지터·배선 노이즈까지 확인한 실제 네 보드 시험을 대체하지 않습니다.

## 5. 제출·발표

- [ ] 실제 Flash/RAM·작업 지연·오류 시험표를 작성. 코드/시뮬레이션/실물 증거를 구분.
- [ ] 보고서 첫 페이지에 본인을 제외한 팀원별 기여 Yes/No 표시.
- [ ] 10월20일 발표를 4분40초로 리허설하여 5분 미만 유지.
- [ ] 현장 시연 실패 대비 영상을 준비하되 미검증 항목을 완료로 적지 않음.
