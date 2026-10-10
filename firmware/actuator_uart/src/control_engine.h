#pragma once
#include "protocol.h"
#include "board_config.h"
namespace Actuator {
struct Outputs { int8_t drive; uint8_t duty; bool lights,wiper,horn; uint8_t servoAngle; };
class Engine {
 Config cfg; Outputs state; uint32_t epoch; uint16_t stopSources; bool adasActive,requireSafetySync; uint32_t highest,session,lastContact,driveStart,sweepAt,pendingAt; uint16_t lastAccepted,lastApplied,pendingId,pendingDuration; bool accepted,pending,asyncReady,localLatched,buttonDown,sweepUp,persistNeeded; int8_t pendingDrive; Proto::Frame asyncFrame,lastRequest; Outputs staged;
 static bool elapsed(uint32_t now,uint32_t start,uint16_t interval){return uint32_t(now-start)>=interval;}
 void off(){state.drive=0;state.duty=0;state.lights=state.wiper=state.horn=false;pending=false;asyncReady=false;}
 void status(Proto::Frame& reply,uint8_t op,uint16_t id){uint8_t p[18]={0};Proto::put32(p,session);Proto::put32(p+4,epoch);p[8]=uint8_t(state.drive);p[9]=state.lights;p[10]=state.wiper;p[11]=state.horn;p[12]=state.servoAngle;p[13]=(pending?1:0)|(localLatched?2:0)|(adasActive?4:0)|(requireSafetySync?8:0);Proto::put16(p+14,lastApplied);Proto::put16(p+16,stopSources);Proto::make(reply,op,id,0,p,18);}
 bool error(const Proto::Frame& req,Proto::Frame& out,uint8_t err){Proto::ack(out,req,err);return true;}
 void drive(int8_t direction,uint16_t duration,uint16_t id,uint32_t now){state.drive=direction;state.duty=cfg.motorDuty>249?249:cfg.motorDuty;driveStart=now;pendingDuration=duration;lastApplied=id;}
 public:
 Engine(const Config& config=Config()):cfg(config){init(0);}
 void init(uint32_t saved){highest=saved;session=0;epoch=0;stopSources=Proto::BOOT_STOP;adasActive=false;requireSafetySync=true;lastContact=driveStart=sweepAt=pendingAt=0;lastAccepted=lastApplied=pendingId=pendingDuration=0;accepted=pending=asyncReady=localLatched=buttonDown=persistNeeded=false;sweepUp=true;pendingDrive=0;state.servoAngle=cfg.servoMin;off();}
 const Outputs& outputs()const{return state;} uint32_t currentSession()const{return session;} uint32_t persistedSession()const{return highest;}
 bool takePersist(){bool p=persistNeeded;persistNeeded=false;return p;}
 void fault(){off();session=0;requireSafetySync=true;stopSources=Proto::COMM_FAULT;}
 void adas(bool active){adasActive=active;if(active){off();requireSafetySync=true;stopSources=Proto::ADAS_OBSTACLE;}}
 uint32_t safetyEpoch()const{return epoch;} bool safetyInhibited()const{return adasActive||requireSafetySync;}
 void localButton(bool down){buttonDown=down;if(down){localLatched=true;off();session=0;requireSafetySync=true;stopSources=Proto::USER_ESTOP;}}
 void tick(uint32_t now){
  if(session&&elapsed(now,lastContact,cfg.watchdogMs)){fault();return;}
  if(pending&&!adasActive&&!requireSafetySync&&elapsed(now,pendingAt,cfg.reversalMs)){pending=false;staged.servoAngle=state.servoAngle;state=staged;if(state.wiper)sweepAt=now;drive(pendingDrive,pendingDuration,pendingId,now);status(asyncFrame,Proto::ACK,pendingId);asyncReady=true;}
  if(state.drive&&elapsed(now,driveStart,pendingDuration)){state.drive=0;state.duty=0;stopSources=Proto::DRIVE_LEASE;}
  if(state.wiper&&elapsed(now,sweepAt,cfg.servoStepMs)){sweepAt=now;int16_t angle=int16_t(state.servoAngle)+(sweepUp?cfg.servoStep:-int16_t(cfg.servoStep));if(angle>=cfg.servoMax){angle=cfg.servoMax;sweepUp=false;}if(angle<=cfg.servoMin){angle=cfg.servoMin;sweepUp=true;}state.servoAngle=uint8_t(angle);}
 }
 bool takeReply(Proto::Frame& out){if(!asyncReady)return false;out=asyncFrame;asyncReady=false;return true;}
 bool onFrame(const Proto::Frame& req,Proto::Frame& out,uint32_t now){
  if(!Proto::valid(req)){fault();return error(req,out,Proto::BAD_FRAME);}uint8_t op=Proto::op(req);const uint8_t* p=Proto::payload(req);
  if(Proto::seq(req)!=0){off();return error(req,out,Proto::BAD_LENGTH);}
  if(op==Proto::HELLO){
   if(Proto::len(req)!=9){off();return error(req,out,Proto::BAD_LENGTH);}
   uint32_t incoming=Proto::get32(p);
   if(Proto::get32(p+4)!=Proto::MODEL_ID||p[8]!=Proto::LABELS_VERSION){off();return error(req,out,Proto::BAD_MODEL);}
   if(buttonDown)return error(req,out,Proto::LOCAL_STOP);
   if(!incoming||incoming==0xffffffffUL||(incoming!=session&&incoming<=highest))return error(req,out,Proto::BAD_SESSION);
   if(incoming!=session){off();epoch=0;requireSafetySync=true;stopSources=Proto::BOOT_STOP;highest=session=incoming;persistNeeded=true;accepted=false;lastAccepted=lastApplied=0;localLatched=false;}
   lastContact=now;uint8_t meta[10];Proto::put32(meta,session);Proto::put32(meta+4,Proto::MODEL_ID);meta[8]=Proto::LABELS_VERSION;meta[9]=15;Proto::make(out,Proto::HELLO_ACK,Proto::id(req),0,meta,10);return true;
  }
  if(op!=Proto::ACTUATE&&op!=Proto::ACTUATE_BATCH&&op!=Proto::STOP_ALL&&op!=Proto::STATUS&&op!=Proto::HEARTBEAT){off();return error(req,out,Proto::BAD_OP);}
  uint8_t count=1;
  if(op==Proto::ACTUATE_BATCH){if(Proto::len(req)<12||p[8]<1||p[8]>4||Proto::len(req)!=11+p[8]){off();return error(req,out,Proto::BAD_LENGTH);}count=p[8];}
  else if(Proto::len(req)!=(op==Proto::ACTUATE?11:(op==Proto::STOP_ALL?10:4))){off();return error(req,out,Proto::BAD_LENGTH);}
  if(localLatched)return error(req,out,Proto::LOCAL_STOP);
  if(!session||Proto::get32(p)!=session)return error(req,out,Proto::BAD_SESSION);
  lastContact=now;
  if(op==Proto::STATUS||op==Proto::HEARTBEAT){status(out,Proto::STATUS_REPLY,Proto::id(req));return true;}
  uint16_t id=Proto::id(req);uint32_t requestedEpoch=Proto::get32(p+4);
  if(accepted&&id==lastAccepted){if(memcmp(req.bytes,lastRequest.bytes,Proto::FRAME_SIZE)){off();return error(req,out,Proto::BAD_OP);}if(requestedEpoch!=epoch)return error(req,out,Proto::STALE_SAFETY);if(op!=Proto::STOP_ALL&&(adasActive||requireSafetySync))return error(req,out,Proto::SAFETY_INHIBIT);if(pending)return error(req,out,Proto::BUSY);status(out,Proto::ACK,id);return true;}
  if(op==Proto::STOP_ALL){if(requestedEpoch<epoch)return error(req,out,Proto::STALE_SAFETY);bool fresh=requestedEpoch>epoch;epoch=requestedEpoch;stopSources=Proto::get16(p+8);off();if(fresh&&!adasActive&&!buttonDown)requireSafetySync=false;accepted=true;lastAccepted=lastApplied=id;lastRequest=req;status(out,Proto::ACK,id);return true;}
  if(requestedEpoch!=epoch)return error(req,out,Proto::STALE_SAFETY);
  if(adasActive||requireSafetySync)return error(req,out,Proto::SAFETY_INHIBIT);
  if(accepted&&!Proto::newer16(id,lastAccepted))return error(req,out,Proto::OLD_COMMAND);
  uint16_t seen=0;bool globalStop=op==Proto::STOP_ALL,invalid=false,conflict=false;
  for(uint8_t i=0;i<count;i++){uint8_t intent=op==Proto::STOP_ALL?uint8_t(Proto::ALL_STOP):p[op==Proto::ACTUATE_BATCH?9+i:8];if(intent>9){invalid=true;continue;}seen|=uint16_t(1)<<intent;if(intent==Proto::ALL_STOP||intent==Proto::UNKNOWN_INTENT)globalStop=true;}
  const uint8_t pairs[8]={0,1,3,4,5,6,8,9};for(uint8_t i=0;i<8;i+=2)if((seen&(uint16_t(1)<<pairs[i]))&&(seen&(uint16_t(1)<<pairs[i+1])))conflict=true;
  if(invalid||(!globalStop&&conflict)){off();return error(req,out,Proto::BAD_OP);}
  if(pending&&!globalStop)return error(req,out,Proto::BUSY);
  accepted=true;lastAccepted=id;lastRequest=req;
  if(globalStop){off();lastApplied=id;status(out,Proto::ACK,id);return true;}
  Outputs next=state;int8_t direction=0;
  if(seen&(1<<Proto::FORWARD))direction=1;if(seen&(1<<Proto::REVERSE))direction=-1;
  if(seen&(1<<Proto::LIGHT_ON))next.lights=true;if(seen&(1<<Proto::LIGHT_OFF))next.lights=false;
  if(seen&(1<<Proto::WIPER_ON))next.wiper=true;if(seen&(1<<Proto::WIPER_OFF))next.wiper=false;
  if(seen&(1<<Proto::HORN_ON))next.horn=true;if(seen&(1<<Proto::HORN_OFF))next.horn=false;
  uint16_t duration=op==Proto::ACTUATE_BATCH?Proto::get16(p+9+count):Proto::get16(p+9);
  if(!duration)duration=cfg.motionMs;if(duration>cfg.maxMotionMs)duration=cfg.maxMotionMs;
  if(direction&&state.drive&&state.drive!=direction){state.drive=0;state.duty=0;staged=next;pending=true;pendingAt=now;pendingId=id;pendingDrive=direction;pendingDuration=duration;return false;}
  state=next;if(seen&(1<<Proto::WIPER_ON))sweepAt=now;
  if(direction)drive(direction,duration,id,now);else lastApplied=id;
  status(out,Proto::ACK,id);return true;
 }
};
}
