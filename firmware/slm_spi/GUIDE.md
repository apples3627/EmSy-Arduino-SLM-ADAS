# SLM SPI Uno R3 업로드 안내

이 폴더 전체를 독립 스케치로 사용합니다. ZIP을 풀면 `slm_spi/slm_spi.ino`와 `src/`가 함께 있어야 합니다. `.ino`만 복사하지 마세요. Arduino IDE에서 `slm_spi.ino`를 열고 **Arduino Uno**, SLM용 Uno R3의 USB 포트를 선택해 업로드합니다. 실제 보드에서 연결을 확인하기 전에는 포트 이름만으로 역할을 단정하지 마세요. 이 작업에서는 실제 업로드·시리얼 모니터를 실행하지 않았습니다.

| MAIN 핀 | SLM 핀 | 용도 |
|---|---|---|
| MAIN D10 | SLM D10 / PB2 | SS |
| MAIN D11 | SLM D11 / PB3 | MOSI |
| MAIN D12 | SLM D12 / PB4 | MISO |
| MAIN D13 | SLM D13 / PB5 | SCK |
| MAIN GND | SLM GND | 공통 기준 |

SPI는 Mode 0, MSB first, 125kHz이며 32바이트 요청과 별도 32바이트 dummy read의 두 transaction을 씁니다. SS를 각 transaction 사이에서 HIGH로 돌립니다. 응답은 전경 loop에서 만들어 두고 ISR은 짧게 바이트만 이동합니다. 쓰기 직후 아직 준비되지 않은 이전 응답이 읽힐 수 있으므로 MAIN이 id/op/CRC를 확인하고 다시 조회합니다. RX는 세 슬롯으로 두 완전한 프레임을 대기시킵니다. SLM D13 LED를 디버깅용으로 점멸시키면 SPI clock과 충돌합니다. SLM D5는 추론 중 HIGH인 선택적 계측 출력입니다.

고정 모델은 model ID `95616d56`, V245/E16/H64, labels version 3입니다. 전체 H64와 global bias C를 한 SLM에서 실행하며 HELLO_ACK 마지막 byte `1`이 이를 표시합니다. MAIN은 결과에 bias를 다시 더하면 안 됩니다. 모델 배열은 flash PROGMEM에 저장되고 재학습하지 않습니다. PC 추론 fallback은 없습니다.

HELLO payload는 session32 네 바이트입니다. 새 HELLO는 입력 job과 공유 상태 snapshot, 대기 RX를 초기화합니다. LOAD_INPUT seq0은 token ID 12개/24바이트, seq1은 네 개/8바이트입니다. 유효 ID는 0..244, padding은 FFFF입니다. START 다음 STATUS로 완료를 확인하고 GET_RESULT seq0 24바이트/seq1 16바이트의 10개 float32 점수를 합칩니다. STATUS는 기존 6바이트 역할 `1`입니다.

STATE_SYNC는 공통 protocol 3의 22바이트 차량 상태를 저장합니다. HELLO session이 다르면 BAD_SESSION, 이전 revision이나 같은 revision의 다른 내용이면 BAD_JOB입니다. 동일 snapshot 재전송은 ACK합니다. SLM은 desired/applied STOP을 관찰할 수 있지만 이 snapshot은 NN 입력 job이나 점수를 바꾸지 않습니다. NN은 stateless 분류기이며 자율 주행 기억이나 출력 제어기가 아닙니다. MAIN이 명령 계획과 안전 정책을 관리하고 ACTUATOR가 적용 상태를 보고합니다.

각 Uno는 자신의 USB 전원으로 사용할 수 있습니다. GND는 함께 연결하지만 별도 USB의 5V 레일을 서로 직접 연결하지 마세요. SPI 배선에는 5V 레일 연결이 필요하지 않습니다. 실제 네 보드의 전기 신호와 지연은 별도 검증이 필요합니다.

검증 명령: `python3 firmware/tests/slm_full_host_test.py`. 실제 portable Worker/프레임 검증과 Python 독립 scalar reference의 33개 점수 비교, 세 compound plan의 clause 분리를 확인합니다. Uno core 1.8.6 빌드: flash 14,948 / 32,256, static SRAM 369 / 2,048바이트. 동적 stack/인터럽트 중첩의 실제 최고 사용량은 물리 보드에서 별도 측정해야 합니다.
