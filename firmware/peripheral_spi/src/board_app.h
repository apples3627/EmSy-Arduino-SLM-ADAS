#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include "engine.h"
#include "transport.h"
#include "lcd.h"
static_assert(F_CPU==16000000UL,"Peripheral requires Uno 16 MHz");
static_assert(E2END==1023,"Session persistence requires Uno EEPROM");
namespace PeripheralBoard {
Peripheral::Engine engine;Peripheral::Lcd lcd;PeripheralTransport::Receiver receiver;PeripheralTransport::TransmitCursor transmitter;
Proto::Frame replies[2],outgoing;volatile uint8_t active=0,reading=0,alertLatch=1,alertGeneration=0;volatile uint32_t ticks=0;bool outgoingPending=false;
uint32_t eepromSession=0;uint8_t eepromIndex=4;
inline uint32_t now(){uint8_t saved=SREG;cli();uint32_t value=ticks;SREG=saved;return value;}
inline bool alertLow(){return !(PIND&_BV(PD2));}
inline void gateOff(){PORTD&=uint8_t(~(_BV(PD3)|_BV(PD4)));TIMSK2=0;}
inline void outputs(){uint8_t saved=SREG;cli();if((engine.requiresAlert()&&(alertLatch||alertLow()))||engine.inhibited())gateOff();else {if(engine.outputs().lights)PORTD|=_BV(PD4);else PORTD&=uint8_t(~_BV(PD4));
#if PERIPHERAL_ACTIVE_BUZZER
 TIMSK2=0;if(engine.outputs().horn)PORTD|=_BV(PD3);else PORTD&=uint8_t(~_BV(PD3));
#else
 if(engine.outputs().horn)TIMSK2=_BV(OCIE2A);else {TIMSK2=0;PORTD&=uint8_t(~_BV(PD3));}
#endif
 }SREG=saved;}
inline uint32_t readSession(){uint32_t value=0;for(uint8_t i=0;i<4;i++){EEAR=1020+i;EECR|=_BV(EERE);value|=uint32_t(EEDR)<<(8*i);}return value==0xffffffffUL?0:value;}
// Write at most one EEPROM byte per loop, without waiting for the 3.4 ms write.
inline void persistStep(){if(eepromIndex>=4||(EECR&_BV(EEPE)))return;EEAR=1020+eepromIndex;EECR|=_BV(EERE);uint8_t value=uint8_t(eepromSession>>(8*eepromIndex));eepromIndex++;if(EEDR==value)return;EEDR=value;uint8_t saved=SREG;cli();EECR|=_BV(EEMPE);EECR|=_BV(EEPE);SREG=saved;}
inline bool publish(){if(!outgoingPending)return true;uint8_t next=active^1;replies[next]=outgoing;uint8_t saved=SREG;cli();bool idle=!reading;if(idle){active=next;SPDR=replies[active].bytes[0];}SREG=saved;if(idle)outgoingPending=false;return idle;}
inline void safety(){uint8_t saved=SREG;cli();bool low=alertLow(),event=alertLatch;SREG=saved;engine.adas(low,event);}
inline void setup(){cli();DDRD|=_BV(PD3)|_BV(PD4);DDRD&=uint8_t(~_BV(PD2));PORTD|=_BV(PD2);gateOff();engine.init(readSession());
 TCCR0A=_BV(WGM01);TCCR0B=_BV(CS01)|_BV(CS00);OCR0A=249;TCNT0=0;TIMSK0=_BV(OCIE0A);
 TCCR2A=_BV(WGM21);TCCR2B=_BV(CS22);OCR2A=124;TCNT2=0;TIMSK2=0;
 EICRA=_BV(ISC01);EIFR=_BV(INTF0);EIMSK=_BV(INT0);
 DDRB=uint8_t((DDRB|_BV(DDB4))&~(_BV(DDB2)|_BV(DDB3)|_BV(DDB5)));PORTB|=_BV(PB2);SPCR=_BV(SPE)|_BV(SPIE);SPSR=0;
 engine.status(replies[0],Proto::STATUS_REPLY,0);replies[1]=replies[0];SPDR=replies[0].bytes[0];PCMSK0=_BV(PCINT2);PCIFR=_BV(PCIF0);PCICR|=_BV(PCIE0);lcd.setup(0);sei();}
inline void loop(){safety();uint32_t t=now();engine.tick(t);outputs();
 if(receiver.queue.overflow){uint8_t saved=SREG;cli();receiver.queue.drain();SREG=saved;engine.inhibit(Proto::COMM_FAULT);gateOff();}
 if(publish()){Proto::Frame request;if(receiver.queue.pop(request)){
  safety();uint8_t generation=alertGeneration;uint32_t before=engine.epoch();engine.handle(request,outgoing,t);
  // Only a successfully accepted strictly newer STOP can consume a sticky edge.
  uint8_t saved=SREG;cli();bool release=Proto::valid(request)&&Proto::op(request)==Proto::STOP_ALL&&Proto::op(outgoing)==Proto::ACK&&engine.epoch()>before&&!engine.inhibited()&&(!engine.requiresAlert()||(!alertLow()&&generation==alertGeneration));if(release)alertLatch=0;bool raced=engine.requiresAlert()&&(alertLatch||alertLow());SREG=saved;
  if(raced){safety();if(Proto::op(outgoing)==Proto::ACK&&Proto::len(outgoing)==18)engine.status(outgoing,Proto::ACK,Proto::id(request));}
  outputs();if(engine.takePersist()){eepromSession=engine.persistedSession();eepromIndex=0;}outgoingPending=true;publish();
 }}
 persistStep();lcd.display(engine,t);lcd.poll(t);engine.lcdFault(lcd.fault());outputs();
}
}
ISR(TIMER0_COMPA_vect){PeripheralBoard::ticks++;}
ISR(TIMER2_COMPA_vect){if(!PeripheralBoard::engine.outputs().horn){TIMSK2=0;PORTD&=uint8_t(~_BV(PD3));return;}if(PeripheralBoard::engine.inhibited()||(PeripheralBoard::engine.requiresAlert()&&(PeripheralBoard::alertLatch||PeripheralBoard::alertLow()))){PeripheralBoard::gateOff();return;}PIND=_BV(PD3);}
ISR(INT0_vect){if(PeripheralBoard::engine.requiresAlert())PeripheralBoard::gateOff();PeripheralBoard::alertLatch=1;PeripheralBoard::alertGeneration++;}
ISR(PCINT0_vect){using namespace PeripheralBoard;if(!(PINB&_BV(PB2))){reading=1;receiver.begin();SPDR=transmitter.begin(replies[active]);}else {if(SPSR&_BV(SPIF)){uint8_t b=SPDR;receiver.byte(b);}receiver.finish(true);reading=0;SPDR=replies[active].bytes[0];}}
ISR(SPI_STC_vect){using namespace PeripheralBoard;uint8_t b=SPDR;if(reading){receiver.byte(b);SPDR=transmitter.next(replies[active]);}else SPDR=replies[active].bytes[0];}
