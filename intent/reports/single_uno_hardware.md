# Uno R3 한 개 실제 추론 예비 시험

결과: **PASS - 184/184 요청 통과**. 모델 `30918170`를 실제 Uno R3에 업로드하고 Flash 검증 옵션으로 확인했다. 4보드 연결 이전의 단일 연산 보드 시험이다.

## 시험 구성

- 보드: USB에서 확인한 Arduino Uno, VID 2341 / PID 0043.
- 포트: `/dev/cu.usbmodem11301`, 9600 baud, 전체 모델 `INTENT_PART=0`.
- 보드 프로그램: `intent/uno_probe/uno_probe.ino`, Flash 14,340 B, 전역 SRAM 413 B.
- PC가 원문 UTF-8 바이트와 줄바꿈만 전송하고, Uno가 어휘 처리·신경망 추론·거절 판단 후 응답했다.
- PC 기준 모델은 수신 결과와 비교하는 데만 사용했다. 응답 누락 시 PC 결과로 대체하지 않는다.
- 92 입력/복구 사례를 2회 반복했다. 기존 봉인된 51개 문장의 재사용 회귀시험, 10개 의도 기본 예시, 대문자·공백·끝 문장부호·CRLF, 96/97 바이트·16/17 단어 경계, 비ASCII·NUL·빈 줄·OOV 등의 오류 및 즉시 정상 입력 복구를 포함한다.
- 시작·종료 시각은 JSON의 UTC 타임스탬프에 기록했다.

## 실제 결과

|항목|결과|
|완전한 시리얼 응답|184/184|
|PC 기준 RAW/EFFECTIVE/REASON 일치|184/184|
|사람이 정한 의도/거절 결과 일치|184/184|
|오류 직후 정상 입력 복구|32/32|
|타임아웃|0|
|왕복 시간 중앙값 p50|131.44 ms|
|왕복 시간 p95|158.35 ms|
|왕복 시간 최소 / 최대|22.21 / 207.44 ms|

왕복 시간은 PC 쓰기 시작부터 Uno의 최종 READY 수신까지이다. 입력 길이·USB/UART·신경망 연산·응답 출력이 모두 포함된다. 오류 입력은 추론 전에 거절될 수 있어 전체 시간 분포에 빠른 거절 응답도 포함된다. 순수 MCU 연산 시간으로 해석하지 않는다.

## 대표 실제 응답

```text
please drive ahead
RAW FORWARD EFFECTIVE FORWARD REASON 0

turn the buzzer on
RAW HORN_ON EFFECTIVE HORN_ON REASON 0

stop everything
RAW STOP EFFECTIVE STOP REASON 0
```

STOP은 이후 제어 펌웨어에서 주행·조명·와이퍼·피에조를 모두 끄는 계약이다. 이번 스케치는 의도만 출력하며 액추에이터 핀을 구동하지 않는다.

## 범위와 후속 단계

실제 Uno의 의도 및 사유 출력이 검증됐다. probe는 AVR 로짓을 출력하지 않으므로 부동소수점 점수의 비트 일치나 순수 연산 시간은 측정하지 않았다. 재사용 평가 문장과 계약 테스트에 대한 결과이며, 모든 영어 문장의 정확도 100%를 뜻하지 않는다. I2C/SPI로 분할한 두 연산 보드, D2/D3 제어 UART, 모터·LED·서보·피에조의 물리 동작은 별도 단계다.

실행 및 재시험:

```sh
python3 intent/tools/serial_client.py /dev/cu.usbmodem11301
python3 intent/tools/test_hardware.py --port /dev/cu.usbmodem11301 --repeat 2 --output intent/reports/single_uno_hardware.json
```

두 프로그램과 Arduino IDE 시리얼 모니터는 같은 포트를 동시에 열지 않는다. 포트를 다시 연결하면 이름을 확인한다. 원본 바이트·Uno 수신 줄·PC 예상 결과·각 요청 시간·Flash 이미지 hash는 [실제 시험 JSON](single_uno_hardware.json)에 보관한다.
