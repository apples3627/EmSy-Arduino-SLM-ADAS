#include <cassert>
#include <cstdio>
#define F_CPU 16000000UL
#include "../main/src/board_app.h"
static bool off(){return !(TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))&&!(PORTB&(_BV(PB1)|_BV(PB2)))&&!(PORTD&(_BV(PD4)|_BV(PD7)))&&!(PORTC&3);}
int main(){using namespace MainIO;PIND=_BV(PIND2);DDRC=255;PORTC=0;MCUCR=_BV(PUD);setup();assert(!(DDRC&0x30)&&(PORTC&0x30)==0x30&&!(MCUCR&_BV(PUD))&&TWBR==255&&TWSR==0);assert(DDRB&_BV(PB2));assert(adasGateRequired==0&&adasLow());motorWrite(1,true);assert(!off()&&OCR1A==255&&OCR1B==255);PCINT2_vect();assert(adasDanger&&!off());motorWrite(-1,true);assert((PORTD&(_BV(PD4)|_BV(PD7)))==_BV(PD7)&&(PORTC&3)==_BV(PC1));adasGateRequired=1;PCINT2_vect();assert(off());PIND|=_BV(PIND6);PCINT2_vect();motorWrite(1,true);assert(off());adasDanger=0;motorWrite(1,true);assert(!off());PIND&=uint8_t(~_BV(PIND6));motorWrite(1,true);assert(off());adasGateRequired=0;motorWrite(1,true);assert(!off());motorWrite(1,false);assert(off());puts("PASS MAIN real registers: bypass rawLOW, active sticky/currentLOW immediate two-channel gate, retained pullups/TWI/SS");}
