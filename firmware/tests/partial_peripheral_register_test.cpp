#include <cassert>
#include <cstdio>
#include "../peripheral_spi/src/board_app.h"
static Proto::Frame hello(){uint8_t p[9];Proto::put32(p,1);Proto::put32(p+4,Proto::MODEL_ID);p[8]=Proto::LABELS_VERSION;Proto::Frame f;Proto::make(f,Proto::HELLO,0,0,p,9);return f;}
static Proto::Frame policy(uint32_t ep,uint8_t mode){uint8_t p[9];Proto::put32(p,1);Proto::put32(p+4,ep);p[8]=mode;Proto::Frame f;Proto::make(f,Proto::SAFETY_POLICY,0,0,p,9);return f;}
static Proto::Frame stop(uint32_t epoch,uint16_t id){uint8_t p[10];Proto::put32(p,1);Proto::put32(p+4,epoch);Proto::put16(p+8,1);Proto::Frame f;Proto::make(f,Proto::STOP_ALL,id,0,p,10);return f;}
static Proto::Frame on(uint32_t epoch,uint16_t id){uint8_t p[14]={0};Proto::put32(p,1);Proto::put32(p+4,epoch);p[8]=3;p[9]=0;p[10]=3;p[11]=8;Proto::put16(p+12,500);Proto::Frame f;Proto::make(f,Proto::ACTUATE_BATCH,id,0,p,14);return f;}
static Proto::Frame transfer(const Proto::Frame& input,bool workDuring=false){using namespace PeripheralBoard;Proto::Frame result;PINB=0;PCINT0_vect();for(uint8_t i=0;i<32;i++){result.bytes[i]=SPDR;SPDR=input.bytes[i];SPI_STC_vect();if(workDuring)loop();}PINB=_BV(PB2);SPSR=0;PCINT0_vect();return result;}
static void send(const Proto::Frame& input){transfer(input);PeripheralBoard::loop();}
int main(){using namespace PeripheralBoard;memset(fakeEeprom,0xff,1024);PIND=_BV(PD2);PINB=_BV(PB2);setup();assert(alertLatch&&!(PORTD&(_BV(PD3)|_BV(PD4))));assert((DDRB&_BV(DDB4))&&!(DDRB&(_BV(DDB2)|_BV(DDB3)|_BV(DDB5))));assert(TCCR0A==_BV(WGM01)&&OCR0A==249);send(hello());send(policy(0,Proto::REQUIRE_ALERT));send(stop(1,1));assert(!alertLatch&&!engine.inhibited());send(on(1,2));assert(PORTD&_BV(PD4));
#if PERIPHERAL_ACTIVE_BUZZER
 assert(PORTD&_BV(PD3));assert(!TIMSK2);
#else
 assert(TIMSK2&_BV(OCIE2A));assert(OCR2A==124&&TCCR2B==_BV(CS22));
#endif
 PIND&=uint8_t(~_BV(PD2));INT0_vect();assert(!(PORTD&(_BV(PD3)|_BV(PD4)))&&!TIMSK2&&alertLatch);PIND|=_BV(PD2);loop();send(on(1,3));assert(engine.inhibited()&&!(PORTD&_BV(PD4)));send(stop(1,4));assert(alertLatch&&engine.inhibited());send(stop(2,5));assert(!alertLatch&&!engine.inhibited());send(on(2,6));assert(PORTD&_BV(PD4));
 // Dead TWI bus must time out while queued SPI frames continue to be accepted.
 ticks=40;loop();assert(lcd.busy());TWCR&=uint8_t(~_BV(TWINT));ticks=46;loop();assert(lcd.fault());send(stop(3,7));assert(engine.epoch()==3&&!engine.inhibited());send(on(3,8));assert(PORTD&_BV(PD4));Proto::Frame hb;uint8_t sid[4];Proto::put32(sid,1);Proto::make(hb,Proto::HEARTBEAT,20,0,sid,4);send(hb);Proto::Frame cached=replies[active],dummy;memset(&dummy,0xff,sizeof(dummy));Proto::Frame read=transfer(dummy,true);assert(!memcmp(read.bytes,cached.bytes,32)&&Proto::valid(read));assert(Proto::payload(read)[13]&16);loop();
 // ISR edge after command processing still gates foreground reapplication.
 PIND&=uint8_t(~_BV(PD2));INT0_vect();PIND|=_BV(PD2);outputs();assert(!(PORTD&_BV(PD4)));TIMER2_COMPA_vect();assert(!(PORTD&_BV(PD4)));ticks=2046;loop();assert(engine.inhibited()&&engine.session()==1);
 send(policy(4,Proto::BYPASS_ADAS));assert(engine.inhibited());PIND&=uint8_t(~_BV(PD2));send(stop(4,9));assert(!engine.inhibited()&&!alertLatch);send(on(4,10));assert(PORTD&_BV(PD4));INT0_vect();outputs();assert(PORTD&_BV(PD4));
#if PERIPHERAL_ACTIVE_BUZZER
 assert(PORTD&_BV(PD3));
#else
 assert(TIMSK2&_BV(OCIE2A));TIMER2_COMPA_vect();assert(TIMSK2&_BV(OCIE2A));
 uint8_t offHorn[12]={0};Proto::put32(offHorn,1);Proto::put32(offHorn+4,4);offHorn[8]=1;offHorn[9]=Proto::HORN_OFF;Proto::Frame offHornFrame;Proto::make(offHornFrame,Proto::ACTUATE_BATCH,11,0,offHorn,12);send(offHornFrame);TIMER2_COMPA_vect();assert((PORTD&_BV(PD4))&&!(PORTD&_BV(PD3))&&!TIMSK2);
#endif
 send(policy(5,Proto::REQUIRE_ALERT));assert(engine.inhibited()&&!(PORTD&_BV(PD4))&&!TIMSK2);send(stop(5,11));assert(engine.inhibited());PIND|=_BV(PD2);send(stop(6,12));assert(!engine.inhibited()&&!alertLatch&&!(PORTD&_BV(PD4)));send(on(6,13));assert(PORTD&_BV(PD4));PIND&=uint8_t(~_BV(PD2));INT0_vect();assert(!(PORTD&_BV(PD4))&&!TIMSK2);puts("PASS peripheral AVR pins/gate/sticky pulse/SPI immutability/LCD timeout");}
