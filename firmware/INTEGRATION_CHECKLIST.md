# 네 Uno 팀 통합 시험

현재 역할은 MAIN, SLM_SPI, ADAS_I2C, ACTUATOR_UART입니다. 모든 보드가 protocol3/model95616d56/labels3 배포본이어야 합니다. 이전 H32 compute ZIP과 섞지 않습니다.

1. 역할별 GUIDE와 PIN_MAP을 읽고 각 ZIP 전체를 풀어 외부 배선 없이 Uno에 업로드합니다. ACTUATOR D0/D1을 업로드 동안 분리합니다. 보드에 역할 라벨을 붙입니다.
2. 전원을 끄고 공통 GND, SPI4선, I2C2선+풀업2개, 교차UART2선, ALERT 공통선+풀업1개를 연결합니다. 독립USB의5V 레일을 서로 연결하지 않습니다. 센서 TrigD7/EchoD8을 확인합니다.
3. 모터·서보 전원 없이 시작합니다. 센서를 분리한 경우 READY가 되면 안 됩니다. boot/fault, desired/applied/known/epoch를 MAIN !status로 확인합니다. ALERT 실제LOW와 출력OFF를 계측합니다.
4. 센서 앞25cm 이상 빈 공간에서 세 번 정상 측정 후 READY와 OFF 확인을 기다립니다. `lights on`, `horn on`, `wipers on`을 하나씩 보내 RESULT/APPLIED를 확인합니다. 출력ACK는 실제 바퀴 이동 측정이 아닙니다.
5. `go forward, lights on, wipers on, horn on`을 보내 계획 전체가 적용되는지 확인합니다. 드라이버가 없다면 모터 대신 저항LED로 방향 출력만 확인합니다. 충돌/알 수 없는 절은 부분 동작 없이 전체 거절되어야 합니다.
6. 정상 공간에서 `go forward`를 보내고20cm 이내 장애물을 넣습니다. 추론 중·UART 전송 중·출력 중 모두 시험합니다. 모터PWM·조명·서보signal·경적이 모두OFF인지, stale ACK로 재시작되지 않는지 확인합니다. 실물 응답지연과 UART/SPI/I2C/ALERT 파형을 기록합니다.
7. 장애물을25cm 이상으로 옮겨 세 번 확인해도 자동 재시작되지 않아야 합니다. READY 후 새 명령으로만 작동합니다.21..24cm에서는 해제되면 안 됩니다.
8. Echo 분리/고정HIGH, 센서 제거, I2C 분리, ADAS 재부팅을 시험합니다. fault/200ms freshness 제한으로 전체 정지해야 합니다. 짧은 ALERT LOW 펄스도 보존되는지 확인합니다. 센서의 감지 실패는 독립 실물 평가 대상입니다.
9. `stop` 및 `lights on and stop and horn on`은 모든 출력0이어야 합니다. desired=0, matching ACK 후 applied=0/known=1, SLM·ADAS snapshot revision 공유를 확인합니다.500ms 주행 lease 종료는 drive만0이며 다른 부가 출력은 유지됩니다.
10. 로컬ACTUATOR D2 버튼, UART 분리, ACTUATOR 재부팅을 시험합니다. 해제 후 !retry/새세션으로 복구하고 이전 명령이 실행되지 않는지 확인합니다. 각 담당자가 결과·참여 여부를 기록합니다.

DC 모터는 H-bridge와 별도 정격 전원으로 연결하며 서보도 안정된 별도 전원을 사용합니다. 최초 실물 주행은 바퀴를 들어 올린 상태에서 짧은500ms lease로 시험합니다. 현재 자동 검증은 호스트 로직/레지스터 모형과 Uno 컴파일입니다. 네 보드 실물 시험의 완료를 뜻하지 않습니다.
