# I2C 완료 처리 및 실물 진단

## I2C 개별 진단 — 2026-10-13

MAIN·SLM_UART 수정본을 재업로드한 뒤 SLM 콘솔에 `!i2c`를 보냅니다. 출력은 MAIN의 마지막 완료된 I2C 시도이며 자동재시도 중에도 유지됩니다. 새로운 sideband 정보는 READY/명령/epoch를 변경하지 않습니다.

```text
!i2c
I2C twsr=... error=... req=... reply=... version=... len=... ok=... fail=... meta=... flags=... since_ms=...
```

TWSR은 십진수입니다:32(0x20)은 쓰기주소 NACK,72(0x48)은 읽기주소 NACK,88(0x58)은 마지막 읽기바이트 완료입니다. error1=쓰기주소응답없음,2=데이터NACK,3=읽기주소응답없음,4=예상밖하드웨어상태,5=응답CRC/프레임/버전불일치,6=진행timeout,7=STOPtimeout,8=metadata/status정책불일치입니다. error0은드라이버성공이고 version4/reply15/len8은ADAS HELLO metadata 형태입니다. meta1은MAIN metadata검증통과입니다. version2/3이면이전펌웨어일수있습니다. since_ms가2000보다크면새로운로그를기다립니다.

코드수정: 이미완료된read STOP뒤의불필요한추가STOP을제거하고CRC결과를바로완료로게시합니다. 오류·NACK·시간초과·취소는기존유한STOP복구를유지합니다. timeout의32비트시간값은안전하게읽은service now를사용합니다. 실물실패의원인이중복STOP이라고확정하지는않습니다.

USB전원을각보드에넣는경우보드끼리5V선을묶지않고GND만공통으로연결합니다. MAIN A4→ADAS A4/A5→A5 및SPI보드의로컬LCD I2C와분리된배선을확인합니다. 현재내부풀업/약30.4kHz설정은유지합니다.

검증: 독립78개검사(45 FSM/21 드라이버진단/12 observer권한검사)와 새6개원인/시간회귀, 기존전체호스트회귀, 네Uno컴파일, ZIP4개hash/standalone include 검사를 통과했습니다. 별도부트로더업로드·실물파형·주소응답측정은 수행하지 않았습니다.
