# 4개 Uno 핀 사용·배선표

핀 이름 앞의 **보드 역할**을 반드시 구분합니다. MAIN D2와 ACTUATOR D2는 같은 번호지만 서로 다른 핀입니다. 점퍼 배선은 전원을 끈 상태에서 연결합니다.

## 1. 보드 간 연결: 이 표의 선만 연결

|출발/기준 핀|상대 핀|신호·방향|주의|
|---|---|---|---|
|MAIN D3 / PD3|ACTUATOR D0 / PD0 RX|MAIN TX → 제어 RX|9600, 8N1, 바이너리 32B|
|MAIN D2 / PD2 INT0|ACTUATOR D1 / PD1 TX|제어 TX → MAIN RX|TX끼리/RX끼리 연결하지 않음|
|MAIN A4 / PC4 SDA|COMPUTE_I2C A4 / PC4 SDA|양방향 SDA|주소 0x12, 100kHz|
|MAIN A5 / PC5 SCL|COMPUTE_I2C A5 / PC5 SCL|MAIN SCL → slave, open-drain|clock stretching 허용|
|MAIN D10 / PB2 SS|COMPUTE_SPI D10 / PB2 SS|MAIN 출력 → slave 입력|active LOW|
|MAIN D11 / PB3 MOSI|COMPUTE_SPI D11 / PB3 MOSI|MAIN 출력 → slave 입력|Mode0, MSB first|
|MAIN D12 / PB4 MISO|COMPUTE_SPI D12 / PB4 MISO|slave 출력 → MAIN 입력|다른 핀에 연결 금지|
|MAIN D13 / PB5 SCK|COMPUTE_SPI D13 / PB5 SCK|MAIN 출력 → slave 입력|125kHz, LED 제어 핀으로 쓰지 않음|
|MAIN GND|I2C·SPI·ACTUATOR GND|공통 기준 전위|네 보드 GND 모두 연결|

**I2C 풀업:** SDA→4.7kΩ→MAIN 5V, SCL→4.7kΩ→MAIN 5V를 한 쌍만 둡니다. 모듈에 기존 풀업이 있는지 먼저 확인합니다. 풀업용 5V와 다른 보드의 5V 공급선을 연결하는 것은 별개입니다.

**각자 USB로 전원 공급할 때:** 다른 보드 5V 핀끼리는 점퍼로 연결하지 않습니다. 위 표의 통신 신호와 공통 GND만 연결합니다. 하나의 공동 전원으로 바꾼다면 정격·공급 구성부터 다시 확인합니다.

## 2. 역할별 예약·사용 핀

|보드|핀|사용|
|---|---|---|
|MAIN|D0/D1|USB PC 콘솔 전용. 외부 장치 연결 없음|
|MAIN|D2/D3|software UART RX/TX|
|MAIN|A4/A5|I2C 계산 보드|
|MAIN|D10/D11/D12/D13|SPI 계산 보드|
|I2C 계산|A4/A5|TWI slave SDA/SCL|
|I2C 계산|D4|추론 중 HIGH 디버그 출력. 선택적 계측만 연결|
|SPI 계산|D10~D13|SS/MOSI/MISO/SCK|
|SPI 계산|D4|추론 중 HIGH 디버그 출력. 선택적 계측만 연결|
|두 계산 보드|D0/D1|펌웨어 동작 중 외부 UART 연결 없음|
|ACTUATOR|D0/D1|MAIN UART RX/TX. 업로드 때 선 분리|
|ACTUATOR|D2|로컬 전체 정지 버튼 입력, 내부 pull-up|
|ACTUATOR|D4/D7/D5|H-bridge IN1/IN2/PWM|
|ACTUATOR|D12|선택적 H-bridge STBY. **MAIN D12에 연결하지 않음**|
|ACTUATOR|D8|조명 LED 출력|
|ACTUATOR|D9|서보 signal|
|ACTUATOR|D6|피에조 경적 signal|

표에 없는 핀은 이 펌웨어가 사용하지 않습니다. 핀을 추가로 쓰려면 해당 타이머/포트와 충돌 여부를 먼저 확인합니다.

## 3. 두 선 DC 모터: H-bridge를 거쳐 연결

모터에 +/− 두 선만 있어도 **모터를 Uno 핀에 직접 연결하면 안 됩니다.** 전진/후진은 H-bridge가 모터 양단의 극성을 바꿔 만듭니다. 모터의 어느 선도 이 구성에서 GND에 영구 고정하지 않고, 두 선 모두 드라이버 출력으로 갑니다.

|기능|TB6612 한 채널 예시|L298N 한 채널 예시|
|---|---|---|
|ACTUATOR D4|AIN1|IN1|
|ACTUATOR D7|AIN2|IN2|
|ACTUATOR D5 PWM|PWMA|ENA|
|ACTUATOR D12|STBY|연결하지 않음|
|모터 두 선|AO1/AO2|OUT1/OUT2|
|로직 전원|VCC: ACTUATOR의 5V 기준, 부품 자료 확인|보드의 로직 전원/레귤레이터 설정 확인|
|모터 전원|VM: 모터 정격에 맞는 외부 공급|모터 전원 입력: 정격에 맞는 외부 공급|
|GND|외부 PSU·driver·Uno 공통|외부 PSU·driver·Uno 공통|
|설정|`MOTOR_HAS_STBY=1` 기본|`MOTOR_HAS_STBY=0`, ENA 점퍼 처리 확인|

두 표는 **인터페이스 예시**입니다. 실제 드라이버와 모터의 전압·정지 전류(stall current)·보호 회로는 부품 데이터시트로 확정해야 합니다. L298N의 **ENA 속도 허용 점퍼**와 **5V 레귤레이터 점퍼**는 다른 설정이므로 구분합니다. ENA 점퍼가 PWM 입력을 상시 HIGH로 묶는 모듈에서는 제거합니다.

기본 방향 신호: 전진 IN1=HIGH/IN2=LOW, 후진 LOW/HIGH, 정지 PWM 비활성·두 입력 LOW. 실제 회전 방향이 반대면 전원을 끈 뒤 모터 두 선을 바꾸거나 `Config::invertDirection`을 변경합니다. 기본 듀티 75/249(약30%), 구동 시간 500ms입니다. 500ms는 MAIN이 보내는 값이므로 시간을 바꾸려면 MAIN 요청 duration과 제어 상한을 함께 확인합니다. 반전 시 100ms 무구동 구간을 둡니다. 정지는 출력 OFF이며 관성까지 즉시 없애는 기계적 제동을 보증하지 않습니다.

드라이버가 아직 없다면 실제 모터는 연결하지 않고, ACTUATOR D4/D7에 각각 저항 포함 LED를 달아 방향 신호와 ACK를 먼저 확인합니다. 독립 드라이버 전원에서 Uno가 꺼졌을 때도 EN/STBY가 OFF를 유지하도록 필요한 풀다운/enable 회로는 선택 부품 자료대로 구성합니다.

## 4. 조명·와이퍼·경적·버튼

- 조명: ACTUATOR D8 → 220~330Ω 저항 → 소형 LED anode, LED cathode→GND. 전력 조명은 별도 드라이버를 씁니다.
- 서보: signal→ACTUATOR D9, V+→서보 정격에 맞는 별도 안정화 전원, GND→공통 GND. 선 색은 모델마다 달라 부품 표시를 확인합니다. 처음 45~135° 범위가 기구를 건드리지 않는지 확인 후 연결합니다.
- 수동 피에조: ACTUATOR D6→신호, 다른 단자→GND. 1kHz 기본이며 정격 전류에 따라 드라이버가 필요할 수 있습니다. 능동 부저라면 `ACTIVE_BUZZER=1`로 변경하고 부품 전압을 확인합니다.
- 전체 정지 버튼: ACTUATOR D2→스위치→GND. 내부 pull-up으로 미눌림 HIGH입니다. MAIN D2와 연결하지 않습니다. 누르면 10ms 후 모든 출력 OFF, 놓아도 이전 동작을 자동 재개하지 않습니다.

서보의 전원 공급은 PWM signal 차단과 다릅니다. STOP은 모터·조명·서보 signal·경적을 모두 끄며 서보 park 동작도 취소합니다. 버튼 해제 뒤 MAIN `!retry`로 새 세션을 만들고 새 명령을 보내야 합니다.

## 5. 타이머·레지스터 충돌 확인

|보드|자원|배정|
|---|---|---|
|MAIN|Timer0 COMPA|1ms 시간 기준, core overflow OFF|
|MAIN|Timer1 COMPA/COMPB + INT0|D3 TX / D2 RX, 약9615 baud|
|MAIN|USART0|D0/D1 PC 콘솔|
|MAIN|TWI / SPI|A4/A5와 D10~D13, 동시에 운용|
|I2C·SPI 계산|Timer0 COMPA|시간 기준, 모델 계산은 foreground|
|ACTUATOR|Timer0 mode7 COMPA / OC0B|1ms tick + D5 PWM, 1kHz|
|ACTUATOR|Timer1 mode14 / OC1A|D9 servo, 50Hz|
|ACTUATOR|Timer2 CTC COMPA|D6 수동 피에조, 1kHz 토글 출력|
|ACTUATOR|USART0|D0/D1 MAIN 링크|

ACTUATOR에서는 Timer2의 D3/D11 PWM을 추가 모터용으로 쓰지 않습니다. MAIN의 Timer1과 ACTUATOR의 Timer1은 서로 다른 칩이므로 충돌하지 않습니다. 모든 핀 기준은 Uno R3입니다.
