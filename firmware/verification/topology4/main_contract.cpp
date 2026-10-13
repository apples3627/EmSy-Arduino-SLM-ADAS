#include <cassert>
#include <cstdio>
#define F_CPU 16000000UL
#include MAIN_BOARD_HEADER
static bool off(){return !(TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))&&!(PORTB&(_BV(PB1)|_BV(PB2)))&&!(PORTD&(_BV(PD4)|_BV(PD7)))&&!(PORTC&(_BV(PC0)|_BV(PC1)));}
int main(){using namespace MainIO;PIND=_BV(PIND6)|_BV(PIND2);DDRC=0xff;PORTC=0;MCUCR=_BV(PUD);setup();assert(!(DDRC&0x30)&&(PORTC&0x30)==0x30&&!(MCUCR&_BV(PUD)));assert(TWBR==255&&TWSR==0);adasDanger=adasEvent=0;assert(DDRB&_BV(PB2));assert(PCMSK2==_BV(PCINT22));assert(TCCR1A==_BV(WGM10)&&OCR1A==0&&OCR1B==0&&TIMSK1==0);assert(TCCR0A==_BV(WGM01)&&OCR0A==249&&TIMSK0==_BV(OCIE0A));assert(TCCR2A==0&&TCCR2B==_BV(CS21)&&OCR2B==208&&TIMSK2==_BV(OCIE2B));
 motorWrite(1,true);assert((TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))==(_BV(COM1A1)|_BV(COM1B1)));assert(OCR1A==MainConfig::MOTOR_DUTY&&OCR1B==MainConfig::MOTOR_DUTY);assert((PORTD&(_BV(PD4)|_BV(PD7)))==_BV(PD4));assert((PORTC&3)==_BV(PC0));
 uint8_t pwm=TCCR1A;Proto::Frame f;Proto::make(f,Proto::STATUS,1);MainLinks::begin(1,f,0);assert(!(PORTB&_BV(PB0))&&(DDRB&_BV(PB2))&&TCCR1A==pwm);MainLinks::finish(1,false);assert((PORTB&_BV(PB0))&&(DDRB&_BV(PB2))&&TCCR1A==pwm);
 motorWrite(-1,true);assert((PORTD&(_BV(PD4)|_BV(PD7)))==_BV(PD7));assert((PORTC&3)==_BV(PC1));motorWrite(-1,false);assert(off());motorWrite(1,true);PIND&=uint8_t(~_BV(PIND6));PCINT2_vect();assert(off()&&adasDanger);PIND|=_BV(PIND6);PCINT2_vect();motorWrite(1,true);assert(off());MainBoard::safety(1);assert(off()&&MainBoard::controller.motorDirection==0&&MainBoard::controller.needsStop());
 assert(!(DDRC&0x30)&&(PORTC&0x30)==0x30&&!(MCUCR&_BV(PUD)));
 // TX compare advances modulo Timer2; RX first samples the start bit at half time.
 uint8_t timer1=TCCR1A;assert(actorSend(f));uint8_t prior=OCR2B;TIMER2_COMPB_vect();assert(OCR2B==uint8_t(prior+208)&&!(PORTD&_BV(PORTD3)));assert(TCCR1A==timer1);TCNT2=250;rxBit=255;INT0_vect();assert(OCR2A==uint8_t(250+104)&&rxBit==0);PIND&=uint8_t(~_BV(PIND2));TIMER2_COMPA_vect();assert(rxBit==1);uint8_t value=0xa5;for(int i=0;i<8;i++){if(value&(1<<i))PIND|=_BV(PIND2);else PIND&=uint8_t(~_BV(PIND2));TIMER2_COMPA_vect();}PIND|=_BV(PIND2);TIMER2_COMPA_vect();assert(rxBit==255&&rw==1&&rx[0]==value&&!softOverflow);assert(TCCR1A==timer1);TIMER0_COMPA_vect();assert(ticks==1);
 puts("PASS actual MAIN register gating: two PWM enables, both direction pairs, independent D8 CS/D10 SS, Timer0/1/2 separation and sticky ALERT ISR");
}
