PASS — 독립 호스트 검증과 네 Arduino Uno R3 빌드를 통과했습니다. 실제 네 보드 업로드·센서 계측·버스 동시 파형 측정은 수행하지 않았습니다.

검증 대상은 공통 프로토콜 `fa02378be1a37cb6609d389e6be43f1059b4d81a`, SLM/ADAS 노드 `c0964a706e75aa6e0dc8bd3213ec66518ac262b3`, MAIN/ACT `7ce9f0e`와 부팅 경합 수정 `b946206abeab1b3ac01a76d6cf897d1136a183a7`입니다. 검증 작업트리의 cherry-pick 통합 커밋은 `5725656`이며, 모델 ID는 `0x95616d56`, full 모델 파일 SHA256은 `930b0afacc6842799c52b2ca41b7f78504f9c05ef520ca9790522f47d3424f01`입니다. 모델 재학습은 없습니다.

구현 테스트 전에 독립적으로 선택한 44개 요구사항을 `scenarios.json`에 기록하고 SHA256 `26a6f7017cf4561963971bb633317833fd65895cb05afb70d27744d3f0e96c0c`로 봉인했습니다. 이는 요구사항 목록이며, 현장 수집 데이터나 44개의 물리 실험을 뜻하지 않습니다. 실행 결과와 재현 명령은 `firmware/verification/adas/results.json` 및 `run.py`에 있습니다.

- 실제 정책 코드: 19.9/20/20.1/24.9/25cm 경계, 2회/3회 clear, 중간 invalid, stale 200/201ms, uint32 시간 wrap, 물리 측정 범위 경계, 두 독립 source의 OR 결합을 검증했습니다.
- 실제 ADAS ISR/loop: Timer1 counter wrap와 pending overflow, 10µs trigger, unarmed edge, 29/30ms echo timeout, open-drain DDR/PORT, invalid range의 VALID 해제와 65535 보고, 상태 snapshot, 읽는 중 불변 응답 buffer를 검증했습니다.
- 실제 SLM Worker: 10개 고정 문장의 100개 점수를 ADAS 변경 전 full C++ kernel의 봉인된 기준과 비교했습니다. 오차 허용값은 1e-5입니다. full H64 metadata와 bias 포함 표식, 과거 H32/protocol2/labels2 거부, computing 중 snapshot 수신이 neural job을 덮어쓰지 않는 동작과 변경된 retry payload 거부를 검증했습니다.
- 실제 ACT ISR와 hardwareOutputs: 위험 LOW에서 PWM/bridge/lamp/servo/horn 레지스터가 즉시 OFF, 전경 루프가 지연된 LOW/HIGH pulse 뒤에도 OFF 유지, UART session 유지, 같은 epoch STOP의 sync 해제 거부, 새 epoch STOP 후 새 명령만 허용, reversal 취소, 500ms lease와 부속 출력 보존을 검증했습니다.
- 실제 MAIN 코드: 요청 drive=0의 즉시 게시와 적용 확인 전 unknown, 늦은 과거 epoch ACK 거부, 두 노드로 STOP snapshot 전달, full SLM 점수에 bias를 추가하지 않는 조립, 물리 LOW/오래된 telemetry의 차단을 검증했습니다.
- 실제 register scheduler + 별도 translation unit의 SLM/ADAS/ACT 엔진을 함께 실행했습니다. 1~4개 clause 각각에 초기/부분 결과/짧은 위험 pulse 일정을 주는 12개 중단 시험에서 이전 세대의 명령이 실행되지 않았고, 해제 후 이전 drive가 자동 복구되지 않았습니다. 이 시간·버스 shim은 실제 전기적 동시 버스 파형의 증거가 아닙니다.

독립 시험에서 ADAS 정상 상태가 ACT HELLO ACK보다 먼저 도착할 때 MAIN이 대기 중 HELLO를 삭제하는 부팅 경합을 발견했습니다. 구현 담당이 `b946206`에서 HELLO를 보존하도록 수정했고, 실제 peer 엔진 스케줄러로 재시험을 통과했습니다.

| Uno 역할 | Flash bytes | 정적 SRAM bytes | 정적 SRAM 여유 bytes |
|---|---:|---:|---:|
| MAIN | 18582 | 1459 | 589 |
| SLM SPI | 14948 | 369 | 1679 |
| ADAS I2C | 3984 | 291 | 1757 |
| ACT UART | 5858 | 421 | 1627 |

네 역할 모두 flash 32256 이하, 정적 SRAM 1536 이하 기준을 통과했습니다. 정적 여유는 linker 결과이며 런타임 stack 최대 사용량을 측정한 값이 아닙니다. 호스트 AVR shim은 실제 ISR와 출력 함수의 레지스터 값을 점검하지만 인터럽트 전기 지연·센서의 실제 오차·기계적 정지 거리·실물 동작을 입증하지 않습니다.

재현: `python3 firmware/verification/adas/run.py --build-uno` (Arduino CLI와 Uno AVR core 필요, 업로드 없음). 공통 프로토콜 테스트는 검증 작업트리의 canonical header를 사용하며, 다른 실제 역할 header는 `--source-root`로 지정할 수 있습니다.

## 최종 배포 재검증 — 2026-10-10

원본 독립 검증 이후 루트 통합 소스 `0ee9d30`에서 전체 호스트 검증8개, 구현 담당 MAIN/ACT 레지스터 회귀2개(지연 HELLO ACK 포함), Python full-score parity33개를 다시 통과했습니다. 공통 헤더의 오래된 protocol2 설명을 protocol3으로 고쳤고 역할별 가이드·핀 표를 정리했습니다. `results.json`은 이 최종 루트 소스의 독립 재실행 및 네 Uno 빌드를 기록합니다.

최종 팀 ZIP4개는 manifest의 archive/file SHA256을 전부 확인하고 임시 폴더에 풀어 각각 독립 Uno 컴파일을 통과했습니다. 기록은 `package_verification.json`입니다. 루트 빌드의 MAIN flash는18,696 / 정적 SRAM1,459(여유589)바이트이며 나머지 세 보드는 위 표와 같습니다. 서로 다른 경로에서 나온 원본 검증 크기는 원본 기록으로 유지하며 배포별 정확한 수치는 build_sizes.json과 package_verification.json을 사용합니다. 실제 보드 업로드는 수행하지 않았습니다.
