# protocol4 네 보드 실물 통합 순서

1. MAIN/SLM_UART/ADAS_I2C/PERIPHERAL_SPI ZIP 전체를 배포하고 보드에 역할 라벨을 붙입니다. 이전역할ZIP을 섞지 않습니다. 외부 배선·구동 전원 없이 각Uno에 업로드합니다.
2. 전원을 끄고 PIN_MAP의 교차UART2선, SPI4선(CS **MAIN D8→SPI D10**), MAIN↔ADAS I2C2선, ALERT(**ADAS D4→MAIN D6+SPI D2**), 공통GND를 연결합니다. 독립USB5V끼리는 연결하지 않습니다.
3. LCD를 SPI 보드 A4/A5에만 연결합니다. MAIN↔ADAS I2C와 합치지 않습니다. 주소0x27/0x3F·LCD backpack 매핑·16×2 가정과 부저 능동/수동 설정을 확인합니다. LCD가 안 나오면 contrast 조정·주소·전원부터 확인합니다. LCD오류가SPI/정지를 막지 않아야 합니다.
4. 모터전원 없이 부팅합니다. SLM USB9600/Newline 콘솔에서 WAIT→READY, !status를 확인합니다. 센서누락/20cm이내에서는 READY가 되면 안 됩니다.25cm이상3회+OFF ACK 후 준비됩니다.
5. LED/부저단독ON/OFF와2~3개의 다중명령을 시험합니다. 와이퍼명령이 있는계획은 앞의LED도 실행하지 않고전체STOP이어야 합니다. STOP이 있으면전체OFF. 반대방향/ONOFF충돌·unknown도전체거절합니다.
6. L298N ENA/ENB 점퍼를 제거하고 ENA D9/ENB D10, IN1D4/IN2D7/IN3A0/IN4A1을 확인합니다. 모터 두 개는OUT1/2,OUT3/4. 모터정격/외부전원/로직5V점퍼 구성을 확정합니다. 최초에는 바퀴를 들어 올리고500ms명령으로 시험합니다.
7. 전진/후진에서두모터가같은차량방향인지확인하고 필요하면 한모터 OUT배선 또는invert 설정을 바꿉니다. lease500ms 후두모터만OFF이며LED/부저는유지됩니다. 반전100ms 동안무구동이고부가변경도지연됩니다.
8. SLM추론중·UART프레임중·SPI전송중·구동중에20cm이내장애물을넣습니다. MAINISR두PWM과방향신호OFF, SPI보드ISRLED/부저OFF를파형으로확인합니다. UART/SPI완료를기다리지않는지실제지연을기록합니다.
9. 장애물을25cm이상으로옮겨3회정상확인해도자동재출발하지않아야합니다. 새명령으로만동작합니다.21~24cm의clearCount초기화와MAIN추가정상3회조건도확인합니다.
10. Echo누락/고정HIGH, I2C분리, SPI분리, UART분리, 각보드재부팅, 짧은ALERT펄스를시험합니다. 무응답/낡은샘플/오래된epoch가출력을살리지못해야합니다. LCDfault에는?또는!로미확인/차단을표시합니다.
11. SLM단독stop을보내MAIN desired0, 주변장치OFF ACK뒤applied0/known1, SLM/ADAS/SPI snapshot동기화를확인합니다. STOP은LCD정보표시를지우는명령이아닙니다. 실제구동회로출력만정지합니다.

호스트/Uno컴파일은 실물완료증거가 아닙니다. 테스트결과·배선사진·파형·각팀원기여여부는 팀통합시기록합니다. 실제부품이확정되지않은부분(부저종류,LCD주소/칩,모터전원)은업로드전config와핀표를확인합니다.
