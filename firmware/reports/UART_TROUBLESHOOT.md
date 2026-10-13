# 소프트웨어 UART 및 세션 복구 점검

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

## 검증 기록

기존33문장 scalar모델parity(오차0), 역할별회귀, 독립11개host검증, 새저장세션42/MAIN7→SID43 복구/EEPROM/모터게이트/낡은hint/진단/링버퍼회귀를 통과했습니다. 추가독립감사에서8192개의링위치/점유/소비인터리빙과208개TCNT1 half-phase를 확인하고 AVR ELF의 송신copy에cli가없고 수신계산에일반나눗셈이없음을 확인했습니다. 리뷰된일반runtime critical section은5µs미만(보수적추정)이며 TWI ISR의정상바이트처리는약15µs입니다. 이는클럭드리프트·전기파형·실물지터를측정한결과가 아닙니다. 실제로그의source8 원인은새DIAG를받기전에는확정하지않습니다.

진단은정보만저장하며 SLM의READY/epoch/세션인증이나출력허가를바꾸지않는회귀도추가했습니다. 부트로더/핀/baud를변경하지않고9600/D2-D3구성을유지합니다. 모터전원을끈상태에서MAIN·SLM_UART·PERIPHERAL_SPI 세역할을재업로드하고 `!status`, `!diag` 결과를확인하세요.
