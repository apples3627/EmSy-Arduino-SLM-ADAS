# 현재 네 Uno R3 배선 — 2026-10-13 / protocol5

핀 번호 앞의 보드 역할을 구분합니다. 모든 배선은 전원을 끄고 연결합니다. 별도 USB 전원의5V 레일끼리는 연결하지 않고 공통 GND만 연결합니다.

## 보드 사이 연결

|출발/기준|연결 대상|기능|
|---|---|---|
|SLM D3 TX|MAIN D2 RX / INT0|software UART9600 8N1|
|MAIN D3 TX|SLM D2 RX / INT0|software UART 반환 ACK/상태|
|SLM D0/D1|USB PC USART0|PC 영어 명령9600/Newline; 외부 배선 없음|
|MAIN A4 SDA|ADAS A4 SDA|I2C0x12/약30.4kHz|
|MAIN A5 SCL|ADAS A5 SCL|I2C clock|
|MAIN **D8 / PB0**|SPI 보드 D10 SS|SPI chip select, active LOW|
|MAIN D11 MOSI|SPI 보드 D11 MOSI|SPI Mode0/125kHz|
|MAIN D12 MISO|SPI 보드 D12 MISO|SPI 응답|
|MAIN D13 SCK|SPI 보드 D13 SCK|SPI clock|
|ADAS **D4 open-drain**|MAIN **D6 PCINT22** + SPI 보드 **D2 INT0**|같은 공통 ALERT선, active LOW|
|각 보드 GND|나머지 세 보드 GND|공통 기준 전위|

ALERT→4.7kΩ→MAIN5V 풀업 한 개. ADAS는 LOW만 구동합니다. MAIN↔ADAS의 SDA/SCL은 두 Uno의 내부 풀업을 활성화합니다. 짧은 배선으로 시험하고 필요 시 외부4.7kΩ을 각선에 추가합니다. MAIN D2/D3는 UART이며 ALERT를 연결하지 않습니다. **MAIN D10은 모터 PWM이므로 SPI 보드 D10과 연결하지 않습니다.**

## MAIN → L298N 두 모터

|MAIN 핀|L298N 입력|사용|
|---|---|---|
|D4 / PD4|IN1|모터 A 방향1|
|D7 / PD7|IN2|모터 A 방향2|
|D9 / PB1 OC1A|ENA|모터 A PWM|
|A0 / PC0|IN3|모터 B 방향1|
|A1 / PC1|IN4|모터 B 방향2|
|D10 / PB2 OC1B|ENB|모터 B PWM|
|GND|GND|모터 전원과 공통|

모터 A의 두 선→OUT1/OUT2, 모터 B의 두 선→OUT3/OUT4입니다. 모터를 Uno 핀/5V에 직접 연결하지 않습니다. 모터 정격에 맞는 별도 공급을 L298N 모터 전원에 연결합니다. 모터 두 선 중 하나를 GND에 고정하는 배선이 아닙니다.

**ENA/ENB enable 점퍼는 두 개 모두 제거**하고 D9/D10 PWM을 연결합니다. 모듈의5V regulator-enable 점퍼는 별개의 설정입니다. 그 설정에 따라5V 단자가 입력/출력일 수 있으므로 실제 모듈 자료를 확인하고 Uno USB5V와 regulator5V 출력을 직접 연결하지 않습니다. 로직5V가 필요하면 공급 구성을 확정한 뒤 배선합니다. 소스는 [ST L298 자료](https://www.st.com/resource/en/datasheet/l298.pdf)를 기준으로 두 enable과 네 방향 입력을 사용합니다. 모터의 전압/정지 전류와 모듈 방열/보호 회로는 실제 부품에서 확인합니다.

전진은 A IN1=HIGH/IN2=LOW, B IN3=HIGH/IN4=LOW이며 후진은 반대입니다. 차체에서 모터가 서로 거울 방향으로 장착돼 회전이 반대라면 전원을 끄고 해당 OUT 두 선을 바꾸거나 main/src/board_config.h의 INVERT_MOTOR_A/B를 조정합니다. 현재는 두 모터를 같이 전진/후진하며 좌우 조향 명령은 없습니다.

Timer1 fast PWM8bit 약977Hz, OCR1A/B=75(약30%)이며 기본 주행500ms입니다. 방향 반전100ms 무구동. STOP/ADAS는 PWM compare 연결과 네 방향 신호를 모두OFF로 합니다. 실제 모터의 관성까지 즉시 없애는 기계적 제동을 보증하지 않습니다. 전원 차단 때 enable이 떠 있지 않도록 ENA/ENB 외부10kΩ pulldown을 권장합니다.

## ADAS → HC-SR04

|센서|ADAS 핀|
|---|---|
|Vcc / GND|5V / GND|
|Trig|D7 / PD7|
|Echo|D8 / PB0 ICP1|

센서 인쇄 핀 순서를 확인합니다. 20cm 정지,25cm 이상3회 해제입니다. ADAS 보드 자체는 부팅·센서 누락·invalid echo·stale에도 ALERT LOW를 출력합니다. MAIN 기본 시연 모드는 유효한 측정이 없으면 BYPASS로 이 신호를 무시합니다. 유효 측정에서 ACTIVE가 되면20cm 정지/25cm3회 해제를 적용합니다. 모드 전환 뒤 새 명령이 필요합니다.

## SPI 보드 → LCD·부저·LED

|장치|SPI 보드 연결|
|---|---|
|LCD GND / VCC|GND /5V|
|LCD SDA / SCL|**SPI 보드 A4 / A5**|
|3핀 부저 S / + / -|D3 /5V /GND|
|LED anode|D4→220~330Ω 직렬저항→anode|
|LED cathode|GND|

LCD 버스는 SPI 보드만의 로컬I2C입니다. **LCD SDA/SCL을 MAIN·ADAS A4/A5에 연결하지 않습니다.** 기본주소0x27/16×2 PCF8574이고0x3F와 backpack 매핑은 config로 바꿉니다. 모듈에 이미 풀업이 있으면 중복 추가하지 않습니다. [NXP PCF8574 자료](https://www.nxp.com/docs/en/data-sheet/PCF8574_PCF8574A.pdf)와 실제 LCD backpack 배선을 확인합니다. 부저 능동/수동 설정 및 S/+/− 순서는 모듈별로 확인합니다.

SPI 보드 D2는 ALERT 전용입니다. 이전 작동 보드의 로컬 정지 버튼을 여기에 연결하지 않습니다. 와이퍼·서보 핀은 이번 구성에서 배정/사용하지 않습니다.

## 타이머와 예약 자원

|보드|자원|사용|
|---|---|---|
|SLM|USART0 / Timer1 COMPA·COMPB·INT0|PC / D2/D3 soft UART|
|SLM|Timer0 COMPA|1ms tick, NN은 foreground|
|MAIN|Timer0 COMPA|1ms tick|
|MAIN|Timer1 OC1A/OC1B|D9/D10 두 모터 PWM, 타이머IRQ 없음|
|MAIN|Timer2 COMPA·COMPB·INT0|D2/D3 soft UART,2MHz tick/208tick비트|
|MAIN|PCINT2 PCINT22|D6 ALERT, 두 모터 즉시OFF|
|MAIN|TWI / SPI|ADAS / 주변장치, CS D8|
|ADAS|Timer0 / Timer1 ICP1·COMPA·OVF|tick / echo capture·trigger종료|
|SPI 보드|SPI STC + SS PCINT0|D10~D13 통신|
|SPI 보드|TWI|로컬LCD A4/A5|
|SPI 보드|Timer0 / Timer2 COMPA|tick / 수동부저1kHz|
|SPI 보드|INT0|D2 ALERT, LED·부저 즉시OFF|

D10은 AVR SPI master의 하드웨어SS이기도 하지만 MAIN에서 **OUTPUT**으로 유지하므로 PWM 출력과 master 기능이 공존합니다. 실제 slave select는 MAIN D8입니다. 네 역할 모두 Arduino core의 millis/delay 타이머 기능은 사용하지 않습니다. MAIN/주변장치 EEPROM1020..1023은 세션에 예약합니다. EEPROM 네 바이트 쓰기는 원자적 전원차단 보장 방식이 아니므로 세션 기록 도중 전원을 끄지 않습니다.

## MAIN–ADAS 내부 I2C 풀업 — 2026-10-13

현재 MAIN과 ADAS는 A4/SDA·A5/SCL의 내부 풀업을 레지스터로 활성화합니다. DDRC4/5=0, PORTC4/5=1, MCUCR.PUD=0입니다. 별도4.7kΩ 없이 우선 짧은 점퍼선으로 두 보드를 연결할 수 있도록 MAIN TWI 속도를 약30.4kHz(TWBR=255, prescaler1)로 낮췄습니다. 주소0x12,70ms센서주기,프레임/CRC는 유지합니다. protocol5의 선택 ADAS 정책은 MAIN 지침을 따릅니다.

두 보드 모두 재업로드해야 합니다. 모터 전원을 끄고MAIN·ADAS를업로드한뒤A4↔A4,A5↔A5,GND↔GND를확인합니다. 센서앞25cm이상공간에서 `!status`의 adas_mode=ACTIVE와 READY를 확인합니다. 센서 오류일 때 기본 시연 모드는 BYPASS로 표시합니다.

AVR내부풀업은대략20~50kΩ로외부4.7kΩ보다약하며, 두보드의풀업은각선에서병렬로작동합니다. 속도를낮춰도전기적상승시간을보증하지는않습니다. 긴배선/큰정전용량/잡음에서문제가남으면외부풀업을추가해야합니다. 실물파형/통신성공은아직측정하지않았습니다. [Arduino 내부풀업자료](https://docs.arduino.cc/retired/boards/arduino-uno-rev3-with-long-pins/)

LCD는SPI보드의별도I2C버스이며이번변경의대상이아닙니다. ALERT선의정지용풀업배선도별개입니다. SDA/SCL을5V에직접단락하는배선이아니며내부저항을통해끌어올립니다.
