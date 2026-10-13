#define F_CPU 16000000UL
#include MAIN_BOARD_HEADER
#include "frame_helpers.h"
static bool off(){return !(TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))&&!(PORTB&(_BV(PB1)|_BV(PB2)))&&!(PORTD&(_BV(PD4)|_BV(PD7)))&&!(PORTC&3);}
int main(){using namespace MainIO;PIND=_BV(PIND2);DDRC=0xff;MCUCR=_BV(PUD);setup();CHECK(off());CHECK(!(DDRC&0x30)&&(PORTC&0x30)==0x30&&!(MCUCR&_BV(PUD)));CHECK(DDRB&_BV(PB2));CHECK(TCCR1A==_BV(WGM10)&&TIMSK1==0);CHECK(TWBR==255&&TWSR==0);CHECK(PCMSK2==_BV(PCINT22));
 adasGateRequired=false;motorWrite(1,true);CHECK((TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))==(_BV(COM1A1)|_BV(COM1B1)));CHECK(OCR1A==MainConfig::MOTOR_DUTY&&OCR1B==MainConfig::MOTOR_DUTY);CHECK((PORTD&(_BV(PD4)|_BV(PD7)))==_BV(PD4));CHECK((PORTC&3)==1);PCINT2_vect();CHECK(!off());motorWrite(-1,true);CHECK((PORTD&(_BV(PD4)|_BV(PD7)))==_BV(PD7)&&(PORTC&3)==2);motorWrite(1,false);CHECK(off());
 PIND|=_BV(PIND6);adasDanger=adasEvent=0;adasGateRequired=true;motorWrite(1,true);CHECK(!off());uint8_t pwm=TCCR1A;Proto::Frame f;Proto::make(f,Proto::STATUS,1);MainLinks::begin(1,f,0);CHECK(!(PORTB&_BV(PB0))&&(DDRB&_BV(PB2))&&TCCR1A==pwm);MainLinks::finish(1,false);CHECK((PORTB&_BV(PB0))&&(DDRB&_BV(PB2))&&TCCR1A==pwm);
 PIND&=uint8_t(~_BV(PIND6));PCINT2_vect();CHECK(off()&&adasDanger);PIND|=_BV(PIND6);PCINT2_vect();motorWrite(1,true);CHECK(off());adasDanger=0;motorWrite(1,true);CHECK(!off());PIND&=uint8_t(~_BV(PIND6));motorWrite(1,true);CHECK(off());
 // Raw LOW cannot resume an old command when the logical controller forbids motion.
 adasGateRequired=false;motorWrite(1,false);CHECK(off());
 CHECK(TCCR0A==_BV(WGM01)&&OCR0A==249&&TIMSK0==_BV(OCIE0A));CHECK(TCCR2A==0&&TCCR2B==_BV(CS21)&&OCR2B==208&&TIMSK2==_BV(OCIE2B));TIMER0_COMPA_vect();CHECK(ticks==1);
 puts("PASS actual MAIN registers: optional LOW bypass, active immediate/sticky ISR gate, both motor channels/directions and independent SPI/timers");}
