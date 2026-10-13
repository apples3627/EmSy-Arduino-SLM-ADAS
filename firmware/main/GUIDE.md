# MAIN 담당자 지침 — 두 모터·UART/I2C/SPI 중재

`main` 폴더 전체를 풀어 main.ino를 Arduino Uno R3에 업로드합니다. 외부 배선과 모터 전원을 분리하고 해당 USB 포트를 선택합니다. MAIN USB는 업로드/전원용이며 PC 영어 명령은 **SLM_UART USB**에 입력합니다. 레지스터 I/O만 사용하며 별도 Arduino 통신/Servo 라이브러리가 필요 없습니다.

[전체 핀 연결표](../PIN_MAP.md)의 현재2026-10-13 구성만 사용합니다. 독립 ZIP에는 같은 파일이 PIN_MAP.md로 동봉됩니다. 핵심: D2RX/D3TX→SLM 교차UART, D6ALERT, A4/A5→ADAS, D8CS→SPI 보드D10, D11~13SPI입니다. L298N은 IN1D4/IN2D7/IN3A0/IN4A1/ENAD9/ENBD10입니다. MAIN D10과 SPI 보드D10을 연결하지 않습니다.

MAIN은 EEPROM에서 새session을 만들고 SLM/peripheralHELLO 및 ADAS metadata를 확인합니다. 센서 정상3회와 새epoch STOP의OFF ACK가 있어야 READY입니다. SLM 계획을 검증하고 주변장치의 같은session/epoch/id ACK를 받은 뒤 두 모터를 켭니다. ADAS D6의PCINT22는 PWM compare와 네 방향 출력을 직접 끕니다. 짧은 LOW/HIGH 펄스도 sticky 이벤트로 남아 전경 루프의 출력 재활성화를 막습니다.

SLM STOP은 MAIN의 새epoch를 만들며 desired전부0, appliedKnown0을 게시합니다. 주변장치OFF ACK와 모터OFF를 확인한 뒤 applied전부0/known1로 동기화합니다. 오래된 명령·ACK는 무시합니다. 반복된 동일STOP은 대기 중 새epoch를 계속 만들지 않습니다. 센서 해제 뒤 이전 명령을 재실행하지 않습니다.

모터 기본500ms lease, 약30% PWM이며 두 모터 모두 전진/후진/정지합니다. 반전100ms 동안 둘 다OFF이고 부가 변경도 대기합니다. INVERT_MOTOR_A/B는 설치 방향에 맞춰 설정합니다. LCD·부저·LED는 SPI 보드가 담당하고 와이퍼는 사용하지 않습니다. 로컬 버튼 기능은 이번 MAIN 핀 배치에 없습니다.

Timer0tick, Timer1두PWM, Timer2softUART를 분리했습니다. SPI선택은D8이며 D10은 OUTPUT을 유지한ENB입니다. I2C/SPI단계와 UART를 비차단 서비스하며 ADC timeout, SPI300ms재시도2회 실패, UART2s신선도 실패에서 전체 정지합니다. appliedKnown은 출력 신호 적용 확인이며 실제 바퀴 센서 측정이 아닙니다.

독립 host/레지스터 및 Uno컴파일 검증을 사용합니다. 실제4대 배선·회로·모터 시험은 통합 지침을 따라 별도로 합니다. 자동 업로드는 수행하지 않았습니다.

## UART/SPI 통신 진단 업데이트 — 2026-10-13

PC의 SLM 콘솔에서 `!status`는 STATUS와 LINK를 출력하고, `!diag`는 MAIN의 상세 카운터를 출력합니다. 먼저 MAIN/SLM_UART/PERIPHERAL_SPI 세 역할을 이 수정본으로 업데이트합니다. 모터 외부 전원을 끄고 역할별 포트를 확인해 업로드한 뒤 연결을 복원하세요. ADAS의 센서/배선 규칙은 그대로입니다.

```text
!status
!diag
```

LINK uart는 MAIN이 연산 보드 응답을 확인했는지, spi는 주변 보드 메타데이터를 확인했는지, spi_safe는 STOP 동기화됐는지, adas는 센서 상태 프레임이 유효한지, clear는 정상 거리3회/freshness 조건인지, alert는 물리LOW인지입니다. uart=1/spi=0이면 UART 전체 단절로 판단하지 말고 SPI D8→D10/11/12/13와 주변 보드 역할/버전을 확인합니다. uart=0이면서 rx_errors가 증가하면 교차D2/D3/GND와 UART 수신 타이밍을 확인합니다.

DIAG cause:1 UART수신오류,2 UART2s watchdog,3 SPI응답 timeout,4 SPI peer 오류,5 SPI안전동기화미완료,6 잘못된적용ACK(예약),7 ADAS freshness,8 ALERT,9 잘못된UART명령(예약),10 SPI 세션복구입니다. peer_error:6모델/라벨불일치,7세션거부,13오래된epoch,14안전차단입니다. rx_bytes/rx_frames/rx_errors는 MAIN UART 수신, spi_bad는 CRC/전송확인 실패 수입니다. 원인은 마지막 기록이며 정상 복구 후에도 남을 수 있습니다.

수정은 송신32바이트 복사 중의 인터럽트 차단 제거(완성 후8bit head 공개), SLM RX half-bit 계산의 나눗셈 제거, SPI 세션거부 시 motorsOFF→새세션EEPROM저장→양쪽재핸드셰이크(boot당최대3회/1s간격), 느린SPI에서도 상태snapshot우선 전달입니다. SPI ERROR_REPLY의 HELLO/BAD_SESSION은 선택적으로 EEPROM의최고세션을 포함합니다. 오래된 낮은hint는 무시하며 replay 보호를 해제하지 않습니다.

기존 STOP/ADAS/epoch/READY 차단은 유지됩니다. `light` 대신 `lights on/off`를 사용하고 READY 전 일반 명령을 보내지 않습니다. 이 수정은 코드의 취약점과 진단부족을 보완한 것이며 현재 실물 실패 원인이 단 하나로 확정됐다는 뜻은 아닙니다. 새펌웨어의 LINK/DIAG 출력으로 실물을 재확인해야 합니다.
