# 검증 기록

2026-10-07, macOS, NumPy 2.3.4, Arduino CLI 1.5.1, Arduino AVR core 1.8.6, AVR GCC 7.3.0.

- `OPENBLAS_NUM_THREADS=1 python3 tools/train.py`: seed 20261007, 1800 Adam 업데이트. 직접 작성한 corpus.txt로 학습, 메타데이터에 SHA256 및 최종 학습 지표 저장. 일반화 성능 평가 없음.
- `python3 tools/verify.py`: 실제 C++ 공유 추론 헤더를 컴파일하여 독립적인 Python float32 양자화 참조와 비교. 빈 문맥, hello, the cat, a robot, 대문자, 기호, UTF-8, 100바이트 문맥, the sun, 줄바꿈 포함 입력의 총 10개 문맥에서 29개 로짓 모두 비교. 최대 오차 0 (현재 호스트/컴파일러). 허용 오차 rtol=2e-6, atol=2e-5. 세 데모 입력의 전체 자동 회귀 생성도 일치 및 마침표 종료 확인.
- `tools/test.sh`: 위 비교에 더해 실제 .ino 파일을 Arduino Serial mock으로 실행. 초기 READY, 정상 출력, 63바이트 허용, 64바이트 거절, 초과 후 다음 정상 입력 복구, CRLF, 빈 줄, 비어휘 문자, 대문자, 나눠 들어온 줄, 종료 후 LED LOW 확인.
- `tools/build.sh`: 실제 `arduino:avr:uno` 타깃 컴파일 성공. Flash 8614/32256 bytes, 전역 SRAM 462/2048 bytes, 남은 SRAM 1586 bytes. Arduino 코어와 UART 버퍼를 포함한 링커 결과.

호스트 검증 결과:

```text
hello friend. we can learn together.
the cat is small. the cat likes milk.
a robot can help you.
```

Arduino 연결이 없으므로 업로드, 실물 실행 시간, 실물 출력 및 최대 스택은 미검증입니다. 호스트와 AVR의 float 연산 결과가 경계 상황에서 다른 토큰을 선택할 가능성은 실물 확인 전까지 남습니다. SRAM 여유는 링크 시 정적 메모리에서 뺀 수치이며 최대 스택을 계측한 보장은 아닙니다.
