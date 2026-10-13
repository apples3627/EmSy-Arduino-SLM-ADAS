#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "board_config.h"
namespace MainIO {
volatile uint32_t ticks=0;
volatile uint8_t adasDanger=1,adasEvent=1,adasGateRequired=MainConfig::REQUIRE_ADAS_FOR_MOTION;
volatile uint8_t rx[64],tx[64],rr=0,rw=0,xr=0,xw=0,softOverflow=0;
volatile uint16_t receivedBytes=0,receiveErrors=0;
volatile uint8_t txBit=10,txByte=0,txFrameLeft=0,rxBit=255,rxByte=0;
inline bool adasLow(){return !(PIND&_BV(PIND6));}
inline uint32_t now(){uint8_t s=SREG;cli();uint32_t n=ticks;SREG=s;return n;}
// Single producer, ISR consumer: unpublished slots stay invisible until final atomic head store.
inline bool actorSend(const Proto::Frame& f){uint8_t write=xw;if(uint8_t((xr-write-1)&63)<32)return false;for(uint8_t i=0;i<32;i++){tx[write]=f.bytes[i];write=(write+1)&63;}xw=write;return true;}
inline void actorFlush(){uint8_t saved=SREG;cli();xw=(xr+txFrameLeft)&63;SREG=saved;}
inline void gateOff(){TCCR1A&=uint8_t(~(_BV(COM1A1)|_BV(COM1A0)|_BV(COM1B1)|_BV(COM1B0)));PORTB&=uint8_t(~(_BV(PB1)|_BV(PB2)));PORTD&=uint8_t(~(_BV(PD4)|_BV(PD7)));PORTC&=uint8_t(~(_BV(PC0)|_BV(PC1)));}
inline void motorWrite(int8_t direction,bool allowed){uint8_t saved=SREG;cli();if((adasGateRequired&&(adasDanger||adasLow()))||!allowed||!direction){gateOff();SREG=saved;return;}int8_t a=MainConfig::INVERT_MOTOR_A?-direction:direction,b=MainConfig::INVERT_MOTOR_B?-direction:direction;uint8_t d=PORTD&uint8_t(~(_BV(PD4)|_BV(PD7))),c=PORTC&uint8_t(~(_BV(PC0)|_BV(PC1)));d|=a>0?_BV(PD4):_BV(PD7);c|=b>0?_BV(PC0):_BV(PC1);PORTD=d;PORTC=c;OCR1A=OCR1B=MainConfig::MOTOR_DUTY;TCCR1A=uint8_t((TCCR1A&~(_BV(COM1A0)|_BV(COM1B0)))|_BV(COM1A1)|_BV(COM1B1));SREG=saved;}
inline uint32_t readSession(){uint32_t v=0;for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){}EEAR=1020+i;EECR|=_BV(EERE);v|=uint32_t(EEDR)<<(i*8);}return v;}
inline void persist(uint32_t v){for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){}EEAR=1020+i;EECR|=_BV(EERE);uint8_t b=uint8_t(v>>(i*8));if(EEDR==b)continue;EEDR=b;uint8_t s=SREG;cli();EECR|=_BV(EEMPE);EECR|=_BV(EEPE);SREG=s;}}
// Both Uno endpoints enable internal SDA/SCL pull-ups. TWBR255/prescaler1 gives ~30.4kHz.
inline void setup(){cli();DDRD|=_BV(DDD3)|_BV(DDD4)|_BV(DDD7);DDRD&=uint8_t(~(_BV(DDD2)|_BV(DDD6)));PORTD|=_BV(PORTD3)|_BV(PORTD2)|_BV(PORTD6);DDRC|=_BV(DDC0)|_BV(DDC1);DDRC&=uint8_t(~(_BV(DDC4)|_BV(DDC5)));MCUCR&=uint8_t(~_BV(PUD));PORTC|=_BV(PORTC4)|_BV(PORTC5);DDRB|=_BV(DDB0)|_BV(DDB1)|_BV(DDB2)|_BV(DDB3)|_BV(DDB5);DDRB&=uint8_t(~_BV(DDB4));PORTB|=_BV(PORTB0);TCCR1A=_BV(WGM10);TCCR1B=_BV(WGM12)|_BV(CS11)|_BV(CS10);TIMSK1=0;OCR1A=OCR1B=0;gateOff();TCCR0A=_BV(WGM01);TCCR0B=_BV(CS01)|_BV(CS00);OCR0A=249;TCNT0=0;TIMSK0=_BV(OCIE0A);TCCR2A=0;TCCR2B=_BV(CS21);TCNT2=0;OCR2B=208;TIMSK2=_BV(OCIE2B);EICRA=_BV(ISC01);EIFR=_BV(INTF0);EIMSK=_BV(INT0);PCMSK2=_BV(PCINT22);PCIFR=_BV(PCIF2);PCICR|=_BV(PCIE2);adasDanger=adasLow();SPCR=_BV(SPE)|_BV(MSTR)|_BV(SPR1)|_BV(SPR0);SPSR=0;TWBR=255;TWSR=0;TWCR=_BV(TWEN);UCSR0B=0;sei();}
}
ISR(TIMER0_COMPA_vect){MainIO::ticks++;}
ISR(TIMER2_COMPB_vect){using namespace MainIO;OCR2B=uint8_t(OCR2B+208);if(txBit==10){if(xr==xw)return;if(!txFrameLeft)txFrameLeft=32;txFrameLeft--;txByte=tx[xr];xr=(xr+1)&63;txBit=0;PORTD&=uint8_t(~_BV(PORTD3));}else if(txBit<8){if(txByte&(1<<txBit))PORTD|=_BV(PORTD3);else PORTD&=uint8_t(~_BV(PORTD3));txBit++;}else{PORTD|=_BV(PORTD3);txBit++;}}
ISR(INT0_vect){using namespace MainIO;if(rxBit!=255)return;rxBit=0;rxByte=0;OCR2A=uint8_t(TCNT2+104);TIFR2=_BV(OCF2A);TIMSK2|=_BV(OCIE2A);EIMSK&=uint8_t(~_BV(INT0));}
ISR(TIMER2_COMPA_vect){using namespace MainIO;bool high=PIND&_BV(PIND2);if(rxBit==0){if(high)rxBit=255;else rxBit=1;}else if(rxBit<=8){if(high)rxByte|=1<<(rxBit-1);rxBit++;}else{if(high){uint8_t n=(rw+1)&63;if(n==rr){softOverflow=1;receiveErrors++;}else{rx[rw]=rxByte;rw=n;receivedBytes++;}}else{softOverflow=1;receiveErrors++;}rxBit=255;}if(rxBit==255){TIMSK2&=uint8_t(~_BV(OCIE2A));EIFR=_BV(INTF0);EIMSK|=_BV(INT0);}else OCR2A=uint8_t(OCR2A+208);}
ISR(PCINT2_vect){if(MainIO::adasLow()){MainIO::adasDanger=1;if(MainIO::adasGateRequired)MainIO::gateOff();}MainIO::adasEvent=1;}
