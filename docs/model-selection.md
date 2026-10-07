# 모델 선택과 Uno R3 제약

2026-10-07 기준. 목표: Uno R3 한 개에서 다음 문자를 계산하고 시리얼로 짧은 영어 문장을 출력한다. PC는 학습, 컴파일, 업로드, 터미널 표시에 사용한다.

## 검토 결과

| 후보 | 저장공간 판단 | 결정 |
|---|---|---|
| HuggingFaceTB/SmolLM2-135M | 이름 기준 135M 가중치를 4비트로 저장해도 약 67.5MB. 양자화 부가 정보·토크나이저·활성값 제외 | Uno R3 내장 메모리에 탑재 불가 |
| roneneldan/TinyStories-1M | config의 어휘 50,257 × 임베딩 64만으로 3,216,448 가중치. 이 행렬 하나를 4비트로 저장해도 1,608,224바이트 | 모델 이름의 1M만 보고 Uno에 들어간다고 판단할 수 없음 |
| 프로젝트 전용 초소형 문자 신경망 | 작은 문자 집합, 짧은 고정 문맥, 학습된 작은 행렬을 Flash에 저장 | 제한된 영어 문장 생성 데모에 사용 |

Uno R3는 ATmega328P, 16MHz, Flash 32KB, SRAM 2KB다. 일반 Uno 부트로더를 사용하는 스케치의 Flash 예산은 32,256바이트다. 스케치에는 모델뿐 아니라 추론 코드, 시리얼 입출력 코드도 들어가며 SRAM은 버퍼·중간 활성값·스택에 함께 쓰인다.

## 해석 범위

선정된 데모는 학습된 신경망으로 다음 문자를 예측하는 작은 언어 모델이다. 대규모 사전학습 LLM, Transformer, SmolLM2 압축본 또는 증류본이 아니다. SmolLM2보다 품질이 좋다는 비교 주장은 하지 않는다. 제한된 자체 학습 데이터의 패턴을 보여주며 일반 지식, 자유로운 질의응답, 한국어 생성을 목표로 하지 않는다. 예시 문장이 잘 나와도 범용 성능을 입증하지 않는다.

가중치는 펌웨어에 포함되고 추론 행렬 연산은 Uno에서 수행한다. PC 터미널은 텍스트를 보내고 수신한다. 보드가 연결되지 않은 상태에서 수행한 호스트 검증과 컴파일은 실제 보드 동작·추론 속도·런타임 스택 사용량 검증과 구별해야 한다.

## 근거

- [SmolLM2 공식 모델 카드](https://huggingface.co/HuggingFaceTB/SmolLM2-135M)
- [SmolLM2 config](https://huggingface.co/HuggingFaceTB/SmolLM2-135M/blob/main/config.json)
- [TinyStories-1M 공식 모델 카드](https://huggingface.co/roneneldan/TinyStories-1M)
- [TinyStories-1M config](https://huggingface.co/roneneldan/TinyStories-1M/blob/main/config.json)
- [Arduino Uno R3 공식 데이터시트](https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf)

실제 선정 모델의 크기, 학습 설정, 검증 결과는 프로젝트의 모델 메타데이터와 검증 보고서를 참조한다.
