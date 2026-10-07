#pragma once
#include "protocol.h"
#include "model/intent_model.h"
static_assert(IntentConfig::MODEL_ID == Proto::MODEL_ID, "model ID mismatch");
static_assert(IntentConfig::VOCAB == Proto::VOCAB_SIZE && IntentConfig::LABELS == 10 && IntentConfig::EMBED == 16 && IntentConfig::HIDDEN == 64, "model shape mismatch");
static_assert(IntentWeights::OFFSET == WORKER_LO, "model shard offset mismatch");
static_assert(IntentWeights::HIDDEN == 32, "worker must contain 32 neurons");
namespace Compute {
class Worker {
 public:
 uint16_t job; uint8_t state,error,mask; bool pending; uint16_t tokens[16]; float scores[10];
 Worker():job(0),state(Proto::IDLE),error(0),mask(0),pending(false){clearTokens();}
 void clearTokens(){for(uint8_t i=0;i<16;i++)tokens[i]=Proto::PAD;}
 void meta(Proto::Frame& f,uint16_t id=0){Proto::computeMeta(f,id,WORKER_LO,WORKER_LO+32);}
 void status(Proto::Frame& f,uint16_t id=0,uint8_t seq=0){uint8_t p[6]={state,error,mask,WORKER_ROLE,0,0};Proto::put16(p+4,job);Proto::make(f,Proto::STATUS_REPLY,id,seq,p,6);}
 void fail(const Proto::Frame& q,Proto::Frame& r,uint8_t e){Proto::ack(r,q,e);}
 void handle(const Proto::Frame& q,Proto::Frame& r){
  if(!Proto::valid(q)){fail(q,r,Proto::BAD_FRAME);return;}
  uint8_t op=Proto::op(q),n=Proto::len(q),seq=Proto::seq(q);uint16_t id=Proto::id(q);
  if(op==Proto::HELLO){if(n!=0&&n!=4){fail(q,r,Proto::BAD_LENGTH);return;}job=0;state=Proto::IDLE;error=mask=0;pending=false;clearTokens();meta(r,id);r.bytes[5]=seq;r.bytes[31]=Proto::crc8(r.bytes,31);return;}
  if(op==Proto::STATUS){if(n){fail(q,r,Proto::BAD_LENGTH);return;}status(r,id,seq);return;}
  if(op==Proto::LOAD_INPUT){
   if(seq>1||n!=(seq?8:24)){fail(q,r,Proto::BAD_LENGTH);return;}
   if(state==Proto::COMPUTING && id!=job){fail(q,r,Proto::BUSY);return;}
   if(seq==1&&(id!=job||!mask)){fail(q,r,Proto::BAD_JOB);return;}
   const uint8_t first=seq?12:0,count=seq?4:12;
   for(uint8_t i=0;i<count;i++){uint16_t t=Proto::get16(Proto::payload(q)+i*2);if(t!=Proto::PAD&&t>=Proto::VOCAB_SIZE){fail(q,r,Proto::BAD_TOKEN);return;}}
   if(id==job&&(mask&(1<<seq))){for(uint8_t i=0;i<count;i++)if(tokens[first+i]!=Proto::get16(Proto::payload(q)+i*2)){fail(q,r,Proto::BAD_JOB);return;}Proto::ack(r,q);return;}
   if(seq==0&&(id!=job||!mask)){job=id;mask=0;error=0;state=Proto::RECEIVING;pending=false;clearTokens();}
   for(uint8_t i=0;i<count;i++)tokens[first+i]=Proto::get16(Proto::payload(q)+i*2);
   mask|=1<<seq;Proto::ack(r,q);return;
  }
  if(op==Proto::START){
   if(n){fail(q,r,Proto::BAD_LENGTH);return;}
   if(id!=job||mask!=3){fail(q,r,Proto::BAD_JOB);return;}
   if(state==Proto::COMPUTING||state==Proto::RESULT_READY){Proto::ack(r,q);return;}
   bool any=false;for(uint8_t i=0;i<16;i++)if(tokens[i]!=Proto::PAD)any=true;
   if(!any){fail(q,r,Proto::BAD_TOKEN);return;}
   pending=true;state=Proto::COMPUTING;Proto::ack(r,q);return;
  }
  if(op==Proto::GET_RESULT){
   if(n||seq>1){fail(q,r,Proto::BAD_LENGTH);return;}
   if(id!=job||state!=Proto::RESULT_READY){fail(q,r,Proto::NOT_READY);return;}
   uint8_t p[24];uint8_t count=seq?4:6,first=seq?6:0;for(uint8_t i=0;i<count;i++)Proto::putFloat(p+i*4,scores[first+i]);Proto::make(r,Proto::RESULT,id,seq,p,count*4);return;
  }
  fail(q,r,Proto::BAD_OP);
 }
 bool computeIfPending(){if(!pending)return false;pending=false;Intent::Input input;Intent::decodeTokens(tokens,input);Intent::logits(input,scores);for(uint8_t i=0;i<10;i++)if(!isfinite(scores[i])){state=Proto::FAULT;error=Proto::BAD_TOKEN;return true;}state=Proto::RESULT_READY;error=0;return true;}
};
}
