#pragma once
#include "protocol.h"
#include "board_config.h"
namespace Actuator {
struct Outputs { int8_t drive; uint8_t duty; bool lights,wiper,horn; uint8_t servoAngle; };
class Engine {
 Config cfg; Outputs state; uint32_t highest,session,lastContact,driveStart,sweepAt,pendingAt; uint16_t lastAccepted,lastApplied,pendingId,pendingDuration; bool accepted,pending,asyncReady,localLatched,buttonDown,sweepUp,persistNeeded; int8_t pendingDrive; Proto::Frame asyncFrame;
 static bool elapsed(uint32_t now,uint32_t start,uint16_t interval){return uint32_t(now-start)>=interval;}
 void off(){state.drive=0;state.duty=0;state.lights=state.wiper=state.horn=false;pending=false;asyncReady=false;}
 void status(Proto::Frame& reply,uint8_t op,uint16_t id){uint8_t p[12]={0};Proto::put32(p,session);p[4]=uint8_t(state.drive);p[5]=state.lights;p[6]=state.wiper;p[7]=state.horn;p[8]=state.servoAngle;p[9]=(pending?1:0)|(localLatched?2:0);Proto::put16(p+10,lastApplied);Proto::make(reply,op,id,0,p,12);}
 bool error(const Proto::Frame& req,Proto::Frame& out,uint8_t err){Proto::ack(out,req,err);return true;}
 void drive(int8_t direction,uint16_t duration,uint16_t id,uint32_t now){state.drive=direction;state.duty=cfg.motorDuty>249?249:cfg.motorDuty;driveStart=now;pendingDuration=duration;lastApplied=id;}
 public:
 Engine(const Config& config=Config()):cfg(config){init(0);}
 void init(uint32_t saved){highest=saved;session=0;lastContact=driveStart=sweepAt=pendingAt=0;lastAccepted=lastApplied=pendingId=pendingDuration=0;accepted=pending=asyncReady=localLatched=buttonDown=persistNeeded=false;sweepUp=true;pendingDrive=0;state.servoAngle=cfg.servoMin;off();}
 const Outputs& outputs()const{return state;} uint32_t currentSession()const{return session;} uint32_t persistedSession()const{return highest;}
 bool takePersist(){bool p=persistNeeded;persistNeeded=false;return p;}
 void fault(){off();session=0;}
 void localButton(bool down){buttonDown=down;if(down){localLatched=true;off();session=0;}}
 void tick(uint32_t now){
  if(session&&elapsed(now,lastContact,cfg.watchdogMs)){fault();return;}
  if(pending&&elapsed(now,pendingAt,cfg.reversalMs)){pending=false;drive(pendingDrive,pendingDuration,pendingId,now);status(asyncFrame,Proto::ACK,pendingId);asyncReady=true;}
  if(state.drive&&elapsed(now,driveStart,pendingDuration)){state.drive=0;state.duty=0;}
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
   if(incoming!=session){off();highest=session=incoming;persistNeeded=true;accepted=false;lastAccepted=lastApplied=0;localLatched=false;}
   lastContact=now;uint8_t meta[10];Proto::put32(meta,session);Proto::put32(meta+4,Proto::MODEL_ID);meta[8]=Proto::LABELS_VERSION;meta[9]=15;Proto::make(out,Proto::HELLO_ACK,Proto::id(req),0,meta,10);return true;
  }
  if(op!=Proto::ACTUATE&&op!=Proto::STOP_ALL&&op!=Proto::STATUS&&op!=Proto::HEARTBEAT){off();return error(req,out,Proto::BAD_OP);}
  if(Proto::len(req)!=(op==Proto::ACTUATE?7:4)){off();return error(req,out,Proto::BAD_LENGTH);}
  if(localLatched)return error(req,out,Proto::LOCAL_STOP);
  if(!session||Proto::get32(p)!=session)return error(req,out,Proto::BAD_SESSION);
  lastContact=now;
  if(op==Proto::STATUS||op==Proto::HEARTBEAT){status(out,Proto::STATUS_REPLY,Proto::id(req));return true;}
  uint16_t id=Proto::id(req);
  if(accepted&&id==lastAccepted){if(pending)return error(req,out,Proto::BUSY);status(out,Proto::ACK,id);return true;}
  if(accepted&&!Proto::newer16(id,lastAccepted))return error(req,out,Proto::OLD_COMMAND);
  uint8_t intent=op==Proto::STOP_ALL?uint8_t(Proto::ALL_STOP):p[4];
  if(intent>9){off();return error(req,out,Proto::BAD_OP);}
  // While reversal waits, only a global stop may supersede it. Independent actions retry after ACK.
  if(pending&&intent!=Proto::ALL_STOP&&intent!=Proto::UNKNOWN_INTENT)return error(req,out,Proto::BUSY);
  accepted=true;lastAccepted=id;
  if(intent==Proto::FORWARD||intent==Proto::REVERSE){uint16_t duration=Proto::get16(p+5);if(!duration)duration=cfg.motionMs;if(duration>cfg.maxMotionMs)duration=cfg.maxMotionMs;int8_t direction=intent==Proto::FORWARD?1:-1;
   if(state.drive&&state.drive!=direction){state.drive=0;state.duty=0;pending=true;pendingAt=now;pendingId=id;pendingDrive=direction;pendingDuration=duration;return false;}drive(direction,duration,id,now);
  }else {lastApplied=id;switch(intent){case Proto::ALL_STOP:case Proto::UNKNOWN_INTENT:off();break;case Proto::LIGHT_ON:state.lights=true;break;case Proto::LIGHT_OFF:state.lights=false;break;case Proto::WIPER_ON:state.wiper=true;sweepAt=now;break;case Proto::WIPER_OFF:state.wiper=false;break;case Proto::HORN_ON:state.horn=true;break;case Proto::HORN_OFF:state.horn=false;break;}}
  status(out,Proto::ACK,id);return true;
 }
};
}
