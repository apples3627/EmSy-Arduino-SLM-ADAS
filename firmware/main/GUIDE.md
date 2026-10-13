# MAIN 담당자 지침 — 두 모터·선택적 ADAS/SPI

`main` 폴더 전체의 main.ino를 Arduino Uno R3에 컴파일·업로드합니다. 모터 외부 전원을 끄고 역할별 USB 포트를 확인합니다. MAIN USB는 업로드/전원용이며 PC 영어 명령은 **SLM_UART USB**로 보냅니다. MAIN은 레지스터 I/O만 사용합니다. 자동 업로드는 수행하지 않았습니다.

최소 시연 구성은 PC + SLM_UART + MAIN입니다. ADAS_I2C와 PERIPHERAL_SPI는 선택적으로 연결합니다. `src/board_config.h`의 `REQUIRE_ADAS_FOR_MOTION=false`가 기본 시연 설정입니다. 센서 미연결·초기화·고장·오래된 측정이면 ADAS_BYPASS로 표시되고 새 모터 명령을 허용합니다. 이 설정에서는 센서 고장 때문에 ALERT가 LOW여도 ISR과 motorWrite가 모터를 차단하지 않습니다. RAW LOW는 LINK/DIAG에 계속 기록합니다. ADAS를 필수로 사용할 구성은 해당 상수를 true로 바꾸고 다시 컴파일합니다.

ADAS 메타데이터와 상태가 유효하고 VALID 비트, 정상 거리20~4000mm, 센서 age와 증가하는 sampleId의 마지막 갱신이 각각200ms 이내이면 ADAS_ACTIVE로 전환합니다. 이때 장애물(200mm 이하/OBSTACLE), 초기화 상태, 해제에 필요한 정상3회 미완료, 물리 ALERT LOW·짧은 sticky 펄스가 정지 우선권을 갖습니다. 센서가 끊기거나 고장 나면 BYPASS로 돌아갑니다. **모드 전환과 연결 변경은 두 모터를 OFF하고 새 safetyEpoch를 만들며 진행 중 계획을 취소합니다. 이전 주행은 자동 재개하지 않고 새 명령을 요구합니다.**

SPI가 없으면 모터만 로컬 적용하고 LED·부저 명령은 건너뜁니다. ACK/STATE_SYNC의 AUX_AVAILABLE(bit5)=0일 때 lights/horn 필드는 사용 불가이며 실제 출력 OFF를 확인했다는 뜻이 아닙니다. SPI가 정상 동기화됐으면 부가장치를 포함한 계획은 같은 session/epoch/id의 적용 ACK 뒤에 모터를 켭니다. 그 ACK가 시간초과되면 해당 계획을 취소·정지하지만 다음 새 모터 명령은 SPI 없이 실행할 수 있습니다. invalid/OOV/상충 자연어 계획은 SLM에서 전체 거부하며 지원하지 않는 와이퍼는 실행하지 않습니다.

STOP_ALL은 주변 보드 연결 여부와 무관하게 즉시 MAIN 모터와 desired 상태를 지우고 새 epoch를 만듭니다. 연결된 SPI에도 STOP을 전달하고 그 OFF ACK를 공유 상태에 반영합니다. 오래된 명령·ACK, 잘못된 CRC/session과 재전송 공격은 허용하지 않습니다. 같은 STOP의 중복은 새 epoch를 반복 생성하지 않습니다. UART 신선도2초 초과·수신 오류는 모터를 정지합니다. SPI 미연결 자체는 CORE READY를 차단하지 않습니다.

Protocol5 SPI 순서는 HELLO → SAFETY_POLICY(모드 및 현재 epoch, ACK2) → **정책보다 엄격히 새 epoch의 STOP_ALL** → 출력 명령입니다. ADAS 모드 변경도 OFF·정책·STOP 장벽 뒤에 부가장치를 다시 사용할 수 있습니다. SENSOR_TELEMETRY는 session/epoch/sampleId/실제 rangeMm/age/flags/mode를 전달합니다. 거리20~4000mm의 신선한 측정만 FRESH로 게시하며 미연결·고장·오래된 거리는 `0xffff`로 표시합니다. SLM이 끊겨도 SPI의 상태·snapshot·약100ms 목표 거리 갱신을 순환 서비스하므로 LCD가 현재 모드를 표시할 수 있습니다. 버스 전송이 느리면 실제 갱신 간격은 늘어납니다.

actorFlags는 bit0 반전 대기, bit2 ACTIVE ADAS 정지, bit3 UART/시작 차단, bit4 명령 진행 중, bit5 AUX_AVAILABLE, bit6 ADAS_ACTIVE, bit7 ADAS_BYPASS입니다. 일반 전진·후진 명령은 duration=0으로 STOP 또는 안전/통신 취소까지 유지합니다. 반전 대기100ms이며 기본 PWM은255/255(100%)로 두 채널을 함께 제어합니다. 0보다 큰 duration을 직접 지정하는 프레임은 기존처럼 최대1000ms의 유한 주행입니다. 부가 출력만 바꾸는 명령은 기존 주행 시간을 갱신하지 않습니다. `INVERT_MOTOR_A/B`는 설치 방향에 맞춰 설정합니다. appliedKnown은 MAIN 출력과 연결된 SPI ACK의 적용 확인이며 실제 바퀴 측정이 아닙니다.

[전체 핀 연결표](../PIN_MAP.md)의 현재 구성만 사용합니다. D2 RX/D3 TX→SLM 교차 UART, D6 ALERT, A4/A5→ADAS, D8 CS→SPI 보드D10, D11~13 SPI입니다. L298N IN1 D4/IN2 D7/IN3 A0/IN4 A1/ENA D9/ENB D10입니다. MAIN D10과 SPI 보드 D10을 연결하지 않습니다. Timer0 tick, Timer1 두 PWM, Timer2 soft UART를 분리했고 SPI 선택은 D8입니다. D10은 ENB 출력으로 유지합니다.

MAIN과 ADAS의 A4/SDA·A5/SCL 내부 풀업은 DDRC4/5=0, PORTC4/5=1, MCUCR.PUD=0이며 TWI 약30.4kHz(TWBR255/prescaler1)를 유지합니다. 주소0x12와 센서70ms 주기도 유지합니다. 두 보드를 USB로 공급하면 보드 간5V를 묶지 않고 GND만 공통 연결합니다. SPI LCD의 로컬 I2C 버스는 ADAS 버스와 별개입니다. 실물 배선·회로·모터와 버스 파형 검증은 통합 지침에 따라 별도로 수행합니다.

SLM 콘솔 `!status`, `!diag`, `!i2c`로 LINK/DIAG/I2C를 확인합니다. LINK alert는 RAW LOW이고 BYPASS 모드에서는 정지 사유가 아닙니다. I2C error1=쓰기주소 NACK,2=데이터 NACK,3=읽기주소 NACK,4=예상밖 상태,5=CRC/프레임/버전 오류,6=timeout,7=STOP timeout,8=메타데이터/상태 거부입니다. Protocol5의 ADAS HELLO는 reply15/len8/version5입니다. DIAG cause1 UART수신오류,2 UART watchdog,3 SPI timeout,4 SPI peer오류,5 SPI차단,7 ADAS freshness,8 ALERT,10 SPI세션복구입니다. 높은 EEPROM session을 가진 SPI의 BAD_SESSION hint는 모터 OFF 후 MAIN 새 session을 저장하고 재핸드셰이크합니다(boot당 최대3회/1초 간격).

호스트 정책 테스트:
```sh
c++ -std=c++17 -Wall -Wextra firmware/tests/partial_main_policy_test.cpp -o /tmp/partial-main-policy
/tmp/partial-main-policy
```

Arduino CLI Uno 컴파일도 확인하며 MAIN SRAM 예산은1536바이트 이하입니다. 장치 업로드·물리 주행 시험은 이 검증에 포함하지 않습니다.

## 전진 로그는 정상인데 모터에서 소리만 나는 경우

`RESULT raw=0 effective=0`, `APPLIED drive=1 horn=0`이면 전진으로 처리된 상태이며 피에조 경적 명령은 아닙니다. 기존 약30% PWM/500ms 구동에서는 출발 토크가 부족할 수 있어 이번 기본값을255/255로 변경하고 일반 주행을 지속하도록 수정했습니다. PWM 주파수 약977Hz는 그대로이며 모터/드라이버에서 들리는 소리만으로 고장을 확정하지 않습니다. MAIN+SLM을 새 코드로 업로드합니다.

소리만 계속 나면 모터 동작 중 L298N 모터전원과 OUT 전압, ENA/ENB PWM, 네 방향 입력을 확인합니다. 외부 모터전원·드라이버 전압강하·공급 전류·부하에 따라 출력 신호가 정상이어도 모터가 출발하지 못할 수 있습니다. MOTOR_DUTY는 board_config.h에서 조정하며 실제 모터 공급과 정격에 맞춥니다. 소프트웨어 drive=1은 실제 회전 측정이 아닙니다.

## 최대 출력 설정

현재 MOTOR_DUTY=255로 ENA/ENB 모두100% 출력입니다. 지속 주행 수정본이 이미 업로드됐다면 이번 변경은 MAIN만 재업로드합니다. 실제 회전 속도는 모터전원과 부하에 따라 달라집니다. STOP·정상 ADAS 정지·통신 단절 정지와 방향 반전 대기는 유지합니다.
