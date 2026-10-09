# 다중 명령 독립 검증

고정된 V4 모델(식별자 `95616d56`)은 봉인된 정상 명령 105개에서 모두 정확한 행동 목록과 입력 순서를 냈다. 정정된 전체 평가에서는 143/143 사례의 계획이 일치했고, 실행 범위를 벗어나는 행동은 0개였다. 처음 평가와 이후 주석 정정은 모두 보존했다.

## 봉인과 독립성

144개 사례를 새 모델/소스 접근 전에 직접 작성했다. 봉인 커밋은 `a4bdd75`, SHA-256은 `08387e2d36e6e75248ed8ed6d9b3d6b76a51b7ebb0500caa3656712511f410bc`이다. 최초 고정 모델은 원본 커밋 `3db2494`, 계획 문법 `7dfffe0`, Python/C++ 정책 정렬 `7bf05ac`이다. 통합 펌웨어 검증 대상은 `0c5dbbf`와 servo 상태 보존 수정 `7261bed`이며, 새 모델과 protocol ID를 검증 checkout에 동기화했다. 평가 전 학습자와 펌웨어 작업자에게 사례 문장을 공개하지 않았다.

학습 데이터와 정규화한 문장 전체가 겹치지 않는 사례는 115개이며, 그 중 행동 평가 대상은 114개다. 정정 후 이 집합의 정확도는 114/114, 정상 사례 정확도와 수락률은 77/77이었다. 전체 문장 중 29개, 절 262개 중 224개는 학습 데이터와 겹친다. 이 시험은 알려진 단어와 명시적인 절을 조합하는 능력을 검증한다. 새로운 단어 및 독립 절 전체의 일반화 성능으로 해석할 수 없다.

## 처음 평가와 주석 정정

첫 결과는 142/143 정확도(99.30%), 정상 105/105, 거부 37/38이었다. M130은 `forward` 뒤에 공백을 붙여 원문 절 길이 97바이트를 만든 경계 사례다. 봉인 주석은 거부를 예상했지만 구현과 고정 규약은 절 양 끝 ASCII 공백을 먼저 제거하고 96바이트 한도를 적용한다. 정규화 절은 7바이트이므로 수락이 규약에 맞다. 이 차이를 모델 오류로 숨기거나 학습에 반영하지 않았다. 최초 결과 파일은 변경하지 않고, 사후 주석 정정과 정정 결과를 별도 파일에 저장했다.

M133은 반복된 `go`로 만든 16토큰 경계 진단이다. 자연스러운 실제 명령이 아니어서 처음부터 행동 정답을 지정하지 않았으며, 행동 정확도 분모에서 제외했다. 구조 문법 통과 여부는 검사했다. 봉인 전에는 M134의 17토큰 제한이 구조 문법이 아니라 절 추론을 거부한다는 주석 오류도 별도로 정정했다.

추가로 직접 작성한 경계 진단 7개에서 공백 제거 후 96/97바이트, 원문 전체 193바이트, 16/17토큰을 검사했다. Python/C++ 결과는 모두 일치했다. 모델이나 계획 코드는 이 정정 때문에 변경하지 않았다. 추가 따옴표/역슬래시 2개 사례에서도 문법 이후 절 추론 거부와 유효한 JSON 출력, Python/C++ 일치를 확인했다.

## 신경망과 계획을 구분한 결과

- 독립적으로 분명한 정답을 붙일 수 있는 양성 절 240개: 원시 NN 240/240, guard 적용 후 240/240. 반복 절을 포함하며 상당수는 학습 절과 겹친다.
- 별도 합성 영어 test 7,376개 저장 텐서 재계산: 원시 NN 정확도 98.5087%, guard 적용 정확도 98.4138%, 양성 수락률 98.2548%, 잘못 수락한 행동 0개.
- 이 데이터들은 공개된 실제 사용자 발화가 아니라 직접 생성한 합성 영어 자료다.
- 문법은 최대 원문 192바이트, 공백 제거한 절 96바이트, 최대 4절을 분리한다. 각 절은 E16/H64/10분류 NN으로 독립 평가하고, 계획 정책은 STOP 우선, UNKNOWN 거부, 역방향/ON-OFF 충돌 거부, 중복 제거를 수행한다. 다중 발화 전체를 한 mean-pooling 벡터로 분류하지 않는다.

실제 생성 C++ 모델과 shared `CommandPlan`을 컴파일했다. 봉인 사례 144개에서 Python/C++ 행동 및 이유가 모두 일치했다. 동일한 사례에서 전체 모델과 두 H32 분할 모델도 행동 및 이유가 모두 일치했다. 262개 절 중 250개에 실제 NN 점수가 계산됐다. 최대 점수 차이는 Python/C++ 전체 `4.98e-8`, C++ 전체/분할 `1.52e-5`였다. 분할 번역 단위의 namespace를 분리해 서로 다른 모델의 inline 심볼이 섞이지 않도록 했다.

## 학습 및 출처 검사

실제 trainer의 `forward`/`lossgrad` 함수만 AST로 추출해 초기값과 저장 checkpoint에서 수치 미분 30개를 검사했다. 최대 오차는 `3.32e-11`이었고 전체 tensor가 초기값에서 변경됐다. train/val/test는 각각 40,596/5,532/7,376개이며 정규화 문장 교집합은 각 쌍에서 0개다. 모델 파일 SHA-256은 `88ca94525dfd32c0a502611da5243d513480b9a52cda5b794bd50a99f9b7b154`로 metadata와 일치한다.

소스 검토에서 학습 gradient와 vocabulary는 train만 사용하고, checkpoint는 val loss로, threshold/margin은 val 점수로 선택한 것을 확인했다. 선택된 step은 1600, threshold는 0.5, margin은 0이다. test는 모델 metadata를 고정한 후에 계산한다. 원시 점수와 양성 수락률을 별도로 보고하므로 전체 거부를 높은 정확도로 오해하지 않는다.

## AVR 및 통합 검증

실제 Arduino CLI Uno 빌드에서 세 probe 모드와 네 보드가 모두 컴파일됐다. 자세한 로그는 `multi_verification/build_*.log`에 있다.

| 대상 | Flash | 정적 RAM | 남는 RAM |
|---|---:|---:|---:|
| 전체 probe | 16,038 | 543 | 1,505 |
| I2C H32 probe | 13,878 | 555 | 1,493 |
| SPI H32 probe | 13,878 | 555 | 1,493 |
| MAIN | 15,864 | 1,360 | 688 |
| I2C 계산기 | 12,858 | 341 | 1,707 |
| SPI 계산기 | 12,960 | 341 | 1,707 |
| UART actuator | 4,954 | 411 | 1,637 |

모델의 학습 parameter는 실제 5,658개다. int8 embedding/weight와 float32 bias/scale 저장량은 5,892바이트다. MAIN의 정적 RAM 여유는 목표 512바이트 이상이다. 이 값은 링크 시 전역 변수를 뺀 여유이며 실행 중 stack 최고 사용량 측정은 아니다. MAIN의 193바이트 원문, 97바이트 절 추출, parser 임시 배열 및 ISR stack이 동시에 필요한 경우의 실기 stack 여유는 아직 확인하지 않았다.

호스트에서 실제 MAIN register scheduler, 실제 I2C/SPI neural worker, 실제 actuator 엔진을 별도 번역 단위로 연결한 통합 시험을 실행했다. 2/3/4절, 뒤 절 UNKNOWN/충돌, STOP 우선, 중복 제거, 두 번째 절 통신 실패 때 앞 절 미실행, UART/TWI/SPI 상태와 ACK 처리를 통과했다. actuator 시험은 한 배치 전체 검증, 잘못된 후속 항목의 부분 실행 방지, 반전 시 100ms 중립, 액세서리 staged commit, STOP/버튼/새 세션/watchdog 취소와 중복 lease 유지 등을 검사했다. 추가 독립 시험은 protocol VERSION 1, LABELS_VERSION 2, 이전 모델 식별자를 거부하는 동작을 확인했다.

실제 USB 업로드, 네 보드의 실제 전기적 동작, 실제 I2C/SPI/UART 캡처와 bus timing은 측정하지 않았다. 호스트 시험을 실제 배선 추적 결과로 주장하지 않는다.

## 재실행

저장된 모델과 펌웨어를 고정한 뒤 프로젝트 루트에서 다음을 실행한다. `first` 결과는 최초 평가 기록이므로 재실행할 때 새 이름을 지정한다.

```sh
python3 intent/multi_verification/evaluate_sealed.py intent rerun
python3 intent/multi_verification/evaluate_sealed.py intent corrected
python3 intent/multi_verification/evaluate_cpp.py
python3 intent/multi_verification/audit_training.py intent intent/train.py
python3 intent/multi_verification/audit_nn_clauses.py
python3 intent/multi_verification/recompute_test_nn.py
sh intent/multi_verification/run_firmware_host.sh
sh intent/multi_verification/build_all.sh
```

`sealed_cases.jsonl`, 원래/정정 요약, C++ 절별 점수, 수치 미분과 각 빌드 로그를 함께 보관한다.
