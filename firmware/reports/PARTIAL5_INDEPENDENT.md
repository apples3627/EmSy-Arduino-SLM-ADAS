# Protocol5 독립 검증 — 2026-10-13

검증한 정본 소스 커밋: `e07b1ce37faf199b7fcb75636a0ed99228313258`. MAIN/SLM/ADAS/SPI 파일 SHA-256과 실제 실행 로그는 [results.json](../verification/partial5/results.json)에 있습니다. 결과는 호스트·컴파일 검증 PASS이고 실물 업로드/동작은 검증하지 않았습니다.

요구사항은 구현 검토 전에 [acceptance_seal.json](../verification/partial5/acceptance_seal.json)에 고정했습니다. SHA-256: `f699ad195b9f51f7fe9e28f1acbf43e2c37cfcac511f597c28b4ab3b1a1fcf44`. 독립 검증 작업자가 별도 worktree에서 작성한 테스트를 MAIN이 합쳐진 실제 코드에 다시 실행했습니다. 최종 기록은 네 역할 모두 같은 정본 커밋/파일을 대상으로 합니다.

## 검증 범위

- 최소 SLM+MAIN에서 ADAS/SPI 없이 새 모터 명령 실행.
- ADAS flags21 및 물리 ALERT LOW에서 명시적 BYPASS 정책·새 STOP 장벽 뒤 LED/부저 실행.
- 정상 유효 거리에서 ACTIVE 전환,10/19.9/20/25/400cm 경계와20cm 정지.
- 모드·연결 전환의 OFF/명령 취소 및 자동 재개 차단, 필수 ADAS 설정 true의 기존 차단.
- 실제 MAIN·SLM·SPI 엔진을 별도 번역 단위로 묶어 명령·ACK·상태 공유 검증.
- 실제 AVR 코드의 두 모터 PWM/방향 및 active/passive 부저·ALERT ISR·내부 I2C 풀업 검증.
- LCD PCF8574 출력 바이트를 해석해 실제 거리 문자·ERR/N/A/STA·BYPASS를 확인. 주소0x27/0x3F 둘 다 실행.
- SID/epoch/sample wrap·오래된 정책/명령 거부·재전송·watchdog·STOP 확인.

독립 검토 중 동일 측정 sample의 텔레메트리 재전송이 age를 다시 작게 만들 수 있는 문제를 발견해 수정했습니다. 현재는 이전 age+경과 시간 아래로 줄이지 못하며 stale 유지 검증도 통과합니다.

## 실행 결과

|독립 테스트|결과|
|---|---|
|protocol_contract|PASS|
|main_strict_contract|PASS|
|main_register_contract|PASS|
|peripheral_contract|PASS|
|peripheral_register_contract|PASS|
|peripheral_register_contract_passive_3f|PASS|
|lcd_display_contract|PASS|
|lcd_display_contract_3f|PASS|
|slm_contract|PASS|
|adas_contract|PASS|
|adas_capture_contract|PASS|
|adas_board_contract|PASS|
|actual_peer_integration|PASS|

총13개 독립 항목 PASS. 기본 테스트 도구에서 별도로 모델33문장 점수 parity(max error0), 부분 연결 정책, I2C STOP/timeout FSM, UART 세션 복구와 레지스터 전용 API 스캔도 PASS입니다.

|Uno 역할|Flash bytes /32256|정적 SRAM bytes /2048|
|---|---:|---:|
|main|13062|808|
|peripheral_spi|7498|493|
|slm_uart|22934|1365|
|adas_i2c|3990|291|

네 정본 스케치와 네 배포 ZIP을 새 임시 폴더에 풀어 컴파일했습니다(8개 빌드 PASS). ZIP·파일 manifest SHA-256 일치, 역할 밖 include 없음, 검증 중 정본 파일 변경 없음도 확인했습니다. SRAM 값은 정적 linker 측정이며 실행 스택 최대값은 측정하지 않았습니다.

## 재현과 실물 후속

```sh
python3 firmware/tools/test.py
python3 firmware/tools/build.py
python3 firmware/tools/package.py
python3 firmware/verification/partial5/run.py --build-uno --verify-packages
```

[부분 연결 실물 시험 순서](../INTEGRATION_CHECKLIST.md)를 따라 네 보드 protocol5 업로드·센서 거리 정확도·배선/파형·모터/부저 정격·실제 정지 지연을 확인해야 합니다. 이전 실물 I2C 실패 원인은 이번 소프트웨어 검증으로 확정하지 않았습니다. 0_web은 이전 구성 기록으로 이번 펌웨어와 격리돼 있습니다.
