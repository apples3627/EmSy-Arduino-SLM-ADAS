#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#define INTENT_PART 2
#define WORKER_LO 32
#define WORKER_ROLE 2
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
 DDRB=uint8_t((DDRB|_BV(DDB4))&~(_BV(DDB2)|_BV(DDB3)|_BV(DDB5)));PORTB|=_BV(PORTB2);SPCR=_BV(SPE)|_BV(SPIE);SPSR=0;SPDR=replies[active].bytes[0];PCMSK0=_BV(PCINT2);PCIFR=_BV(PCIF0);PCICR|=_BV(PCIE0);
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
ISR(PCINT0_vect){
 using namespace BoardApp;
 if(!(PINB&_BV(PINB2))){reading=1;receiver.begin();SPDR=transmitter.begin(replies[active]);}
 else {
  // Consume a completed final byte if PCINT wins priority over SPI_STC.
  if(SPSR&_BV(SPIF)){uint8_t b=SPDR;receiver.byte(b);}
  receiver.finish(true);reading=0;SPDR=replies[active].bytes[0];
 }
}
ISR(SPI_STC_vect){
 using namespace BoardApp;uint8_t b=SPDR;
 if(reading){receiver.byte(b);SPDR=transmitter.next(replies[active]);}
 else SPDR=replies[active].bytes[0];
}
