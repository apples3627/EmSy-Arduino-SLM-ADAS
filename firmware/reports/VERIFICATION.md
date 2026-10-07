# 네 Uno 레지스터 펌웨어 검증 기록

학습 모델 ID30918170, 10의도, 라벨버전2, H64의 두 H32 shard입니다. MCU 코드는 레지스터 I/O이며 Arduino API 스캔을 수행합니다. 스케치 startup/setup/loop만 Arduino core를 사용합니다.

## 실제 실행한 호스트 검사

- 공통 프레임: CRC8 known vector, 32개 바이트 비트 반전 거절, nonzero padding 거절, 바이트 스트림 재동기화, float little-endian, 모델/범위 metadata.
- 계산 보드의 실제 portable Worker: 토큰 두 청크, 중복/변경/누락/잘못된 순서·ID·CRC, START 멱등성, all-PAD/잘못된 token, 40바이트 결과 재조합. 두 실제 shard와 전체 모델의 점수 비교 및 의도 일치.
- 제어 보드의 실제 Engine: 전체 STOP·독립 기능 ON/OFF, lease 만료·중복 재전송, 반전 ACK 지연/STOP 취소, 세션 replay/새 HELLO, CRC·길이·op·명령번호 오류, watchdog와 로컬 버튼, duration 상한.
- 메인의 실제 Scores/policy: 단일 bias, 두 worker/두 chunk, stale/duplicate/nonfinite/CRC 거절, STOP 정규화와 세대 변경, 세션/command ACK 검증.
- 실제 MAIN 레지스터 헤더를 실행하는 가짜 AVR 통합: 실제 두 Worker와 제어 Engine의 HELLO→초기 STOP ACK→READY→입력 두 chunk→START→STATUS→결과 합→ACTUATE→ACK 흐름. UART TX/RX bit sampling, TWI ISR 단계, SPI byte pipeline, 3초 계산 지연 중 STATUS, 32,769회 유휴 STATUS에서 command ID 보존, UART FE/DOR/UPE, TWI 무IRQ/STOP 고착의 bounded 복구, 늦은 결과·ACK와 STOP 취소를 검사.

검사 명령은 `python3 firmware/tools/test.py`입니다. 가짜 AVR 검사는 실제 소스의 로직을 실행하지만 실제 IRQ 지연/전기 파형을 측정하지 않습니다.

## Uno 타깃 빌드

`python3 firmware/tools/build.py`로 네 스케치를 실제 Arduino AVR core에 컴파일합니다. 측정값은 `build_sizes.json`, 원본 로그는 `firmware/build/<role>/compile.log`에 기록합니다. Flash 한도32,256B, SRAM2,048B이며 전역 SRAM≤1,536B(정적 여유≥512B)를 검사합니다. 정적 여유는 최대 스택 실측이 아닙니다.

## 남은 실물 검증

이번 요청에서는 보드를 자동 업로드하지 않았습니다. 현재 한 대에는 이전에 검증한 전체 모델 probe가 유지됩니다. 네 보드의 실제 UART/I2C/SPI 중첩, GPIO/PWM/서보/피에조 파형, 모터 드라이버·전원·기구 및 안전 정지 물리 동작은 팀원이 각자 업로드·배선한 후 `INTEGRATION_CHECKLIST.md`대로 확인해야 합니다.

## 확정 빌드 크기

|보드|Flash / 32,256B|전역 SRAM / 2,048B|정적 SRAM 여유|
|---|---:|---:|---:|
|main|13,812B|1,196B|852B|
|compute_i2c|12,902B|341B|1,707B|
|compute_spi|13,004B|341B|1,707B|
|actuator_uart|4,488B|365B|1,683B|

네 보드 모두 실제 AVR 컴파일과 호스트 테스트가 통과했다. 프로젝트 코드의 경고는 정리했으며 Arduino AVR core 1.8.6 new.cpp의 unused tag 경고는 코어에서 발생한다. 실제 최대 스택은 팀 실물 시험에서 별도 측정한다.
