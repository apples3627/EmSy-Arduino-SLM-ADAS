#pragma once
#include "policy.h"
#include "snapshot.h"
namespace Adas {
struct Worker {
 Policy policy; Node::Snapshot snapshot;
 void meta(Proto::Frame& f,uint16_t id=0,uint8_t seq=0){uint8_t p[8]={Proto::ADAS_ROLE,Proto::HC_SR04,AdasConfig::STOP_CM,AdasConfig::RELEASE_CM,AdasConfig::CLEAR_REQUIRED,Proto::VERSION,0,0};Proto::put16(p+6,AdasConfig::PERIOD_MS);Proto::make(f,Proto::ADAS_META,id,seq,p,8);}
 void status(Proto::Frame& f,uint32_t now,uint16_t id=0,uint8_t seq=0){uint8_t p[18]={Proto::ADAS_ROLE,policy.flags(),0,0,0,0,0,0,AdasConfig::STOP_CM,AdasConfig::RELEASE_CM,policy.clearCount,AdasConfig::CLEAR_REQUIRED};Proto::put16(p+2,policy.rangeMm);Proto::put16(p+4,policy.sampleId);Proto::put16(p+6,policy.age(now));Proto::put16(p+12,AdasConfig::PERIOD_MS);Proto::put32(p+14,snapshot.known?snapshot.shared.revision:0);Proto::make(f,Proto::ADAS_STATUS_REPLY,id,seq,p,18);}
 void handle(const Proto::Frame& q,Proto::Frame& r,uint32_t now){
  if(!Proto::valid(q)){Proto::ack(r,q,Proto::BAD_FRAME);return;}
  switch(Proto::op(q)){
  case Proto::HELLO:if(Proto::len(q)!=4){Proto::ack(r,q,Proto::BAD_LENGTH);return;}snapshot.hello(Proto::get32(Proto::payload(q)));meta(r,Proto::id(q),Proto::seq(q));return;
  case Proto::STATUS:if(Proto::len(q)){Proto::ack(r,q,Proto::BAD_LENGTH);return;}status(r,now,Proto::id(q),Proto::seq(q));return;
  case Proto::STATE_SYNC:Proto::ack(r,q,snapshot.accept(q));return;
  default:Proto::ack(r,q,Proto::BAD_OP);return;
  }
 }
};
}
