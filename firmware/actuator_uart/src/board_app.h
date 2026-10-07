#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "control_engine.h"
static_assert(F_CPU == 16000000UL, "ACTUATOR requires a 16 MHz Uno");
static_assert(E2END == 1023, "EEPROM session address requires ATmega328P");
static_assert(F_CPU / 64 / (249 + 1) == 1000, "Timer0 tick must be 1 kHz");
static_assert(F_CPU / 8 / (39999UL + 1) == 50, "Servo frame must be 50 Hz");
static_assert(F_CPU / 64 / (124 + 1) / 2 == 1000, "Passive piezo must be 1 kHz");
namespace Actuator {
static Config board_config;
static Engine engine(board_config);
static Proto::StreamParser parser;
static volatile uint32_t milliseconds;
static volatile uint8_t rx[128],rxHead,rxTail,tx[64],txHead,txTail,rxFault;
static uint8_t stopCandidate;static bool stopStable;static uint32_t stopChanged;
inline uint32_t now(){uint8_t saved=SREG;cli();uint32_t t=milliseconds;SREG=saved;return t;}
inline uint32_t readSession(){uint32_t value=0;for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){} EEAR=1020+i;EECR|=_BV(EERE);value|=uint32_t(EEDR)<<(8*i);}return value;}
inline void saveSession(uint32_t session){for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){} EEAR=1020+i;EECR|=_BV(EERE);uint8_t value=uint8_t(session>>(8*i));if(EEDR==value)continue;EEAR=1020+i;EEDR=value;uint8_t saved=SREG;cli();EECR|=_BV(EEMPE);EECR|=_BV(EEPE);SREG=saved;}while(EECR&_BV(EEPE)){} }
inline uint8_t txFree(){return uint8_t((txTail-txHead-1)&63);}
inline void send(const Proto::Frame& frame){for(uint8_t i=0;i<32;i++){tx[txHead]=frame.bytes[i];txHead=(txHead+1)&63;}UCSR0B|=_BV(UDRIE0);}
inline void bridgeDirection(int8_t direction){uint8_t saved=SREG;cli();uint8_t value=PORTD&uint8_t(~(_BV(PD4)|_BV(PD7)));if(direction>0)value|=_BV(PD4);if(direction<0)value|=_BV(PD7);PORTD=value;SREG=saved;}
inline void hardwareOutputs(){const Outputs& s=engine.outputs();static Outputs applied={0,0,false,false,false,0};static bool initial=true;if(!initial&&s.drive==applied.drive&&s.duty==applied.duty&&s.lights==applied.lights&&s.wiper==applied.wiper&&s.horn==applied.horn&&s.servoAngle==applied.servoAngle)return;bool motorChanged=initial||s.drive!=applied.drive||s.duty!=applied.duty;initial=false;applied=s;int8_t direction=board_config.invertDirection?-s.drive:s.drive;
 if(motorChanged){
 // Always gate PWM before changing either bridge input.
 TCCR0A&=uint8_t(~(_BV(COM0B1)|_BV(COM0B0)));PORTD&=uint8_t(~_BV(PD5));
 bridgeDirection(direction);
#if MOTOR_HAS_STBY
 if(direction&&s.duty)PORTB|=_BV(PB4);else PORTB&=uint8_t(~_BV(PB4));
#endif
 if(direction&&s.duty){OCR0B=s.duty;TCCR0A|=_BV(COM0B1);}else OCR0B=0;
 }
 if(s.lights)PORTB|=_BV(PB0);else PORTB&=uint8_t(~_BV(PB0));
 if(s.wiper){OCR1A=2000UL+uint32_t(s.servoAngle)*2000UL/180;TCCR1A|=_BV(COM1A1);}else{TCCR1A&=uint8_t(~(_BV(COM1A1)|_BV(COM1A0)));PORTB&=uint8_t(~_BV(PB1));}
#if ACTIVE_BUZZER
 TIMSK2=0;if(s.horn)PORTD|=_BV(PD6);else PORTD&=uint8_t(~_BV(PD6));
#else
 if(s.horn)TIMSK2=_BV(OCIE2A);else{TIMSK2=0;PORTD&=uint8_t(~_BV(PD6));}
#endif
}
inline void setup(){cli();DDRD|=_BV(PD4)|_BV(PD5)|_BV(PD6)|_BV(PD7);PORTD&=uint8_t(~(_BV(PD4)|_BV(PD5)|_BV(PD6)|_BV(PD7)));DDRD&=uint8_t(~_BV(PD2));PORTD|=_BV(PD2);DDRB|=_BV(PB0)|_BV(PB1);
#if MOTOR_HAS_STBY
 DDRB|=_BV(PB4);PORTB&=uint8_t(~_BV(PB4));
#endif
 PORTB&=uint8_t(~(_BV(PB0)|_BV(PB1)));TCCR0A=_BV(WGM00)|_BV(WGM01);TCCR0B=_BV(WGM02)|_BV(CS01)|_BV(CS00);OCR0A=249;OCR0B=0;TCNT0=0;TIFR0=_BV(OCF0A)|_BV(TOV0);TIMSK0=_BV(OCIE0A);
 TCCR1A=_BV(WGM11);TCCR1B=_BV(WGM13)|_BV(WGM12)|_BV(CS11);ICR1=39999;OCR1A=2500;TCNT1=0;TIMSK1=0;
 TCCR2A=_BV(WGM21);TCCR2B=_BV(CS22);OCR2A=124;TCNT2=0;TIMSK2=0;TIFR2=_BV(OCF2A);
 UCSR0A=0;UBRR0H=0;UBRR0L=103;UCSR0C=_BV(UCSZ01)|_BV(UCSZ00);UCSR0B=_BV(RXEN0)|_BV(TXEN0)|_BV(RXCIE0);sei();uint32_t saved=readSession();if(saved==0xffffffffUL)saved=0;engine.init(saved);hardwareOutputs();}
inline void loop(){uint32_t t=now();uint8_t down=(PIND&_BV(PD2))?0:1;if(down!=stopCandidate){stopCandidate=down;stopChanged=t;}if(down!=stopStable&&uint32_t(t-stopChanged)>=10){stopStable=down;engine.localButton(down);}
 engine.tick(t);if(rxFault){uint8_t saved=SREG;cli();rxFault=0;rxTail=rxHead;SREG=saved;parser.reset();engine.fault();}
 Proto::Frame frame,reply;
 if(txFree()>=32&&engine.takeReply(reply))send(reply);
 while(rxTail!=rxHead&&txFree()>=32){uint8_t b=rx[rxTail];rxTail=(rxTail+1)&127;uint8_t before=parser.used;bool complete=parser.push(b,frame);if(complete){bool has=engine.onFrame(frame,reply,t);hardwareOutputs();if(engine.takePersist())saveSession(engine.persistedSession());if(has)send(reply);}else if(before==31){engine.fault();}hardwareOutputs();}
 hardwareOutputs();}
}
ISR(TIMER0_COMPA_vect){++Actuator::milliseconds;}
ISR(TIMER2_COMPA_vect){PIND=_BV(PD6);}
ISR(USART_RX_vect){uint8_t status=UCSR0A,b=UDR0;if(status&(_BV(FE0)|_BV(DOR0)|_BV(UPE0))){Actuator::rxFault=1;return;}uint8_t next=(Actuator::rxHead+1)&127;if(next==Actuator::rxTail){Actuator::rxFault=1;return;}Actuator::rx[Actuator::rxHead]=b;Actuator::rxHead=next;}
ISR(USART_UDRE_vect){if(Actuator::txTail==Actuator::txHead){UCSR0B&=uint8_t(~_BV(UDRIE0));return;}UDR0=Actuator::tx[Actuator::txTail];Actuator::txTail=(Actuator::txTail+1)&63;}
