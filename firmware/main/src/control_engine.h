#pragma once
#include "protocol.h"
namespace MainVehicle {
struct Queue {Proto::Frame frames[3];uint8_t count;Queue():count(0){}void clear(){count=0;}bool push(const Proto::Frame& f){if(count==3)return false;frames[count++]=f;return true;}bool pop(Proto::Frame& f){if(!count)return false;f=frames[0];for(uint8_t i=1;i<count;i++)frames[i-1]=frames[i];--count;return true;}};
class Controller {
 uint32_t now_,uAt,pAt,pollAt,heartbeatAt,syncAt,pSyncAt,adasAt,sampleAt,reversalAt,driveAt;uint16_t sample,lastU,pCounter,duration;uint8_t pRetries,adasFlags,adasCount;uint16_t adasAge;bool sampleKnown,adasMeta,adasHealthy,physicalLow,blocked,needStop,pWaiting,haveLastU,uReplyValid;Proto::Frame pRequest,uRequest,uReply;uint8_t actions[4],actionCount;int8_t stagedDrive;uint8_t stagedFlags;
 static bool elapsed(uint32_t now,uint32_t at,uint32_t ms){return uint32_t(now-at)>=ms;}
 void u(const Proto::Frame& f,bool critical=false){if(critical)uart.clear();uart.push(f);}
 void helloU(){uint8_t p[9];Proto::put32(p,state.session);Proto::put32(p+4,Proto::MODEL_ID);p[8]=Proto::LABELS_VERSION;Proto::Frame f;Proto::make(f,Proto::HELLO,0,0,p,9);u(f);heartbeatAt=now_;}
 void requestP(const Proto::Frame& f){pRequest=f;pWaiting=true;pRetries=0;pAt=now_;periph.push(f);}
 uint16_t nextP(){if(!++pCounter)++pCounter;return pCounter;}
 void helloP(){uint8_t p[9];Proto::put32(p,state.session);Proto::put32(p+4,Proto::MODEL_ID);p[8]=Proto::LABELS_VERSION;Proto::Frame f;Proto::make(f,Proto::HELLO,0,0,p,9);requestP(f);}
 void flags(){uint8_t f=(blocked?4:0)|(periphSafe?0:8)|(pending?16:0)|(reversing?1:0);if(f!=state.actorFlags){state.actorFlags=f;state.revision++;syncAt=pSyncAt=now_-500;}ready=slmReady&&periphReady&&periphSafe&&!blocked&&!pending&&!needStop&&state.appliedKnown;}
 void changed(){state.revision++;flags();syncAt=now_-500;pSyncAt=now_-500;}
 void statusU(uint8_t op,uint16_t id){uint8_t p[18]={};Proto::put32(p,state.session);Proto::put32(p+4,state.safetyEpoch);p[8]=uint8_t(state.appliedDrive);p[9]=state.appliedFlags&1?1:0;p[11]=state.appliedFlags&4?1:0;p[13]=state.actorFlags;Proto::put16(p+14,id);Proto::put16(p+16,state.stopSources);Proto::make(uReply,op,id,0,p,18);uReplyValid=true;u(uReply,true);}
 void errorU(const Proto::Frame& q,uint8_t err){Proto::Frame f;Proto::ack(f,q,err);u(f,true);}
 bool freshADAS()const{return adasMeta&&adasHealthy&&sampleKnown&&!physicalLow&&(adasFlags&8)&&!(adasFlags&23)&&adasCount>=3&&uint32_t(adasAge)+uint32_t(now_-adasAt)<=200&&uint32_t(now_-sampleAt)<=200;}
 void sendStopP(){uint8_t p[10];Proto::put32(p,state.session);Proto::put32(p+4,state.safetyEpoch);Proto::put16(p+8,state.stopSources);Proto::Frame f;Proto::make(f,Proto::STOP_ALL,nextP(),0,p,10);requestP(f);}
 void sendBatchP(){uint8_t p[15];Proto::put32(p,state.session);Proto::put32(p+4,state.safetyEpoch);p[8]=actionCount;for(uint8_t i=0;i<actionCount;i++)p[9+i]=actions[i];Proto::put16(p+9+actionCount,duration);Proto::Frame f;Proto::make(f,Proto::ACTUATE_BATCH,nextP(),0,p,uint8_t(11+actionCount));requestP(f);}
 public:
 Proto::SharedState state;Queue uart,periph;int8_t motorDirection;bool pending,reversing,ready,adasClear,slmReady,periphReady,periphSafe;uint8_t peripheralFlags;uint32_t generation;
 Controller(){init(1,0);}
 void init(uint32_t session,uint32_t now){now_=uAt=pAt=pollAt=heartbeatAt=syncAt=pSyncAt=adasAt=sampleAt=reversalAt=driveAt=now;sample=lastU=pCounter=0;duration=500;pRetries=adasFlags=adasCount=0;adasAge=0xffff;sampleKnown=adasMeta=adasHealthy=haveLastU=uReplyValid=false;physicalLow=blocked=needStop=true;pWaiting=false;state={session,1,1,0,0,0,0,0,Proto::BOOT_STOP,false,12};motorDirection=0;pending=reversing=ready=adasClear=slmReady=periphReady=periphSafe=false;peripheralFlags=12;generation=0;actionCount=0;stagedDrive=0;stagedFlags=0;uart.clear();periph.clear();helloU();helloP();}
 void stop(uint16_t sources,uint32_t now){now_=now;motorDirection=0;pending=reversing=false;generation++;state.desiredDrive=0;state.desiredFlags=0;state.appliedKnown=false;state.stopSources=sources;if(state.safetyEpoch!=0xffffffffUL)state.safetyEpoch++;periphSafe=false;needStop=true;bool keepHello=!periphReady&&pWaiting&&Proto::op(pRequest)==Proto::HELLO;if(!keepHello){periph.clear();pWaiting=false;}changed();}
 void alert(bool low,bool sticky,uint32_t now){now_=now;physicalLow=low;if(sticky||(low&&!blocked)){blocked=true;adasClear=false;stop(Proto::ADAS_OBSTACLE,now);}flags();}
 bool acceptUART(const Proto::Frame& f,uint32_t now){now_=now;if(!Proto::valid(f)||Proto::seq(f))return false;
  if(Proto::op(f)==Proto::SLM_META){if(!Proto::slmCompatible(f,state.session))return false;slmReady=true;uAt=now;changed();return true;}
  const uint8_t* p=Proto::payload(f);
  if(Proto::op(f)==Proto::STATUS_REPLY){if(Proto::len(f)!=18||Proto::get32(p)!=state.session||Proto::get32(p+4)!=state.safetyEpoch)return false;uAt=now;return true;}
  if(Proto::op(f)==Proto::ACK||Proto::op(f)==Proto::ERROR_REPLY)return false;
  uint8_t op=Proto::op(f);if(op!=Proto::STOP_ALL&&op!=Proto::ACTUATE_BATCH){errorU(f,Proto::BAD_OP);stop(Proto::COMM_FAULT,now);return false;}
  if(Proto::len(f)<4||Proto::get32(p)!=state.session){errorU(f,Proto::BAD_SESSION);return false;}
  if((op==Proto::STOP_ALL&&Proto::len(f)!=10)||(op==Proto::ACTUATE_BATCH&&(Proto::len(f)<12||p[8]<1||p[8]>4||Proto::len(f)!=11+p[8]))){errorU(f,Proto::BAD_LENGTH);stop(Proto::COMM_FAULT,now);return false;}
  uint16_t id=Proto::id(f);if(!id){errorU(f,Proto::OLD_COMMAND);return false;}
  if(haveLastU&&id==lastU){if(memcmp(f.bytes,uRequest.bytes,32)){errorU(f,Proto::BAD_OP);stop(Proto::COMM_FAULT,now);return false;}if(pending||(!uReplyValid&&needStop&&op==Proto::STOP_ALL))return true;if(uReplyValid&&Proto::get32(Proto::payload(uReply)+4)==state.safetyEpoch&&Proto::op(uReply)==Proto::ACK)u(uReply,true);else errorU(f,Proto::STALE_SAFETY);return true;}
  if(haveLastU&&!Proto::newer16(id,lastU)){errorU(f,Proto::OLD_COMMAND);return false;}
  uAt=now;
  if(op==Proto::STOP_ALL){if(Proto::get32(p+4)>state.safetyEpoch){errorU(f,Proto::STALE_SAFETY);return false;}lastU=id;haveLastU=true;uRequest=f;uReplyValid=false;state.command=id;stop(Proto::SLM_STOP,now);return true;}
  if(Proto::get32(p+4)!=state.safetyEpoch){errorU(f,Proto::STALE_SAFETY);return false;}
  if(!ready){errorU(f,Proto::SAFETY_INHIBIT);return false;}
  uint16_t seen=0;bool invalid=false,unsupported=false,globalStop=false;for(uint8_t i=0;i<p[8];i++){uint8_t v=p[9+i];if(v>9)invalid=true;else{if(!Proto::supportedIntent(v))unsupported=true;seen|=uint16_t(1)<<v;if(v==2)globalStop=true;}}
  bool conflict=((seen&3)==3)||((seen&24)==24)||((seen&768)==768);
  lastU=id;haveLastU=true;uRequest=f;uReplyValid=false;state.command=id;
  if(invalid||(!globalStop&&(unsupported||conflict))){stop(Proto::SLM_STOP,now);errorU(f,Proto::BAD_OP);return false;}
  if(globalStop){stop(Proto::SLM_STOP,now);return true;}
  stagedDrive=state.appliedDrive;stagedFlags=state.appliedFlags;
  if(seen&1)stagedDrive=1;if(seen&2)stagedDrive=-1;if(seen&8)stagedFlags|=1;if(seen&16)stagedFlags&=uint8_t(~1);if(seen&256)stagedFlags|=4;if(seen&512)stagedFlags&=uint8_t(~4);
  duration=Proto::get16(p+9+p[8]);if(!duration)duration=500;if(duration>1000)duration=1000;actionCount=p[8];for(uint8_t i=0;i<actionCount;i++)actions[i]=p[9+i];
  pending=true;state.desiredDrive=stagedDrive;state.desiredFlags=stagedFlags;state.appliedKnown=false;state.stopSources=0;reversing=stagedDrive&&motorDirection&&stagedDrive!=motorDirection;if(reversing){motorDirection=0;reversalAt=now;}changed();return true;
 }
 bool acceptPeripheral(const Proto::Frame& f,const Proto::Frame& request,uint32_t now){now_=now;if(!Proto::valid(f))return false;
  if(Proto::op(request)==Proto::HELLO&&Proto::peripheralCompatible(f,state.session)){periphReady=true;pWaiting=false;pollAt=now;return true;}
  if(!pWaiting||memcmp(request.bytes,pRequest.bytes,32)||Proto::id(f)!=Proto::id(request))return false;
  if(Proto::op(f)==Proto::ERROR_REPLY){if(Proto::len(f)==2&&Proto::payload(f)[0]==Proto::op(request)){pWaiting=false;periphReady=false;stop(Proto::COMM_FAULT,now);}return false;}
  if(Proto::op(request)==Proto::STATE_SYNC){bool ok=Proto::op(f)==Proto::ACK&&Proto::len(f)==2&&Proto::payload(f)[0]==Proto::STATE_SYNC&&!Proto::payload(f)[1];if(ok){pWaiting=false;pSyncAt=now;}return ok;}
  const uint8_t* p=Proto::payload(f);if(Proto::len(f)!=18||Proto::get32(p)!=state.session||Proto::get32(p+4)!=state.safetyEpoch||p[8]||p[10]||p[12])return false;
  if(Proto::op(request)==Proto::STATUS){if(Proto::op(f)!=Proto::STATUS_REPLY)return false;pWaiting=false;pollAt=now;peripheralFlags=p[13];if((p[13]&12)&&!needStop){stop(Proto::COMM_FAULT,now);return true;}return true;}
  if(Proto::op(f)!=Proto::ACK||Proto::get16(p+14)!=Proto::id(request)||(p[13]&1))return false;
  pWaiting=false;pollAt=now;peripheralFlags=p[13];
  if(Proto::op(request)==Proto::STOP_ALL){if(p[9]||p[11]){stop(Proto::COMM_FAULT,now);return false;}motorDirection=0;state.appliedDrive=0;state.appliedFlags=0;state.appliedKnown=true;periphSafe=!(p[13]&12);needStop=!periphSafe;changed();if(haveLastU&&Proto::op(uRequest)==Proto::STOP_ALL)statusU(Proto::ACK,lastU);return true;}
  if(Proto::op(request)!=Proto::ACTUATE_BATCH||!pending||blocked||(p[13]&12)){stop(Proto::COMM_FAULT,now);return false;}
  uint8_t bits=(p[9]?1:0)|(p[11]?4:0);if(bits!=stagedFlags){stop(Proto::COMM_FAULT,now);return false;}
  motorDirection=stagedDrive;if(seenMotion()){driveAt=now;}state.appliedDrive=motorDirection;state.appliedFlags=bits;state.appliedKnown=true;pending=reversing=false;changed();statusU(Proto::ACK,lastU);return true;
 }
 bool seenMotion()const{for(uint8_t i=0;i<actionCount;i++)if(actions[i]<2)return true;return false;}
 bool acceptADAS(const Proto::Frame& f,const Proto::Frame& request,uint32_t now){now_=now;if(!Proto::valid(f)||Proto::id(f)!=Proto::id(request)||Proto::seq(f))return false;const uint8_t* p=Proto::payload(f);
  if(Proto::op(request)==Proto::HELLO){adasMeta=Proto::op(f)==Proto::ADAS_META&&Proto::len(f)==8&&p[0]==Proto::ADAS_ROLE&&p[1]==1&&p[2]==20&&p[3]==25&&p[4]==3&&p[5]==Proto::VERSION&&Proto::get16(p+6)==70;return adasMeta;}
  if(Proto::op(request)==Proto::STATE_SYNC)return Proto::op(f)==Proto::ACK&&Proto::len(f)==2&&p[0]==Proto::STATE_SYNC&&!p[1];
  if(Proto::op(request)!=Proto::STATUS)return false;
  adasHealthy=Proto::op(f)==Proto::ADAS_STATUS_REPLY&&Proto::len(f)==18&&p[0]==Proto::ADAS_ROLE&&p[8]==20&&p[9]==25&&p[11]==3&&Proto::get16(p+12)==70;
  if(!adasHealthy)return false;adasFlags=p[1];adasAge=Proto::get16(p+6);adasCount=p[10];adasAt=now;uint16_t n=Proto::get16(p+4);if(!sampleKnown||Proto::newer16(n,sample)){sample=n;sampleKnown=true;sampleAt=now;}return true;
 }
 void tick(uint32_t now){now_=now;bool safe=freshADAS();adasClear=safe;if(!safe&&!blocked){blocked=true;uint16_t source=(adasFlags&2)?Proto::ADAS_OBSTACLE:Proto::ADAS_SENSOR_FAULT;stop(source,now);}else if(safe&&blocked){blocked=false;stop(Proto::BOOT_STOP,now);}
  if(slmReady&&elapsed(now,uAt,2000)){slmReady=false;stop(Proto::COMM_FAULT,now);}
  if(motorDirection&&elapsed(now,driveAt,duration)){motorDirection=0;state.desiredDrive=state.appliedDrive=0;state.stopSources=Proto::DRIVE_LEASE;changed();}
  if(pWaiting&&elapsed(now,pAt,300)){if(pRetries<2){if(periph.push(pRequest)){pRetries++;pAt=now;}}else{pWaiting=false;periphReady=false;stop(Proto::COMM_FAULT,now);}}
  if(!pWaiting){if(!periphReady)helloP();else if(needStop){if(!blocked||!state.appliedKnown||elapsed(now,pollAt,300))sendStopP();}else if(pending){if(!reversing||elapsed(now,reversalAt,100)){reversing=false;sendBatchP();}}else if(elapsed(now,pollAt,100)){uint8_t p[4];Proto::put32(p,state.session);Proto::Frame f;Proto::make(f,Proto::STATUS,nextP(),0,p,4);requestP(f);}else if(elapsed(now,pSyncAt,500)){uint8_t p[22];Proto::encodeState(p,state);Proto::Frame f;Proto::make(f,Proto::STATE_SYNC,nextP(),0,p,22);requestP(f);}}
  flags();if(elapsed(now,heartbeatAt,250)){if(!slmReady)helloU();else{uint8_t p[22];Proto::encodeState(p,state);Proto::Frame f;Proto::make(f,Proto::STATE_SYNC,0,0,p,22);u(f);Proto::put32(p,state.session);Proto::make(f,Proto::STATUS,0,0,p,4);u(f);heartbeatAt=now;}}
  if(slmReady&&elapsed(now,syncAt,500)){uint8_t p[22];Proto::encodeState(p,state);Proto::Frame f;Proto::make(f,Proto::STATE_SYNC,0,0,p,22);u(f);syncAt=now;}
 }
 const Proto::Frame& pendingPeripheral()const{return pRequest;}bool awaitingPeripheral()const{return pWaiting;}bool needsStop()const{return needStop;}uint16_t latestSample()const{return sample;}uint8_t sensorFlags()const{return adasFlags;}
};
}
