# 현재 프로젝트: 영어 명령 모델과 자동차 제어

학습된 10의도 모델과 Uno 추론 스케치는 [intent 실행 안내](intent/README.md)에 있습니다. 크락션 ON/OFF를 포함하며 STOP은 주행·조명·와이퍼·크락션을 모두 끄는 계약입니다.

[4개 Uno 레지스터 펌웨어·팀 업로드 가이드](firmware/README.md) · [전체 핀 연결표](firmware/PIN_MAP.md)

[독립 검증 결과](intent/INDEPENDENT_VERIFICATION.md) · [최신 계획서](output/pdf/arduino_language_model_project_plan.pdf)

아래는 프로젝트 출발점인 기존 문자 생성 데모의 기록입니다.

---

# Arduino Uno R3 단독 짧은 문장 생성 데모

Uno R3가 작은 **학습된 신경망**의 다음 문자 예측과 자동 회귀 생성을 직접 수행합니다. PC는 학습·업로드 및 시리얼 입출력에 사용하며, 실행 중 추론 계산에는 필요하지 않습니다. 모니터 없이도 보드는 계산하지만 생성된 글을 읽으려면 시리얼 수신 장치가 필요합니다.

이 모델은 SmolLM2나 범용 LLM이 아닙니다. Uno의 Flash 32 KB, SRAM 2 KB에 맞춘 교육용 문자 언어 모델입니다. 직접 작성한 17줄 영어 문장만 학습하여 해당 표현을 재현하며, 질문 답변·한국어·일반 지식·범용 대화 능력은 없습니다. 학습 문장 밖의 입력은 무의미하거나 반복적인 결과가 나올 수 있습니다. [모델 선택 근거](docs/model-selection.md)도 참고하세요.

실제 내보낸 모델의 호스트 검증 예시:

```text
hello → hello friend. we can learn together.
the cat → the cat is small. the cat likes milk.
a robot → a robot can help you.
```

## Arduino IDE에서 사용

1. `tiny_uno/tiny_uno.ino`를 Arduino IDE로 엽니다. 같은 폴더의 두 헤더도 필요합니다.
2. 보드를 **Arduino Uno**, 연결된 실제 포트를 선택하고 업로드합니다.
3. 시리얼 모니터를 **9600 baud**, 줄 끝을 **Newline** 또는 **Both NL & CR**로 설정합니다.
4. `READY` 이후 `hello`, `the cat`, `a robot` 중 하나를 보내세요. `TEXT ` 줄에 입력을 포함한 생성 결과가 표시되고 다시 `READY`가 나타납니다.

보드가 연결되지 않은 상태에서 컴파일 및 호스트 검증만 수행했습니다. 업로드·실물 속도·실물 출력은 아직 확인하지 않았습니다. 추론 중 내장 LED가 켜지고 종료하면 꺼집니다. Uno의 부동 소수점 연산은 느리므로 생성 완료와 `READY`를 기다린 뒤 다음 입력을 보내세요.

## 명령행 빌드 및 검증

필요 도구: Python 3, NumPy, C++ 컴파일러, Arduino CLI 및 `arduino:avr` 코어. 현재 macOS Arduino IDE 내장 CLI 경로를 기본 사용합니다. 다른 환경에서는 `ARDUINO_CLI=/path/to/arduino-cli`를 지정하세요.

```sh
cd /Users/jiseonpark/Desktop/EmSy/LLM
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/python tools/verify.py
./tools/test.sh
./tools/build.sh
```

`test.sh`는 `python3`를 사용하므로 가상환경을 사용한다면 먼저 `source .venv/bin/activate` 하세요. `build.sh`는 컴파일만 수행합니다. 업로드는 IDE에서 실제 보드와 포트를 확인한 뒤 실행하세요. 다른 OS의 기본 CLI는 PATH의 `arduino-cli`입니다.

선택적인 터미널 클라이언트는 프롬프트만 보내고 보드 출력을 표시합니다. 이 파일에는 모델 가중치나 추론 코드가 없습니다.

```sh
.venv/bin/python tools/serial_client.py /dev/cu.usbmodemXXXX
```

실제 포트 이름으로 바꾸세요. 시리얼 모니터와 클라이언트를 동시에 열지 마세요.

## 모델 및 재학습

구조는 문자 29개 → 최근 16문자의 4차원 임베딩 → 평탄화된 64개 입력 → ReLU 은닉층 48개 → 다음 문자 로짓 29개입니다. softmax는 학습에만 사용합니다. 보드는 가장 큰 로짓을 고르는 greedy 방식으로 최대 80개의 새 문자를 생성하며, 줄 끝 토큰을 예측하면 종료합니다.

`corpus.txt`가 실제 학습 입력입니다. 소문자 영어 `a-z`, 공백, 마침표, 줄바꿈만 허용합니다. `tools/train.py`는 NumPy로 역전파와 Adam 학습을 수행하며, 고정 seed로 1800단계 학습합니다. 외부 모델·데이터셋·API를 사용하지 않습니다.

```sh
OPENBLAS_NUM_THREADS=1 .venv/bin/python tools/train.py
.venv/bin/python tools/verify.py
./tools/build.sh
```

재학습은 `model.npz`(Python 검증용), `model_metadata.json`, `tiny_uno/model_weights.h`(보드용)를 갱신합니다. `--corpus path`와 `--steps N`도 지원합니다. 재현에는 동일 NumPy 버전과 BLAS 환경을 권장합니다. `model_metadata.json`의 정확도와 loss는 학습 데이터 지표이며, 별도 평가 데이터에서 일반화 성능을 측정한 값이 아닙니다.

임베딩과 두 선형층의 가중치는 각 텐서의 최대 절댓값으로 대칭 int8 양자화합니다. bias와 scale은 float32입니다. 가중치와 bias는 `PROGMEM`에 저장되고 보드가 Flash에서 읽습니다. 연산 중 복원된 float 값을 사용하므로 순수 정수 추론은 아닙니다. 펌웨어는 동적 메모리 할당을 사용하지 않습니다.

## 시리얼 규약과 메모리

입력은 최대 63바이트의 한 줄입니다. CR은 무시하고 LF가 생성을 시작합니다. 대문자는 소문자로 바꾸고 어휘 밖의 각 바이트는 공백으로 바꿉니다. UTF-8 한국어는 바이트마다 공백이 되므로 영어 ASCII 입력을 사용하세요. 너무 긴 줄은 다음 LF까지 버린 뒤 `ERROR prompt too long; max 63 bytes`, `READY`를 출력합니다. 빈 줄도 허용됩니다. 생성 중 추가 입력을 연속 전송하면 작은 UART 버퍼가 넘칠 수 있으므로 `READY`를 기다리세요.

출력은 시작 안내, `READY`, 생성 시 `TEXT <정규화된 프롬프트+생성문>`, `READY` 순서입니다. 결과의 종료 토큰은 출력하지 않습니다. 최대 길이에 도달하면 문장이 중간에 끝날 수 있습니다.

측정한 Uno 컴파일 결과는 Flash **8614 / 32256 bytes**, 전역 SRAM **462 / 2048 bytes**입니다. 남은 **1586 bytes**는 스택과 실행 시 여유 공간이며, 실물의 최대 스택 사용량을 측정한 값은 아닙니다. 프롬프트 64바이트, 문맥 16바이트, 은닉 작업 공간 192바이트를 고정 사용하며, 로짓 배열·임베딩 배열을 SRAM에 별도로 만들지 않습니다. 상세한 검증 범위는 [검증 기록](VERIFICATION.md)에 있습니다.
