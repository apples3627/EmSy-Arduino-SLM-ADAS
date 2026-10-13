#include <cassert>
#include <cstdio>
#define F_CPU 16000000UL
#include PERIPHERAL_BOARD_HEADER
#include "frame_helpers.h"
static void exchange(const Proto::Frame& q){memcpy(PeripheralBoard::receiver.queue.incoming(),q.bytes,32);assert(PeripheralBoard::receiver.queue.publish(32));PeripheralBoard::loop();assert(Proto::valid(PeripheralBoard::replies[PeripheralBoard::active]));}
static void spiFrame(const Proto::Frame& q){PINB&=uint8_t(~_BV(PB2));PCINT0_vect();for(uint8_t b:q.bytes){SPDR=b;SPI_STC_vect();}PINB|=_BV(PB2);SPSR=0;PCINT0_vect();PeripheralBoard::loop();}
int main(){using namespace PeripheralBoard;memset(fakeEeprom,0xff,sizeof(fakeEeprom));PIND=_BV(PD2);PINB=_BV(PB2);setup();assert((DDRD&(_BV(PD3)|_BV(PD4)))==(_BV(PD3)|_BV(PD4)));assert(!(DDRD&_BV(PD2)));
 exchange(hello());exchange(stop(2,1));assert(!alertLatch&&!engine.inhibited());exchange(batch(3,1,{Proto::LIGHT_ON,Proto::HORN_ON}));assert(PORTD&_BV(PD4));
#if PERIPHERAL_ACTIVE_BUZZER
 assert(PORTD&_BV(PD3));
#else
 assert(TIMSK2&_BV(OCIE2A));
#endif
 PIND&=uint8_t(~_BV(PD2));INT0_vect();assert(!(PORTD&(_BV(PD3)|_BV(PD4)))&&!TIMSK2);PIND|=_BV(PD2);outputs();assert(!(PORTD&(_BV(PD3)|_BV(PD4)))&&!TIMSK2);safety();assert(engine.inhibited());exchange(stop(4,1));assert(alertLatch&&engine.inhibited());exchange(stop(5,2));assert(!alertLatch&&!engine.inhibited());exchange(batch(6,2,{Proto::LIGHT_ON,Proto::HORN_ON}));assert(PORTD&_BV(PD4));
 // A stuck TWINT transaction expires in five ms; SPI still handles requests.
 ticks=40;TWCR=0;loop();assert(lcd.busy());TWCR=0;ticks=45;loop();assert(lcd.fault()&&!lcd.busy());Proto::Frame status;uint8_t p[4];Proto::put32(p,100);Proto::make(status,Proto::STATUS,7,0,p,4);spiFrame(status);auto reply=replies[active];assert(Proto::op(reply)==Proto::STATUS_REPLY&&Proto::id(reply)==7&&(Proto::payload(reply)[13]&16));assert(PORTD&_BV(PD4));
 spiFrame(batch(8,2,{Proto::LIGHT_OFF,Proto::HORN_OFF}));assert(Proto::op(replies[active])==Proto::ACK&&!(PORTD&(_BV(PD3)|_BV(PD4)))&&!TIMSK2);
 puts("PASS actual peripheral ISR registers: sticky pulse, recovery STOP, active/passive buzzer and LCD timeout with SPI alive");
}
