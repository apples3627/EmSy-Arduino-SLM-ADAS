# MAIN 담당자 지침 — 두 모터·UART/I2C/SPI 중재

`main` 폴더 전체를 풀어 main.ino를 Arduino Uno R3에 업로드합니다. 외부 배선과 모터 전원을 분리하고 해당 USB 포트를 선택합니다. MAIN USB는 업로드/전원용이며 PC 영어 명령은 **SLM_UART USB**에 입력합니다. 레지스터 I/O만 사용하며 별도 Arduino 통신/Servo 라이브러리가 필요 없습니다.

[전체 핀 연결표](../PIN_MAP.md)의 현재2026-10-13 구성만 사용합니다. 독립 ZIP에는 같은 파일이 PIN_MAP.md로 동봉됩니다. 핵심: D2RX/D3TX→SLM 교차UART, D6ALERT, A4/A5→ADAS, D8CS→SPI 보드D10, D11~13SPI입니다. L298N은 IN1D4/IN2D7/IN3A0/IN4A1/ENAD9/ENBD10입니다. MAIN D10과 SPI 보드D10을 연결하지 않습니다.

MAIN은 EEPROM에서 새session을 만들고 SLM/peripheralHELLO 및 ADAS metadata를 확인합니다. 센서 정상3회와 새epoch STOP의OFF ACK가 있어야 READY입니다. SLM 계획을 검증하고 주변장치의 같은session/epoch/id ACK를 받은 뒤 두 모터를 켭니다. ADAS D6의PCINT22는 PWM compare와 네 방향 출력을 직접 끕니다. 짧은 LOW/HIGH 펄스도 sticky 이벤트로 남아 전경 루프의 출력 재활성화를 막습니다.

SLM STOP은 MAIN의 새epoch를 만들며 desired전부0, appliedKnown0을 게시합니다. 주변장치OFF ACK와 모터OFF를 확인한 뒤 applied전부0/known1로 동기화합니다. 오래된 명령·ACK는 무시합니다. 반복된 동일STOP은 대기 중 새epoch를 계속 만들지 않습니다. 센서 해제 뒤 이전 명령을 재실행하지 않습니다.

모터 기본500ms lease, 약30% PWM이며 두 모터 모두 전진/후진/정지합니다. 반전100ms 동안 둘 다OFF이고 부가 변경도 대기합니다. INVERT_MOTOR_A/B는 설치 방향에 맞춰 설정합니다. LCD·부저·LED는 SPI 보드가 담당하고 와이퍼는 사용하지 않습니다. 로컬 버튼 기능은 이번 MAIN 핀 배치에 없습니다.

Timer0tick, Timer1두PWM, Timer2softUART를 분리했습니다. SPI선택은D8이며 D10은 OUTPUT을 유지한ENB입니다. I2C/SPI단계와 UART를 비차단 서비스하며 ADC timeout, SPI300ms재시도2회 실패, UART2s신선도 실패에서 전체 정지합니다. appliedKnown은 출력 신호 적용 확인이며 실제 바퀴 센서 측정이 아닙니다.

독립 host/레지스터 및 Uno컴파일 검증을 사용합니다. 실제4대 배선·회로·모터 시험은 통합 지침을 따라 별도로 합니다. 자동 업로드는 수행하지 않았습니다.
