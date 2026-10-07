# Uno용 영어 의도 신경망 - 실제 학습 모델

영어 문장의 의도를 분류하는 **학습된 신경망**입니다. 단어 임베딩16차원 → 평균 풀링 → ReLU64뉴런 → 10개 의도 점수 구조이며, 생성형 LLM이나 범용 자연어 이해 모델은 아닙니다. PC에서 학습했고 Uno용 int8 가중치와 공유 C++ 추론 코드를 내보냈습니다. 보드는 어휘·가중치·산술을 모두 내부에 보관하여 PC 추론 없이 실행할 수 있습니다.

이번 구현은 **의도 출력 probe**입니다. 모터·조명·서보·피에조를 구동하는 4보드 통신 펌웨어를 구현하거나 실제 보드에 업로드하지 않았습니다. H32씩 분할된 계산 모델은 내보냈지만 I2C/SPI 프로토콜은 앞으로 연결해야 합니다.

## 지금 PC에서 시험

저장소 루트에서 NumPy2.3.4가 설치된 Python3를 사용하세요.

```sh
python3 intent/tools/infer.py 'please drive ahead'
python3 intent/tools/infer.py 'stop the wipers'
python3 intent/tools/infer.py 'turn the buzzer on'
python3 intent/tools/infer.py 'stop everything'
printf '%s\n' 'honk' 'silence the buzzer' 'do not move forward' | python3 intent/tools/infer.py
./intent/tools/test.sh
./intent/tools/build.sh
```

`infer.py`는 **PC 검증 전용**이며 raw_label, effective_label, 점수, confidence, margin, 거절 이유를 JSON으로 반환합니다. 실제 배포에서 PC가 이 결과를 보드로 보내는 방식은 사용하지 않습니다. 독립 감사용 호출 API는 `intent/reference.py`의 `infer(text)`, `normalize(text)`, `prepare(text, vocabulary)`입니다.

## Uno에서 실행

Arduino IDE로 `intent/uno_probe/uno_probe.ino`를 열어 Arduino Uno를 선택하세요. `src/` 폴더가 함께 있어야 합니다. 9600 baud, Newline 시리얼 모니터에서 영어 입력을 보내면 RAW와 EFFECTIVE 의도를 출력합니다. 이 스케치는 출력 핀을 구동하지 않습니다.

기본 빌드는 전체 모델입니다. `build.sh`는 INTENT_PART=0(전체), 1(I2C shard), 2(SPI shard)의 세 스케치를 컴파일합니다. shard probe는 부분 로짓10개만 출력합니다. 실제 업로드는 연결된 보드와 포트를 확인한 뒤 별도로 진행하세요.

선택적 `python3 intent/tools/serial_client.py PORT`는 pyserial이 필요합니다. 이 클라이언트는 입력·출력만 전달하며 모델을 읽거나 계산하지 않습니다.

## 입력과 거절 규칙

최대96 ASCII bytes, 최대16단어입니다. 대문자를 소문자로 바꾸고 공백을 압축하며 문장 끝의 .!?를 제거합니다. 비ASCII, NUL, 제어문자, apostrophe, 내부 문장부호, 과도한 길이·단어 수는 거절합니다. 잘라서 실행하지 않습니다. 학습 TRAIN에서 만든 **247단어 어휘** 밖의 단어는 OOV로 거절합니다. 단어 조합과 순서는 다양하게 입력할 수 있지만 처음 보는 동의어까지 이해한다는 보장은 없습니다.

단어별 학습 임베딩의 평균을 사용하므로 단어 순서를 구별하지 못합니다. `please drive forward`와 `drive forward please`처럼 같은 의도의 어순 변형에는 도움이 되지만 복잡한 문법·부정 범위를 해석할 수 없습니다.

따라서 not/no/never, and/or/then, if/when/unless/whether, 설명·질문을 나타내는 일부 단어에 **일반적인 보수적 거절 guard**를 별도로 둡니다. while/before/after/until/neither/nor/also/plus 등도 일반 guard로 거절하며 어휘 밖 단어는 OOV로 거절됩니다. 모든 guard 단어 목록은 `reference.py`에 공개되어 있습니다. 이 guard는 행동별 키워드→의도 매핑이 아닙니다. 점수로 다음 의도를 학습하는 부분과 별개이며 raw 신경망 성능과 guarded 성능을 각각 보고합니다. 세미콜론 등 복합 구두점은 입력 단계에서 거절됩니다.

모델의 UNKNOWN, guard, OOV, 잘못된 입력, 낮은 confidence/margin, 비정상 float 결과는 모두 **유효 의도 UNKNOWN / GLOBAL_STOP** 정책입니다. validation만 사용하여 고른 confidence0.9, margin0.2는 합성 데이터에 대한 기준이며 실제 확률이나 안전성을 보증하지 않습니다.

## 고정 라벨과 전체 정지 계약

|ID|의도|계약|
|0 / 1|FORWARD / REVERSE|앞 / 뒤 구동 의도|
|2|STOP|모터 정지 + 조명OFF + 와이퍼OFF + hornOFF|
|3 / 4|LIGHT_ON / LIGHT_OFF|조명만 변경|
|5 / 6|WIPER_ON / WIPER_OFF|와이퍼만 변경; WIPER_OFF는 전체STOP 아님|
|7|UNKNOWN|불확실한 동작을 실행하지 않고 전체STOP|
|8 / 9|HORN_ON / HORN_OFF|피에조 tone ON latch / noTone OFF|

`stop the wiper`, `stop the horn`을 해당 기능만 정지하는 의도로 학습했습니다. `stop`, `halt`, `stop everything`, `all off`는 전체STOP입니다. 신경망 성능을 가리는 exact-stop shortcut은 현재 probe에 넣지 않았습니다. `Intent::applyPolicy`는 전체STOP 상태 계약을 검증하는 순수 소프트웨어 함수이며 실제 GPIO/tone 호출은 아닙니다.

향후 제어 Uno의 피에조 D6는 잠정안입니다. TB6612의 STBY가 필요하면 D12/A0 등 별도 빈 핀으로 재배치합니다. tone은 Timer2를 사용하므로 모터PWM D3/D11은 피하고 D5를 유지합니다. Servo의 Timer1은 제어 Uno에 있고 메인의 softwareUART Timer1은 다른 보드에 있습니다. 부품·기구·전원은 미정입니다.

## 데이터·학습·재현

`dataset/generate.py`의 직접 작성한 행동 표현과 template를 조합한 합성 영어41121문장입니다. 외부 API·모델·데이터셋을 사용하지 않았습니다. train31202, validation4251, test5668입니다. 전체 template family를 분리했고 정규화된 동일 문장을 전역 중복 제거했습니다. **행동 표현과 알려진 어휘는 split 간 공유**하므로 새 어휘나 실제 사용자 문장에 대한 일반화 평가로 해석하면 안 됩니다.

V1의 독립 감사에서 OOV·낮은 coverage·cross-function 오분류가 확인되어 V2를 학습했습니다. V2 fresh83문장에서는 정확도84.34%, positive63개 중50개(79.37%)만 올바르게 수락했고 UNKNOWN20개는 모두 거절했습니다. 12개 OOV와1개 낮은 확신이 한계였습니다. 잘못 수락한 행동은0개였습니다. 이러한 **범주 요약만** 받고 감사 원문을 읽지 않은 채, 공통 방향·차량·전체시스템·부품·활성화상태·요청 연결어를 넓힌 bounded V3를 학습했습니다. 기존 감사는 이후 회귀 평가이며 새 V3 독립 감사와 구분합니다.

V1/V2의 hash·측정 결과는 experiments/v1.json, experiments/v2.json에 보존했습니다. V2의 초기 would-like guard 오류와 수정 결과도 보존했습니다. V3는 제한된 i/we would like you to, would you like to 요청 prefix만 like를 허용합니다. once는 at once 요청 표현일 때만 허용하며 비한정 지속시간 요구는 보수적으로 거절합니다.

V3 synthetic TEST manifest는 학습 전에 고정했습니다. 고정seed20261007, full-batch Adam1600단계로 학습하고 validation cross-entropy 최소 checkpoint를 선택했습니다. int8 threshold/margin은 validation에서만 선택했습니다. VAL action coverage99.8409%, 잘못 수락한 동작0개였습니다. 이후 V3 synthetic TEST를 한 번 평가했으며, 별도의 새로운 봉인 감사 사례는 trainer가 읽지 않았습니다. 모델·데이터·정책은 이 V3 평가 후 동결합니다.

```sh
python3 intent/dataset/generate.py
OPENBLAS_NUM_THREADS=1 python3 intent/train.py
python3 intent/tools/export.py
./intent/tools/test.sh
./intent/tools/build.sh
```

위 명령은 기존 모델을 덮어쓰는 재현용입니다. TEST 결과를 보고 seed·구조·임계값을 바꿔 선택하지 마세요. 새로운 실험의 모델 선택에는 TRAIN/VAL만 사용하고, 이미 공개된 TEST는 후속 회귀시험으로 표시해야 합니다.

`model_float.npz`, `model_int8.npz`, `metadata.json`, `training_log.json`, `dataset/manifest.json`에 실제 가중치·설정·hash·선택 시점·실행 시간을 저장했습니다. 수동 역전파의 finite-difference 검사 결과는 metrics.json에 저장했습니다. V1 초기 보고서가 마지막 학습 gradient를 잘못 출력한 문제를 정정했고 학습 알고리즘이나 V1 가중치에는 영향이 없었습니다.

## V3 합성 TEST 결과와 제한

|모델·정책|macro-F1|action coverage|수락 동작 정확도|잘못 수락한 동작|
|float 신경망 raw, 첫 평가|1.000|100%|100%|0|
|int8 신경망 raw, 첫 평가|1.000|100%|100%|0|
|int8 + guard/임계값, 첫 평가|0.99866|99.8409%|100%|0|

5668개 합성 TEST에서 STOP/HORN_ON recall은99.2857%, 나머지 라벨은100%입니다. 일부 의미상정지·시간한정 문장을 보수적인 guard가 거절했고, UNKNOWN은 전체정지 정책으로 처리합니다. 전체 confusion matrix와 지표는 metrics.json에 저장되어 있습니다. V3의 새로운 독립 감사 결과는 주 스레드에서 별도로 제공합니다.

이 결과는 공유 어휘·행동 표현의 다른 template군에 한정됩니다. 합성 점수100%를 실제 사용자 정확도100%로 해석하지 마세요. 새로운 동의어·복잡한 문법·모호한 조합·도메인 밖 표현은 보장하지 않습니다. 점수는 자동차의 물리적 정확도나 안전성 지표가 아닙니다.

## export·메모리·검증

int8 가중치는 e/w/o별 원래 tensor scale 하나씩 사용합니다. H32+H32 shard는 이미 양자화된 배열을 자르고 원래 scale을 유지하며 출력 bias는 메인에서 한 번만 더합니다. `src/generated/`의 vocabulary/label/model ID가 모두 맞아야 합니다. 모델ID는 weight hash·vocabulary hash·고정 라벨에서 계산합니다.

전송 인터페이스는16개의 uint16 little-endian 단어ID, 총32 B입니다. ID0~246는 어휘, PAD0xFFFF는 빈 칸이며 pooling에서 무시합니다. 메인이 PROGMEM 사전으로 토큰화하여 같은 token block을 I2C/SPI로 보내야 합니다. 이번 probe는 사전을 포함하여 한 보드에서 입력→추론을 확인합니다. 실제 4보드에서는 수신 token의 범위·개수·PAD를 검증해야 합니다.

실제 Uno 타깃 컴파일 결과:

|probe|Flash / 32256 B|전역SRAM / 2048 B|남은SRAM|
|전체|14340 B|413 B|1635 B|
|I2C shard|12620 B|425 B|1623 B|
|SPI shard|12620 B|425 B|1623 B|

모델의 가중치·bias·scale은5924 B입니다. 사전·guard·출력 문자열·코드는 별도입니다. 고정 버퍼와 PROGMEM을 사용하며 동적 할당은 없습니다. 남은SRAM은 링크 시 수치로 실물 최대 스택을 측정한 보장이 아닙니다.

1366개 TRAIN/입력 계약 문맥에서 C++와 Python scalar float32 점수를 비교해 최대오차5.00e-8, 두 실제 export shard 합과 전체 모델의 최대오차1.17e-5를 확인했습니다. selected token/intent도 일치했습니다. 비ASCII·NUL·초과입력·OOV 거절, family 누수·중복, 전체STOP/UNKNOWN의 모든 플래그OFF, horn latch와 NaN 거절을 검사했습니다. 해당 parity 시험은 TEST·독립 감사 입력을 모델 선택에 쓰지 않습니다.

## 연결된 Uno 한 대의 시리얼 수락 시험

전체 모델 `INTENT_PART=0` probe를 Uno에 올린 뒤 시리얼 모니터를 닫고 실행합니다. pyserial과 NumPy가 필요합니다.

```sh
python3 intent/tools/test_hardware.py --port /dev/cu.usbmodem11301 --repeat 2 --timeout 15 --output intent/reports/single_uno_hardware.json
python3 intent/tools/test_hardware_unit.py
```

PC는 원문 UTF-8 bytes와 줄바꿈만 보냅니다. Uno의 `RAW / EFFECTIVE / REASON` 답을 실제로 읽은 뒤, PC reference는 비교용 예상값만 계산합니다. 응답이 없거나 잘못되면 PC 답으로 대체하지 않고 실패 보고서를 남기며 종료 코드1을 반환합니다. 시작 배너를 기다리고 요청마다 `READY`를 확인하므로 한 번에 한 문장만 전송합니다. CRLF의 CR은 probe가 제거하는 전송 framing이며 의미 입력에 포함하지 않습니다.

고정된 V3 감사51문장을 재사용한 회귀시험과 수동 지정한10개 라벨·대소문자·공백·구두점·96/97bytes·16/17단어·비ASCII·NUL·빈 입력·OOV를 반복합니다. 모든 입력 파서 거절 직후에는 정상 명령으로 복구를 확인합니다. JSON에는 실제 보드 원문 로그·전송bytes·모델hash·CPU parity와 별도의 의미 정답·confusion·UART 왕복 p50/p95를 저장합니다. 이미 공개된 알려진 어휘 사례의 수락 시험이며 새로운 실제 문장 정확도의 보장은 아닙니다. probe가 점수를 출력하지 않으므로 AVR float 로짓의 bitwise parity를 주장하지 않습니다. 이 시험은 출력 핀이나 실제 모터·조명·와이퍼·horn을 구동하지 않습니다.
