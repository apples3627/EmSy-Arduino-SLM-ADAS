#pragma once
#include "protocol.h"
namespace Peripheral {
struct Outputs {bool lights,horn;};
class Engine {
 Outputs state_; uint32_t sid_,highest_,epoch_,contact_; uint16_t last_,sources_; bool accepted_,sync_,adas_,lcdFault_,persist_; Proto::Frame previous_; Proto::SharedState snapshot_; bool snapshotValid_; Proto::SensorTelemetry sensor_; uint32_t sensorAt_,policyEpoch_,clock_; uint8_t mode_; bool sensorValid_,policySet_;
 void off(){state_.lights=state_.horn=false;}
 bool error(const Proto::Frame& req,Proto::Frame& out,uint8_t err,bool stop=true){if(stop)off();Proto::ack(out,req,err);return true;}
 public:
 Engine(){init(0);}
 void init(uint32_t saved){sid_=epoch_=contact_=0;highest_=saved;last_=0;sources_=Proto::BOOT_STOP;accepted_=adas_=lcdFault_=persist_=snapshotValid_=false;sync_=true;policySet_=sensorValid_=false;mode_=Proto::REQUIRE_ALERT;policyEpoch_=sensorAt_=clock_=0;off();memset(&snapshot_,0,sizeof(snapshot_));memset(&sensor_,0,sizeof(sensor_));}
 const Outputs& outputs()const{return state_;} uint32_t session()const{return sid_;} uint32_t epoch()const{return epoch_;} bool requiresAlert()const{return !policySet_||mode_==Proto::REQUIRE_ALERT;} bool bypass()const{return policySet_&&mode_==Proto::BYPASS_ADAS;} bool policyConfigured()const{return policySet_;} bool inhibited()const{return sync_||adasActive();} bool syncRequired()const{return sync_;} bool adasActive()const{return !policySet_||(requiresAlert()&&adas_);}
 bool takePersist(){bool p=persist_;persist_=false;return p;} uint32_t persistedSession()const{return highest_;}
 const Proto::SharedState& snapshot()const{return snapshot_;} bool hasSnapshot()const{return snapshotValid_;}
 void lcdFault(bool v){lcdFault_=v;}
 void inhibit(uint16_t source){off();sync_=true;sources_|=source;}
 void adas(bool active,bool event=false){adas_=active;if(requiresAlert()&&(active||event))inhibit(Proto::ADAS_OBSTACLE);}
 uint32_t clock()const{return clock_;}
 const Proto::SensorTelemetry& sensor()const{return sensor_;} bool hasSensor()const{return sensorValid_;}
 bool sensorStale(uint32_t now)const{return sensorValid_&&(sensor_.epoch!=epoch_||(sensor_.flags&Proto::SENSOR_STALE)||uint32_t(now-sensorAt_)>200||uint32_t(sensor_.ageMs)+uint32_t(now-sensorAt_)>200);}
 void tick(uint32_t now){clock_=now;if(sid_&&uint32_t(now-contact_)>=2000)inhibit(Proto::COMM_FAULT);}
 void status(Proto::Frame& out,uint8_t op,uint16_t id){uint8_t p[18]={0};Proto::put32(p,sid_);Proto::put32(p+4,epoch_);p[9]=state_.lights;p[11]=state_.horn;p[13]=(adasActive()?4:0)|(sync_?8:0)|(lcdFault_?16:0)|(bypass()?32:0);Proto::put16(p+14,last_);Proto::put16(p+16,sources_);Proto::make(out,op,id,0,p,18);}
 bool handle(const Proto::Frame& req,Proto::Frame& out,uint32_t now){
  clock_=now;
  if(!Proto::valid(req)){inhibit(Proto::COMM_FAULT);return error(req,out,Proto::BAD_FRAME);} const uint8_t* p=Proto::payload(req);uint8_t op=Proto::op(req);
  if(Proto::seq(req))return error(req,out,Proto::BAD_LENGTH,op!=Proto::STATE_SYNC&&op!=Proto::SENSOR_TELEMETRY);
  if(op==Proto::HELLO){
   if(Proto::len(req)!=9)return error(req,out,Proto::BAD_LENGTH);
   if(Proto::get32(p+4)!=Proto::MODEL_ID||p[8]!=Proto::LABELS_VERSION)return error(req,out,Proto::BAD_MODEL);
   uint32_t sid=Proto::get32(p);if(!sid||sid==0xffffffffUL)return error(req,out,Proto::BAD_SESSION);
   if(sid!=sid_&&sid<=highest_){off();uint8_t hint[6]={Proto::HELLO,Proto::BAD_SESSION};Proto::put32(hint+2,highest_);Proto::make(out,Proto::ERROR_REPLY,Proto::id(req),0,hint,6);return true;}
   if(sid!=sid_){off();sid_=highest_=sid;epoch_=0;last_=0;accepted_=false;sync_=true;sources_=Proto::BOOT_STOP;persist_=true;snapshotValid_=sensorValid_=policySet_=false;mode_=Proto::REQUIRE_ALERT;policyEpoch_=0;}
   contact_=now;Proto::peripheralMeta(out,sid_,Proto::id(req));return true;
  }
  if(op!=Proto::ACTUATE_BATCH&&op!=Proto::STOP_ALL&&op!=Proto::STATUS&&op!=Proto::HEARTBEAT&&op!=Proto::STATE_SYNC&&op!=Proto::SAFETY_POLICY&&op!=Proto::SENSOR_TELEMETRY)return error(req,out,Proto::BAD_OP);
  if(op==Proto::ACTUATE_BATCH){if(Proto::len(req)<12||p[8]<1||p[8]>4||Proto::len(req)!=11+p[8])return error(req,out,Proto::BAD_LENGTH);}
  else if(Proto::len(req)!=(op==Proto::STOP_ALL?10:op==Proto::STATE_SYNC?22:op==Proto::SAFETY_POLICY?9:op==Proto::SENSOR_TELEMETRY?16:4))return error(req,out,Proto::BAD_LENGTH,op!=Proto::STATE_SYNC&&op!=Proto::SENSOR_TELEMETRY);
  if(!sid_||Proto::get32(p)!=sid_)return error(req,out,Proto::BAD_SESSION,op!=Proto::STATE_SYNC&&op!=Proto::SENSOR_TELEMETRY);
  if(op==Proto::SAFETY_POLICY){
   uint32_t ep=Proto::get32(p+4);uint8_t mode=p[8];
   if(mode>Proto::BYPASS_ADAS)return error(req,out,Proto::BAD_OP,false);
   if(ep<epoch_||(policySet_&&ep<policyEpoch_))return error(req,out,Proto::STALE_SAFETY,false);
   if(policySet_&&ep==policyEpoch_){if(mode!=mode_)return error(req,out,Proto::STALE_SAFETY,false);contact_=now;Proto::ack(out,req);return true;}
   if(policySet_&&mode!=mode_&&ep<=epoch_)return error(req,out,Proto::STALE_SAFETY,false);
   off();sync_=true;policyEpoch_=ep;mode_=mode;policySet_=true;contact_=now;Proto::ack(out,req);return true;
  }
  if(op==Proto::SENSOR_TELEMETRY){
   Proto::SensorTelemetry s;if(!Proto::decodeTelemetry(req,s))return error(req,out,Proto::BAD_LENGTH,false);
   if(s.epoch!=epoch_)return error(req,out,Proto::STALE_SAFETY,false);
   if(sensorValid_&&sensor_.epoch==s.epoch&&s.sampleId!=sensor_.sampleId&&!Proto::newer16(s.sampleId,sensor_.sampleId))return error(req,out,Proto::OLD_COMMAND,false);
   // A duplicate cannot make the same physical measurement young again.
   if(sensorValid_&&sensor_.epoch==s.epoch&&s.sampleId==sensor_.sampleId){uint32_t elapsed=uint32_t(now-sensorAt_);uint32_t age=elapsed>65535?65535:uint32_t(sensor_.ageMs)+elapsed;if(age>65535)age=65535;if(s.ageMs<age)s.ageMs=uint16_t(age);}
   sensor_=s;sensorAt_=now;sensorValid_=true;contact_=now;Proto::ack(out,req);return true;
  }
  contact_=now;
  if(op==Proto::STATUS||op==Proto::HEARTBEAT){status(out,Proto::STATUS_REPLY,Proto::id(req));return true;}
  if(op==Proto::STATE_SYNC){Proto::SharedState s;if(!Proto::decodeState(req,s))return error(req,out,Proto::BAD_LENGTH,false);if(s.safetyEpoch<epoch_||(snapshotValid_&&s.safetyEpoch<snapshot_.safetyEpoch))return error(req,out,Proto::STALE_SAFETY,false);if(snapshotValid_){if(s.revision<snapshot_.revision)return error(req,out,Proto::OLD_COMMAND,false);if(s.revision==snapshot_.revision){uint8_t canonical[22];Proto::encodeState(canonical,snapshot_);if(memcmp(canonical,p,22))return error(req,out,Proto::BAD_OP,false);Proto::ack(out,req);return true;}}snapshot_=s;snapshotValid_=true;Proto::ack(out,req);return true;}
  uint32_t ep=Proto::get32(p+4);uint16_t id=Proto::id(req);
  if(op==Proto::STOP_ALL){
   if(ep<epoch_||(policySet_&&ep<policyEpoch_))return error(req,out,Proto::STALE_SAFETY);
   bool fresh=ep>epoch_;epoch_=ep;off();if(fresh)sources_=Proto::get16(p+8);else sources_|=Proto::get16(p+8);if(adasActive())sources_|=Proto::ADAS_OBSTACLE;if(fresh&&policySet_&&!adasActive())sync_=false;
   // A repeated STOP can switch everything off, but can never release inhibition.
   if(!accepted_||fresh||Proto::newer16(id,last_)){last_=id;previous_=req;accepted_=true;}
   status(out,Proto::ACK,id);return true;
  }
  if(ep!=epoch_)return error(req,out,Proto::STALE_SAFETY);
  if(inhibited())return error(req,out,Proto::SAFETY_INHIBIT);
  if(accepted_&&id==last_){if(memcmp(req.bytes,previous_.bytes,32))return error(req,out,Proto::BAD_OP);status(out,Proto::ACK,id);return true;}
  if(accepted_&&!Proto::newer16(id,last_))return error(req,out,Proto::OLD_COMMAND);
  uint16_t seen=0;for(uint8_t i=0;i<p[8];i++){uint8_t intent=p[9+i];if(intent>9)return error(req,out,Proto::BAD_OP);seen|=uint16_t(1)<<intent;}
  if(seen&(uint16_t(1)<<Proto::ALL_STOP)){off();last_=id;previous_=req;accepted_=true;status(out,Proto::ACK,id);return true;}
  if(seen&((1<<Proto::WIPER_ON)|(1<<Proto::WIPER_OFF)|(1<<Proto::UNKNOWN_INTENT)))return error(req,out,Proto::BAD_OP);
  const uint8_t pairs[6]={0,1,3,4,8,9};for(uint8_t i=0;i<6;i+=2)if((seen&(uint16_t(1)<<pairs[i]))&&(seen&(uint16_t(1)<<pairs[i+1])))return error(req,out,Proto::BAD_OP);
  if(seen&(uint16_t(1)<<Proto::ALL_STOP))off();else {if(seen&(1<<Proto::LIGHT_ON))state_.lights=true;if(seen&(1<<Proto::LIGHT_OFF))state_.lights=false;if(seen&(1<<Proto::HORN_ON))state_.horn=true;if(seen&(1<<Proto::HORN_OFF))state_.horn=false;}
  last_=id;previous_=req;accepted_=true;status(out,Proto::ACK,id);return true;
 }
};
}
