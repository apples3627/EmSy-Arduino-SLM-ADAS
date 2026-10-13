#pragma once
#include "protocol.h"
#include "control_engine.h"
#include "register_io.h"
#include "compute_links.h"
namespace MainBoard {
MainVehicle::Controller controller;Proto::StreamParser parser;
uint32_t adasPollAt=0,adasSyncAt=0,adasRevision=0;bool adasReady=false,adasNeedHello=true;
inline void safety(uint32_t now){uint8_t s=SREG;cli();bool sticky=MainIO::adasDanger;bool low=MainIO::adasLow();MainIO::adasDanger=low;MainIO::adasEvent=0;SREG=s;controller.alert(low,sticky&&!low,now);}
inline void receive(uint32_t now){using namespace MainIO;if(softOverflow){softOverflow=0;rr=rw;parser.reset();controller.stop(Proto::COMM_FAULT,now);}while(rr!=rw){uint8_t b=rx[rr];rr=(rr+1)&63;Proto::Frame f;if(parser.push(b,f))controller.acceptUART(f,now);}}
inline void adas(uint32_t now){auto& link=MainLinks::bus[0];MainLinks::service(0,now);if(link.done){link.done=false;bool ok=link.ok&&controller.acceptADAS(link.response,link.request,now);if(Proto::op(link.request)==Proto::HELLO){adasReady=ok;adasNeedHello=!ok;}if(Proto::op(link.request)==Proto::STATE_SYNC&&ok){adasRevision=Proto::get32(Proto::payload(link.request)+4);adasSyncAt=now;}}
 if(link.phase)return;Proto::Frame f;
 if(adasNeedHello){uint8_t p[4];Proto::put32(p,controller.state.session);Proto::make(f,Proto::HELLO,0,0,p,4);MainLinks::begin(0,f,now);return;}
 if(uint32_t(now-adasPollAt)>=70){Proto::make(f,Proto::STATUS,0);MainLinks::begin(0,f,now);adasPollAt=now;return;}
 if(adasReady&&(adasRevision!=controller.state.revision||uint32_t(now-adasSyncAt)>=500)){uint8_t p[22];Proto::encodeState(p,controller.state);Proto::make(f,Proto::STATE_SYNC,0,0,p,22);MainLinks::begin(0,f,now);}}
inline void peripheral(uint32_t now){auto& link=MainLinks::bus[1];MainLinks::service(1,now);if(link.done){link.done=false;if(link.ok)controller.acceptPeripheral(link.response,link.request,now);}if(!link.phase){Proto::Frame f;if(controller.periph.pop(f))MainLinks::begin(1,f,now);}}
inline void transmit(){Proto::Frame f;if(uint8_t((MainIO::xr-MainIO::xw-1)&63)>=32&&controller.uart.pop(f))MainIO::actorSend(f);}
inline void setup(){MainIO::setup();uint32_t saved=MainIO::readSession();if(saved==0xffffffffUL)saved=0;uint32_t session=saved+1;if(!session||session==0xffffffffUL){MainIO::gateOff();for(;;){}}MainIO::persist(session);controller.init(session,MainIO::now());}
inline void loop(){uint32_t now=MainIO::now();safety(now);receive(now);safety(now);adas(now);controller.tick(now);peripheral(now);safety(now);MainIO::motorWrite(controller.motorDirection,controller.periphSafe&&controller.adasClear&&!controller.needsStop());transmit();}
}
