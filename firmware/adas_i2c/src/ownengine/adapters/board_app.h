#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "worker.h"
#include "transport.h"
#include "capture.h"
namespace BoardApp {
Adas::Worker worker;Compute::Receiver receiver;Compute::TransmitCursor transmitter;
Proto::Frame replies[2],outgoing;
volatile uint8_t active=0,reading=0;
volatile uint32_t ticks=0;
bool outgoingPending=false,waiting=false;uint32_t triggered=0,lastTrigger=0;
uint32_t now(){uint8_t s=SREG;cli();uint32_t n=ticks;SREG=s;return n;}
// Open drain: PORT latch stays LOW, only direction changes.
void alert(){PORTD&=~_BV(PORTD4);if(worker.policy.sources.asserted())DDRD|=_BV(DDD4);else DDRD&=~_BV(DDD4);}
bool publish(){if(!outgoingPending)return true;uint8_t next=active^1;replies[next]=outgoing;uint8_t s=SREG;cli();bool idle=!reading;if(idle)active=next;SREG=s;if(idle)outgoingPending=false;return idle;}
void setup(){cli();alert();worker.status(replies[0],0);replies[1]=replies[0];TCCR0A=_BV(WGM01);TCCR0B=_BV(CS01)|_BV(CS00);OCR0A=249;TCNT0=0;TIMSK0=_BV(OCIE0A);Capture::setup();DDRC&=~(_BV(DDC4)|_BV(DDC5));PORTC&=~(_BV(PORTC4)|_BV(PORTC5));TWAR=0x24;TWCR=_BV(TWEN)|_BV(TWEA)|_BV(TWIE);lastTrigger=uint32_t(0)-AdasConfig::PERIOD_MS;sei();}
void loop(){
 uint32_t time=now(),width;
 if(Capture::take(width)){waiting=false;worker.policy.sample(width,time);}
 if(waiting&&uint32_t(time-triggered)>=AdasConfig::TIMEOUT_MS){Capture::cancel();waiting=false;worker.policy.fault();}
 if(!waiting&&uint32_t(time-lastTrigger)>=AdasConfig::PERIOD_MS){lastTrigger=triggered=time;if(Capture::arm())waiting=true;else worker.policy.fault();}
 worker.policy.tick(time);alert();
 if(!publish())return;
 Proto::Frame request;
 if(receiver.queue.pop(request)){
  worker.handle(request,outgoing,time);
  if(Proto::valid(request)&&Proto::op(request)==Proto::HELLO&&Proto::len(request)==4){uint8_t s=SREG;cli();receiver.queue.drain();SREG=s;}
  outgoingPending=true;publish();
 }
}
}
ISR(TIMER0_COMPA_vect){BoardApp::ticks++;}
ISR(TWI_vect){
 using namespace BoardApp;
 switch(TWSR&0xf8){
 case 0x60: case 0x68: receiver.begin();break;
 case 0x80: case 0x90: receiver.byte(TWDR);break;
 case 0xa0: receiver.finish(false);break;
 case 0xa8: case 0xb0: reading=1;TWDR=transmitter.begin(replies[active]);break;
 case 0xb8: TWDR=transmitter.next(replies[active]);break;
 case 0xc0: case 0xc8: reading=0;break;
 case 0x00: reading=0;TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN)|_BV(TWEA)|_BV(TWIE);return;
 default:break;
 }
 TWCR=_BV(TWINT)|_BV(TWEN)|_BV(TWEA)|_BV(TWIE);
}
