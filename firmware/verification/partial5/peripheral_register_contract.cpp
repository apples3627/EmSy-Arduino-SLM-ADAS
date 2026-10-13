#define F_CPU 16000000UL
#include PERIPHERAL_BOARD_HEADER
#include "frame_helpers.h"
static Proto::Frame exchange(const Proto::Frame& q){using namespace PeripheralBoard;memcpy(receiver.queue.incoming(),q.bytes,32);CHECK(receiver.queue.publish(32));loop();CHECK(Proto::valid(replies[active]));return replies[active];}
static bool off(){return !(PORTD&(_BV(PD3)|_BV(PD4)))&&!TIMSK2;}
static bool on(){
#if PERIPHERAL_ACTIVE_BUZZER
 return (PORTD&(_BV(PD3)|_BV(PD4)))==(_BV(PD3)|_BV(PD4));
#else
 return (PORTD&_BV(PD4))&&(TIMSK2&_BV(OCIE2A));
#endif
}
int main(){using namespace PeripheralBoard;memset(fakeEeprom,0xff,sizeof(fakeEeprom));PIND=0;PINB=_BV(PB2);setup();CHECK(off());exchange(hello());exchange(stop(2,1));exchange(batch(3,1,{3,8}));CHECK(off());exchange(policy(4,2,1));exchange(batch(5,1,{3,8}));CHECK(off());exchange(stop(6,2));CHECK(!engine.inhibited()&&!alertLatch);exchange(batch(7,2,{3,8}));CHECK(on());INT0_vect();CHECK(on());outputs();CHECK(on());
 exchange(policy(8,3,0));CHECK(off()&&engine.inhibited());exchange(stop(9,3));CHECK(off()&&engine.inhibited());PIND|=_BV(PD2);exchange(stop(10,4));CHECK(off()&&!engine.inhibited()&&!alertLatch);exchange(batch(11,4,{3,8}));CHECK(on());PIND&=uint8_t(~_BV(PD2));INT0_vect();CHECK(off());PIND|=_BV(PD2);outputs();CHECK(off());safety();CHECK(engine.inhibited());exchange(stop(12,4));CHECK(off()&&engine.inhibited());exchange(stop(13,5));CHECK(off()&&!engine.inhibited());exchange(batch(14,5,{3,8}));CHECK(on());
 // LCD TWI timeout cannot block the SPI request handler or falsify auxiliary outputs.
 ticks=40;TWCR=0;loop();CHECK(lcd.busy());TWCR=0;ticks=45;loop();CHECK(lcd.fault()&&!lcd.busy());exchange(batch(15,5,{4,9}));CHECK(off());
#if !PERIPHERAL_ACTIVE_BUZZER
 TIMER2_COMPA_vect();CHECK(off());exchange(batch(16,5,{3}));CHECK(PORTD&_BV(PD4));TIMER2_COMPA_vect();CHECK(PORTD&_BV(PD4));
#endif
 puts("PASS actual peripheral registers: boot/policy/STOP barrier, LOW bypass vs active ISR, sticky recovery and LCD timeout");}
