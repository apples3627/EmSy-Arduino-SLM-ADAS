#include <cstdio>
#define F_CPU 16000000UL
#ifndef ACTOR_BOARD_HEADER
#define ACTOR_BOARD_HEADER "../../actuator_uart/src/board_app.h"
#endif
#include ACTOR_BOARD_HEADER
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void hello(){uint8_t p[9];Proto::put32(p,100);Proto::put32(p+4,Proto::MODEL_ID);p[8]=3;Proto::Frame q,r;Proto::make(q,Proto::HELLO,1,0,p,9);Actuator::engine.onFrame(q,r,0);}
static Proto::Frame stop(uint16_t id,uint32_t epoch){uint8_t p[10];Proto::put32(p,100);Proto::put32(p+4,epoch);Proto::put16(p+8,0);Proto::Frame q;Proto::make(q,Proto::STOP_ALL,id,0,p,10);return q;}
static Proto::Frame batch(uint16_t id,uint32_t epoch){uint8_t p[15];Proto::put32(p,100);Proto::put32(p+4,epoch);p[8]=4;p[9]=Proto::FORWARD;p[10]=Proto::LIGHT_ON;p[11]=Proto::WIPER_ON;p[12]=Proto::HORN_ON;Proto::put16(p+13,500);Proto::Frame q;Proto::make(q,Proto::ACTUATE_BATCH,id,0,p,15);return q;}
static bool off(){return !(TCCR0A&_BV(COM0B1))&&!(PORTD&(_BV(PD4)|_BV(PD5)|_BV(PD6)|_BV(PD7)))&&!(PORTB&(_BV(PB0)|_BV(PB1)|_BV(PB4)))&&!(TCCR1A&_BV(COM1A1))&&!TIMSK2;}
int main(){PIND=_BV(PD2)|_BV(PD3);Actuator::adasDanger=0;Actuator::engine.init(0);hello();Proto::Frame r;auto q=stop(2,1);Actuator::engine.onFrame(q,r,1);q=batch(3,1);CHECK(Actuator::engine.onFrame(q,r,2));Actuator::hardwareOutputs();CHECK(TCCR0A&_BV(COM0B1));CHECK(PORTB&_BV(PB0));CHECK(TCCR1A&_BV(COM1A1));CHECK(TIMSK2);
 PIND&=~_BV(PD3);INT1_vect();CHECK(off());CHECK(Actuator::engine.currentSession()==100);Actuator::hardwareOutputs();CHECK(off());
 // MAIN/foreground stalls throughout a short low pulse: release must not erase stop.
 PIND|=_BV(PD3);INT1_vect();Actuator::hardwareOutputs();CHECK(off());Actuator::safety();CHECK(Actuator::engine.safetyInhibited());CHECK(Actuator::engine.outputs().drive==0);Actuator::safety();CHECK(Actuator::engine.safetyInhibited());q=batch(4,1);Actuator::engine.onFrame(q,r,3);CHECK(Proto::op(r)==Proto::ERROR_REPLY);q=stop(5,1);Actuator::engine.onFrame(q,r,4);CHECK(Actuator::engine.safetyInhibited());q=stop(6,2);Actuator::engine.onFrame(q,r,5);CHECK(!Actuator::engine.safetyInhibited());Actuator::hardwareOutputs();CHECK(off());q=batch(7,2);Actuator::engine.onFrame(q,r,6);Actuator::hardwareOutputs();CHECK(TCCR0A&_BV(COM0B1));Actuator::engine.tick(506);Actuator::hardwareOutputs();CHECK(Actuator::engine.outputs().drive==0);CHECK(Actuator::engine.outputs().lights&&Actuator::engine.outputs().wiper&&Actuator::engine.outputs().horn);
 // A reversal request must remain neutral and cannot survive an IRQ.
 q=batch(8,2);Actuator::engine.onFrame(q,r,507);uint8_t rev[11];Proto::put32(rev,100);Proto::put32(rev+4,2);rev[8]=Proto::REVERSE;Proto::put16(rev+9,500);Proto::make(q,Proto::ACTUATE,9,0,rev,11);CHECK(!Actuator::engine.onFrame(q,r,508));Actuator::hardwareOutputs();CHECK(!(TCCR0A&_BV(COM0B1)));PIND&=~_BV(PD3);INT1_vect();CHECK(off());Actuator::safety();Actuator::engine.tick(608);CHECK(!Actuator::engine.outputs().drive&&!Actuator::engine.outputs().wiper&&!Actuator::engine.outputs().horn);CHECK(!Actuator::engine.takeReply(r));PIND|=_BV(PD3);INT1_vect();Actuator::safety();q=stop(10,3);Actuator::engine.onFrame(q,r,609);q=batch(11,3);Actuator::engine.onFrame(q,r,610);rev[8]=Proto::LIGHT_OFF;Proto::put32(rev+4,3);Proto::make(q,Proto::ACTUATE,11,0,rev,11);Actuator::engine.onFrame(q,r,611);CHECK(Proto::op(r)==Proto::ERROR_REPLY&&!Actuator::engine.outputs().drive);
 printf("PASS actor actual-register gate %u assertions\n",checks);
}
