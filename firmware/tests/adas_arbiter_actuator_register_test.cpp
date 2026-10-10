#define F_CPU 16000000UL
#define E2END 1023
#include "main_fake_avr/avr/io.h"
enum {PD2=2,PD3=3,PD4=4,PD5=5,PD6=6,PD7=7,PB0=0,PB1=1,PB4=4,COM0B1=5,COM0B0=4,COM1A1=7,COM1A0=6,WGM00=0,WGM02=3,WGM11=1,WGM13=4,CS22=2,WGM21=1,ISC10=2,INTF1=1,INT1=1,OCF0A=1,TOV0=0,OCF2A=1,OCIE2A=1};
inline volatile uint8_t OCR0B,TIFR0,TCCR2A,TCCR2B,OCR2A,TCNT2,TIMSK2,TIFR2,UBRR0H,UBRR0L;
inline volatile uint16_t ICR1;
#include "../actuator_uart/src/board_app.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
static Proto::Frame request(uint8_t op,uint16_t id,uint32_t epoch,uint8_t intent=0){Proto::Frame f;uint8_t p[15]={};Proto::put32(p,1);uint8_t n=4;if(op==Proto::HELLO){Proto::put32(p+4,Proto::MODEL_ID);p[8]=3;n=9;}else if(op==Proto::STOP_ALL){Proto::put32(p+4,epoch);Proto::put16(p+8,Proto::ADAS_OBSTACLE);n=10;}else if(op==Proto::ACTUATE){Proto::put32(p+4,epoch);p[8]=intent;Proto::put16(p+9,500);n=11;}Proto::make(f,op,id,0,p,n);return f;}
int main(){using namespace Actuator;Proto::Frame out;PIND=_BV(PD3);adasDanger=adasEvent=0;engine.init(0);engine.onFrame(request(Proto::HELLO,0,0),out,0);engine.onFrame(request(Proto::ACTUATE,1,0),out,1);assert(Proto::payload(out)[1]==Proto::SAFETY_INHIBIT);engine.onFrame(request(Proto::STOP_ALL,1,1),out,2);for(uint8_t i: {uint8_t(Proto::FORWARD),uint8_t(Proto::LIGHT_ON),uint8_t(Proto::WIPER_ON),uint8_t(Proto::HORN_ON)})engine.onFrame(request(Proto::ACTUATE,2+i,1,i),out,3+i);
 // Commands are monotonic: configure all accessory outputs with fresh IDs.
 engine.onFrame(request(Proto::ACTUATE,20,1,Proto::FORWARD),out,20);hardwareOutputs();assert(engine.outputs().drive==1&&(TCCR0A&_BV(COM0B1))&&(PORTB&_BV(PB0))&&(TCCR1A&_BV(COM1A1))&&TIMSK2);
 // A falling INT1 gates every register immediately even while foreground is stalled.
 PIND&=uint8_t(~_BV(PD3));INT1_vect();assert(!(TCCR0A&(_BV(COM0B1)|_BV(COM0B0)))&&!(PORTD&(_BV(PD4)|_BV(PD5)|_BV(PD6)|_BV(PD7)))&&!(PORTB&(_BV(PB0)|_BV(PB1)|_BV(PB4)))&&!(TCCR1A&(_BV(COM1A1)|_BV(COM1A0)))&&!TIMSK2);
 // Rising before foreground must not erase the safety event or restore cached motion.
 PIND|=_BV(PD3);INT1_vect();assert(adasDanger);hardwareOutputs();assert(!(TCCR0A&_BV(COM0B1)));safety();safety();assert(!engine.outputs().drive&&engine.safetyInhibited());engine.onFrame(request(Proto::ACTUATE,21,1),out,30);assert(Proto::payload(out)[1]==Proto::SAFETY_INHIBIT);engine.onFrame(request(Proto::STOP_ALL,22,1),out,31);assert(engine.safetyInhibited());engine.onFrame(request(Proto::STOP_ALL,23,2),out,32);assert(!engine.safetyInhibited());engine.onFrame(request(Proto::ACTUATE,24,1),out,33);assert(Proto::payload(out)[1]==Proto::STALE_SAFETY&&!engine.outputs().drive);engine.onFrame(request(Proto::ACTUATE,25,2),out,34);hardwareOutputs();assert(engine.outputs().drive==1&&(TCCR0A&_BV(COM0B1)));
 // Alert in the middle of an otherwise valid whole UART frame rejects the completed old actuation.
 Proto::StreamParser stream;auto frame=request(Proto::ACTUATE,26,2,Proto::REVERSE);for(uint8_t i=0;i<16;i++)assert(!stream.push(frame.bytes[i],out));PIND&=uint8_t(~_BV(PD3));INT1_vect();safety();for(uint8_t i=16;i<32;i++){Proto::Frame in;if(stream.push(frame.bytes[i],in))engine.onFrame(in,out,35);}assert(Proto::payload(out)[1]==Proto::SAFETY_INHIBIT&&!engine.outputs().drive);assert(engine.currentSession()==1);
 puts("ACT INT1 register gate + sticky pulse + partial UART safety epoch: PASS");
}
