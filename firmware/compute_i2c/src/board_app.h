#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#define INTENT_PART 1
#define WORKER_LO 0
#define WORKER_ROLE 1
#include "neural_worker.h"
#include "transport.h"
namespace BoardApp {
Compute::Worker worker;
Compute::Receiver receiver;
Proto::Frame replies[2],outgoing;
volatile uint8_t active=0,reading=0;
Compute::TransmitCursor transmitter;
volatile uint32_t ticks=0;
bool outgoingPending=false;
bool publish(){
 if(!outgoingPending)return true;
 uint8_t next=active^1;
 replies[next]=outgoing;
 uint8_t saved=SREG;cli();
 bool idle=!reading;
 if(idle)active=next;
 SREG=saved;
 if(idle)outgoingPending=false;
 return idle;
}
void timerSetup(){TCCR0A=_BV(WGM01);TCCR0B=_BV(CS01)|_BV(CS00);OCR0A=249;TCNT0=0;TIMSK0=_BV(OCIE0A);}
void setup(){
 cli();DDRD|=_BV(DDD4);PORTD&=~_BV(PORTD4);
 worker.meta(replies[0]);replies[1]=replies[0];timerSetup();
 DDRC&=~(_BV(DDC4)|_BV(DDC5));PORTC&=~(_BV(PORTC4)|_BV(PORTC5));TWAR=0x24;TWCR=_BV(TWEN)|_BV(TWEA)|_BV(TWIE);
 sei();
}
void loop(){
 if(!publish())return;
 Proto::Frame request;
 if(receiver.queue.pop(request)){
  worker.handle(request,outgoing);
  if(Proto::valid(request)&&Proto::op(request)==Proto::HELLO){uint8_t saved=SREG;cli();receiver.queue.drain();SREG=saved;}
  outgoingPending=true;if(!publish())return;
 }
 if(worker.pending){PORTD|=_BV(PORTD4);worker.computeIfPending();PORTD&=~_BV(PORTD4);worker.status(outgoing,worker.job);outgoingPending=true;publish();}
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
