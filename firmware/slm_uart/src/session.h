#pragma once
#include "protocol.h"
namespace SlmUart {
inline bool newer32(uint32_t a,uint32_t b){return int32_t(a-b)>0;}
inline bool elapsed(uint32_t now,uint32_t then,uint32_t ms){return uint32_t(now-then)>=ms;}
// Portable session core. A pending frame is immutable through retries.
struct Session {
 Proto::SharedState shared; Proto::Frame pendingFrame;
 uint32_t session,generation,snapshotAt,linkAt,sentAt; uint16_t counter;
 uint8_t diagnostic[24];bool diagnosticKnown=false;uint32_t diagnosticAt=0;
 uint8_t i2cDiagnostic[20];bool i2cDiagnosticKnown=false;uint32_t i2cDiagnosticAt=0;
 uint8_t retries; bool known,pending,sendDue,appliedEvent,errorEvent,stopNeeded;
 Session():shared{},session(0),generation(0),snapshotAt(0),linkAt(0),sentAt(0),counter(0),retries(0),known(false),pending(false),sendDue(false),appliedEvent(false),errorEvent(false),stopNeeded(false){}
 bool fresh(uint32_t now)const{return session&&known&&!elapsed(now,snapshotAt,2000)&&!elapsed(now,linkAt,2000);}
 bool ready(uint32_t now)const{return fresh(now)&&shared.appliedKnown&&!(shared.actorFlags&(4|8|Proto::BUSY_FLAG))&&!pending&&!stopNeeded;}
 uint32_t epoch()const{return shared.safetyEpoch;}
 void cancelBatch(){if(pending&&Proto::op(pendingFrame)==Proto::ACTUATE_BATCH){pending=sendDue=false;errorEvent=true;stopNeeded=true;}}
 void fail(){++generation;cancelBatch();known=false;stopNeeded=true;errorEvent=true;}
 bool nextId(uint16_t& id){if(counter==0xffff){errorEvent=true;return false;}id=++counter;return true;}
 bool issueStop(uint32_t now){if(!session)return false;uint16_t id;if(!nextId(id))return false;uint8_t p[10];Proto::put32(p,session);Proto::put32(p+4,epoch());Proto::put16(p+8,Proto::SLM_STOP);Proto::make(pendingFrame,Proto::STOP_ALL,id,0,p,10);pending=sendDue=true;stopNeeded=false;retries=0;sentAt=now;++generation;return true;}
 bool issuePlan(const uint8_t* intents,uint8_t count,uint32_t now){if(!ready(now)||!count||count>4)return false;for(uint8_t i=0;i<count;i++)if(!Proto::supportedIntent(intents[i])||intents[i]==Proto::ALL_STOP)return false;uint16_t id;if(!nextId(id))return false;uint8_t p[15];Proto::put32(p,session);Proto::put32(p+4,epoch());p[8]=count;memcpy(p+9,intents,count);Proto::put16(p+9+count,0);Proto::make(pendingFrame,Proto::ACTUATE_BATCH,id,0,p,11+count);pending=sendDue=true;retries=0;sentAt=now;return true;}
 void sent(uint32_t now){sendDue=false;sentAt=now;}
 void tick(uint32_t now){if(known&&!fresh(now)){fail();}if(pending&&!sendDue&&elapsed(now,sentAt,300)){if(retries<2){++retries;sendDue=true;}else{pending=false;errorEvent=true;stopNeeded=true;known=false;++generation;}}}
 void status(Proto::Frame& out,const Proto::Frame& query)const{uint8_t p[18]={0};Proto::put32(p,session);Proto::put32(p+4,epoch());p[8]=uint8_t(shared.appliedDrive);p[9]=(shared.appliedFlags&1)!=0;p[10]=0;p[11]=(shared.appliedFlags&4)!=0;p[13]=shared.actorFlags;if(!known||!shared.appliedKnown)p[13]|=8;Proto::put16(p+14,shared.command);Proto::put16(p+16,shared.stopSources);Proto::make(out,Proto::STATUS_REPLY,Proto::id(query),Proto::seq(query),p,18);}
 // true means reply is initialized. ACK acceptance ignores a prior busy snapshot.
 bool receive(const Proto::Frame& f,uint32_t now,Proto::Frame& reply){if(!Proto::valid(f))return false;uint32_t sid;const uint8_t* p=Proto::payload(f);
  if(Proto::op(f)==Proto::HELLO){if(!Proto::helloRequest(f,sid)){Proto::ack(reply,f,Proto::BAD_MODEL);return true;}if(sid!=session){session=sid;shared={};shared.session=sid;known=pending=sendDue=stopNeeded=false;counter=0;diagnosticKnown=i2cDiagnosticKnown=false;appliedEvent=errorEvent=false;++generation;}linkAt=now;Proto::slmMeta(reply,session,Proto::id(f));return true;}
  if(!session)return false;
  if(Proto::op(f)==Proto::I2C_DIAGNOSTIC){if(Proto::len(f)==20&&!Proto::seq(f)&&Proto::get32(p)==session&&!newer32(epoch(),Proto::get32(p+4))){memcpy(i2cDiagnostic,p,20);i2cDiagnosticKnown=true;i2cDiagnosticAt=now;}return false;}
  if(Proto::op(f)==Proto::DIAGNOSTIC){if(Proto::len(f)==24&&!Proto::seq(f)&&Proto::get32(p)==session&&!newer32(epoch(),Proto::get32(p+4))){memcpy(diagnostic,p,24);diagnosticKnown=true;diagnosticAt=now;}return false;}
  if(Proto::op(f)==Proto::STATE_SYNC){Proto::SharedState state;if(!Proto::decodeState(f,state)||state.session!=session)return false;if(known&&(newer32(shared.revision,state.revision)||newer32(epoch(),state.safetyEpoch)))return false;if(state.safetyEpoch!=epoch()){++generation;cancelBatch();}shared=state;known=true;snapshotAt=linkAt=now;return false;}
  if((Proto::op(f)==Proto::STATUS||Proto::op(f)==Proto::HEARTBEAT)&&Proto::len(f)==4&&Proto::get32(p)==session){linkAt=now;status(reply,f);return true;}
  if(Proto::op(f)==Proto::ERROR_REPLY&&Proto::len(f)==2&&pending&&Proto::id(f)==Proto::id(pendingFrame)&&Proto::seq(f)==Proto::seq(pendingFrame)&&p[0]==Proto::op(pendingFrame)){pending=sendDue=false;errorEvent=true;stopNeeded=true;++generation;return false;}
  if(Proto::op(f)!=Proto::ACK||Proto::len(f)!=18||!pending||Proto::id(f)!=Proto::id(pendingFrame)||Proto::seq(f)!=Proto::seq(pendingFrame)||Proto::get32(p)!=session||Proto::get16(p+14)!=Proto::id(pendingFrame)||p[10]||p[9]>1||p[11]>1||int8_t(p[8])<-1||int8_t(p[8])>1)return false;
  uint32_t e=Proto::get32(p+4),requested=Proto::get32(Proto::payload(pendingFrame)+4);bool stop=Proto::op(pendingFrame)==Proto::STOP_ALL;
  if(stop?(!newer32(e,requested)||p[8]||p[9]||p[11]):(e!=requested||(p[13]&1)))return false;
  if(newer32(epoch(),e))return false;if(e!=epoch())++generation;shared.safetyEpoch=e;shared.appliedDrive=int8_t(p[8]);shared.appliedFlags=(p[9]?1:0)|(p[11]?4:0);shared.command=Proto::get16(p+14);shared.stopSources=Proto::get16(p+16);shared.appliedKnown=true;shared.actorFlags=p[13];known=true;snapshotAt=linkAt=now;pending=sendDue=false;appliedEvent=true;return false;
 }
};
}
