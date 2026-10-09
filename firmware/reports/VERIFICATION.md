# 네 Uno 레지스터 펌웨어 검증 기록

학습 모델 ID95616d56, 10의도, 라벨버전3, H64의 두 H32 shard입니다. MCU 코드는 레지스터 I/O이며 Arduino API 스캔을 수행합니다. 스케치 startup/setup/loop만 Arduino core를 사용합니다.

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

## V4 다중 명령 검사

전체192 raw ASCII 바이트·공백 제거 후 각 절96바이트/16토큰·4절 한도다. 명시적인 각 절은 학습된 NN으로 분류하고 모든 절의 결과를 검증한 뒤 ACTUATE_BATCH 한 프레임으로 적용한다. 후속 절 UNKNOWN/통신 실패·상충에서는 접두사 실행이 없다. STOP 우선·중복 제거·반전 대기 일괄 commit/취소 및 기존 와이퍼 각도 유지가 실제 엔진 호스트 테스트에서 통과했다.

독립 검사와 이전 주석 정정 기록은 intent/MULTI_VERIFICATION.md를 따른다. 네 MCU 물리 동작은 미검증이다.

## 확정 빌드 크기

|보드|Flash / 32,256B|전역 SRAM / 2,048B|정적 SRAM 여유|
|---|---:|---:|---:|
|main|15,902B|1,360B|688B|
|compute_i2c|12,858B|341B|1,707B|
|compute_spi|12,960B|341B|1,707B|
|actuator_uart|4,954B|411B|1,637B|

STOP 절이 인식되면 남은 절의 추론을 기다리지 않고 전체 정지한다. 뒤 절의 연산 보드가 응답하지 않아도 이미 켜진 경적·와이퍼를 오래 유지하지 않도록 실제 메인 레지스터 호스트 회귀시험을 추가했다.

정적 RAM 여유는 최대 stack의 실물 측정값이 아니다. 현재 USB Uno 포트가 없어 V4 업로드·물리 시험은 수행하지 않았다.
