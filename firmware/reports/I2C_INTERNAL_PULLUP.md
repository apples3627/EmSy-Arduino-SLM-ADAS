# 내부 I2C 풀업 변경

## MAIN–ADAS 내부 I2C 풀업 — 2026-10-13

현재 MAIN과 ADAS는 A4/SDA·A5/SCL의 내부 풀업을 레지스터로 활성화합니다. DDRC4/5=0, PORTC4/5=1, MCUCR.PUD=0입니다. 별도4.7kΩ 없이 우선 짧은 점퍼선으로 두 보드를 연결할 수 있도록 MAIN TWI 속도를 약30.4kHz(TWBR=255, prescaler1)로 낮췄습니다. 기존주소0x12,70ms센서주기,프레임/CRC,STOP/ADAS우선정책은 유지합니다.

두 보드 모두 재업로드해야 합니다. 모터 전원을 끄고MAIN·ADAS를업로드한뒤A4↔A4,A5↔A5,GND↔GND를확인합니다. 센서앞25cm이상공간에서 `!status`, `!diag`로 adas=1/clear=1 및READY를확인합니다.

AVR내부풀업은대략20~50kΩ로외부4.7kΩ보다약하며, 두보드의풀업은각선에서병렬로작동합니다. 속도를낮춰도전기적상승시간을보증하지는않습니다. 긴배선/큰정전용량/잡음에서문제가남으면외부풀업을추가해야합니다. 실물파형/통신성공은아직측정하지않았습니다. [Arduino 내부풀업자료](https://docs.arduino.cc/retired/boards/arduino-uno-rev3-with-long-pins/)

LCD는SPI보드의별도I2C버스이며이번변경의대상이아닙니다. ALERT선의정지용풀업배선도별개입니다. SDA/SCL을5V에직접단락하는배선이아니며내부저항을통해끌어올립니다.

검증: 기존전체host회귀와MODELFLOAT32 33문장parity오차0, MAIN/ADAS 실제setup레지스터에서DDRC입력/PORTC풀업/PUD해제, 모터변경/STOP/ALERT후풀업유지, 네Uno컴파일 및최신ZIP4개/파일SHA256검사 통과.
