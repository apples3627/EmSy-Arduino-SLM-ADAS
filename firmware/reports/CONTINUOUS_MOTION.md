# 전진·후진 지속 주행 수정 — 2026-10-13

검증 소스 커밋: `a3db3ea81e90855b55c9221dfadea884e305aece`. [최종 실행 로그·파일 해시](../verification/partial5/continuous_motion_results.json). protocol5/모델95616d56/라벨3 유지.

사용자가 제공한 실물 로그는 `forward → RESULT raw=0 effective=0 reason=0 → PLAN 1 0 → APPLIED drive=1 horn=0`입니다. 전진 분류와 MAIN의 적용 응답은 정상이며 모터/L298N에서 소리가 났다고 보고했습니다. 기존 기본 PWM75/255와500ms 동작이 출발하지 못하는 원인일 가능성은 있지만, 실제 전압/전류/회전은 측정하지 않았으므로 하드웨어 원인을 확정하지 않습니다.

SLM의 일반 계획 duration은0이고 MAIN은0을 지속 주행으로 처리합니다. STOP·정상 ADAS의 장애물·안전모드/연결 전환·UART 오류/2초 watchdog은 계속 정지합니다. 방향 반전100ms 무구동도 유지합니다. 기본 PWM은180/255(약71%)로 올렸고 주파수 약977Hz는 유지했습니다. 실제 정격과 부하는 MAIN board_config.h에서 맞춥니다.

직접 전송하는 nonzero duration은 최대1000ms의 기존 유한 주행을 지원합니다. 부가 출력만 바꾸는 계획은 주행 시간을 초기화하지 않습니다. 서브에이전트가 유한 주행 종료와 LED 적용이 같은 tick에서 겹치면 오래된 stagedDrive가 한 tick 동안 모터를 되살리는 문제를 재현했고 수정했습니다. 종료 때 보류된 부가 계획의 방향도0으로 지웁니다. 수정 후 direction/desired/applied 모두0을 유지함을 별도 재현으로 확인했습니다.

## 확인한 결과

- 실제 모델/Planner에서 forward와 go forward는 raw/effective0, 경적은8. 모델 변경 없음.
- continuous_motion_test: 10초 이상 주행 유지, 부가 명령 영향 없음, STOP, 반전100ms, 명시적 유한 주행의 종료, 종료 직전 LED 계획에 의한 재주행 방지, UART watchdog의1999/2000ms 경계 PASS.
- 실제 SLM+MAIN+SPI 통합: forward5500ms 유지, STOP, 주행 중 UART 단절 정지 PASS.
- 기존13개 독립 계약, 모델33문장 parity, 레지스터 I/O 스캔, UART 복구, I2C timeout/STOP 회귀 PASS.
- 네 Uno 정본과 네 배포 ZIP 컴파일·SHA-256 대조 PASS. MAIN13080B/808B, SLM22926B/1365B(Flash/정적 SRAM).

## 적용

[MAIN](../main/main.ino)과 [SLM_UART](../slm_uart/slm_uart.ino)를 각각 새 코드로 업로드합니다. SPI/ADAS는 기존 protocol5면 재업로드하지 않아도 됩니다. 정본 폴더나 최신 ZIP 전체의 src/를 유지합니다. 모터 전원을 끄고 업로드 후 바퀴를 들어 올려 `forward` 유지 → `stop` → `reverse` 유지 → `stop`을 시험합니다.

실제 하드웨어 업로드·모터 회전·공급 전압/전류는 이번 작업에서 확인하지 않았습니다. 지속 소음/미회전이면 L298N 모터 전원 및 OUT 전압, ENA/ENB와 방향 입력, 모터 부하를 확인해야 합니다. APPLIED drive=1은 논리 출력 상태이며 실제 회전을 측정한 값은 아닙니다.
