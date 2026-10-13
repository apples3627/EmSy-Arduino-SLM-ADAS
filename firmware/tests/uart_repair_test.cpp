#include <cassert>
#include <cstdio>
#include "../main/src/board_app.h"
extern "C" void repaired_peer_init(uint32_t);
extern "C" void repaired_peer_exchange(const uint8_t*,uint8_t*,uint32_t);
int main(){
 using namespace MainBoard;using namespace MainIO;
 controller.init(7,0);PIND=_BV(PIND6);adasDanger=0;Proto::Frame q,r;assert(controller.periph.pop(q));assert(Proto::op(q)==Proto::HELLO);repaired_peer_init(42);repaired_peer_exchange(q.bytes,r.bytes,0);assert(Proto::op(r)==Proto::ERROR_REPLY&&Proto::len(r)==6&&Proto::get32(Proto::payload(r)+2)==42);assert(!controller.acceptPeripheral(r,q,0));assert(controller.rotateSessionRequested&&controller.proposedSession==43&&!controller.motorDirection);
 // Recovery reaches real EEPROM and register gate, not just the portable controller.
 TCCR1A=_BV(COM1A1)|_BV(COM1B1);PORTB=_BV(PB1)|_BV(PB2);PORTD|=_BV(PD4)|_BV(PD7);PORTC|=_BV(PC0)|_BV(PC1);ticks=1000;recoverSession(1000);assert(controller.state.session==43&&readSession()==43);assert(!(TCCR1A&(_BV(COM1A1)|_BV(COM1B1)))&&!(PORTB&(_BV(PB1)|_BV(PB2)))&&!(PORTD&(_BV(PD4)|_BV(PD7)))&&!(PORTC&(_BV(PC0)|_BV(PC1))));assert(!controller.ready&&!controller.motorDirection);assert(controller.periph.pop(q));repaired_peer_exchange(q.bytes,r.bytes,1000);assert(controller.acceptPeripheral(r,q,1000)&&controller.periphReady);
 // New HELLO must not rotate again from an old cached rejection with a smaller hint.
 controller.init(50,2000);assert(controller.periph.pop(q));uint8_t stale[6]={Proto::HELLO,Proto::BAD_SESSION};Proto::put32(stale+2,42);Proto::make(r,Proto::ERROR_REPLY,0,0,stale,6);assert(!controller.acceptPeripheral(r,q,2000));assert(!controller.rotateSessionRequested&&controller.awaitingPeripheral());
 // Diagnostics count real receive errors and never grant drive readiness.
 controller.uart.clear();receivedBytes=99;receiveErrors=2;uartFrames=3;spiBadFrames=4;controller.note(Proto::UART_RX_ERROR);diagnosticAt=0;diagnostic(3000);assert(controller.uart.pop(r));assert(Proto::valid(r)&&Proto::op(r)==Proto::DIAGNOSTIC&&Proto::len(r)==24);const uint8_t* p=Proto::payload(r);assert(Proto::get32(p)==50&&p[9]==Proto::UART_RX_ERROR&&Proto::get16(p+12)==99&&Proto::get16(p+14)==3&&Proto::get16(p+16)==2&&Proto::get16(p+18)==4);assert(!controller.ready&&!controller.motorDirection);
 // Complete frames wrap and publish only after staging; ring capacity stays bounded.
 Proto::make(q,Proto::STATUS,123);xw=xr=63;assert(actorSend(q)&&xw==31);for(uint8_t i=0;i<32;i++)assert(tx[(63+i)&63]==q.bytes[i]);assert(!actorSend(q));
 puts("PASS UART repair: persisted higher-session recovery, motor gate, stale-hint rejection, diagnostics and ring wrap");
}
