# Protocol5 실물 통합·부분 연결 시험

1. 네 역할별 새 ZIP 전체를 배포합니다. 외부 배선·모터 전원 없이 각 Uno에 해당 .ino를 업로드합니다. 구버전4를 섞지 않습니다.
2. 전원을 끄고 PIN_MAP을 따라 UART 교차 D2/D3, 공통 GND를 연결합니다. 별도 USB 전원끼리5V를 묶지 않습니다. 우선 SLM+MAIN만 켜고 SLM USB에서 Python CLI를 실행합니다.
3. READY 뒤 `!status`에서 aux=0/adas_mode=BYPASS를 확인합니다. `go forward and lights on and horn on`은 모터 명령을 적용하고 부가 출력은 SKIPPED로 알려야 합니다. 모터 전원 없이 MAIN의 두 PWM·방향 출력부터 확인합니다.
4. SPI 연결은 MAIN D8→SPI D10, D11/12/13끼리입니다. MAIN D10은 ENB PWM이므로 SS와 연결하지 않습니다. 연결 변경은 OFF와 새 동기화·새 명령을 요구합니다. aux=1 뒤 LED·부저 ON/OFF를 확인합니다.
5. LCD는 SPI 보드 A4/A5에만 연결합니다. 주소0x27/0x3F, contrast, 백팩 매핑을 확인합니다. ADAS 미연결이면 N/A와 A:B여야 합니다. LCD만 분리해도 SPI·LED·부저가 동작해야 합니다. 수동 부저는 config에서 ACTIVE_BUZZER=0을 사용합니다.
6. MAIN A4/A5↔ADAS A4/A5, ADAS D4→MAIN D6+SPI D2의 공통 ALERT와 풀업을 연결합니다. HC-SR04 Trig D7/Echo D8/5V/GND를 확인합니다. 양 보드 I2C 내부 풀업과 약30.4kHz를 사용합니다.
7. 센서 오류/미연결의 flags21·ERR 상태에서 BYPASS와 새 명령 실행을 확인합니다. 실제 정상 거리25cm 이상3회 뒤 ACTIVE와 거리 표시를 확인합니다. 모드 변경 순간 모든 출력이 OFF이고 자동 재개하지 않아야 합니다.
8. 정상 ADAS에서10/19.9/20/25/400cm 경계 및 장애물을 시험합니다. 20cm 이하는 MAIN 두 PWM/방향, SPI LED/부저 OFF이고 LCD에는 실제 거리+STP가 표시돼야 합니다.25cm 이상3회 해제 뒤에도 새 명령이 필요합니다. 실제 출력 정지 지연을 기록합니다.
9. L298N ENA/ENB 점퍼 제거, ENA D9/ENB D10, IN1 D4/IN2 D7/IN3 A0/IN4 A1, 모터OUT1/2·OUT3/4를 확인합니다. 별도 정격 모터전원·로직 공급을 확인하고 바퀴를 들어 올린 뒤 시험합니다. 두 모터의 차량 기준 방향,5초 이상 주행 유지·STOP으로 정지와 반전100ms 대기를 확인합니다.
10. 주행 중 센서/I2C 또는 SPI 단절을 시험합니다. 전환 순간 현재 계획은 취소돼야 하며, BYPASS/aux=0 READY 뒤 새 모터 명령은 가능해야 합니다. SPI 단절 중 그 보드 출력은 로컬 watchdog2초 이내 OFF인지 별도로 확인합니다. 오래된 명령/ACK로 자동 재개하면 실패입니다.
11. SLM UART 분리·보드 재부팅·STOP·다중 명령의 충돌/unknown/와이퍼를 시험합니다. STOP은 연결 여부와 관계없이 MAIN을 즉시 정지하고 연결된 SPI로 전달해야 합니다. LCD 정보를 꺼야 하는 명령은 아닙니다.

기본값은 센서 고장에도 움직이는 요청된 시연 설정입니다. ADAS 필수 구성은 MAIN REQUIRE_ADAS_FOR_MOTION=true로 별도 컴파일합니다. 호스트/컴파일 통과는 위 실물 시험의 완료 증거가 아닙니다. 실제 결과·거리 정확도·배선사진·파형·팀원 기여는 팀에서 기록하세요.
