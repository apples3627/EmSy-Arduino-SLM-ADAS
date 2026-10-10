#include "../actuator_uart/src/control_engine.h"
#include "../main/src/model/intent_model.h"
#include <cassert>
static Actuator::Engine actor;
static Proto::SharedState states[2];
static uint32_t sessions[2];
static uint8_t flags=8,clearCount=3;static bool freezeSample;
static uint16_t distance=300,age=0,job,sample;
static Intent::Input input;
static float z[10];
extern "C" void main_actor_reset(){actor.init(0);}
extern "C" bool main_actor_exchange(const uint8_t* q,uint8_t* r,uint32_t now){Proto::Frame in,out;memcpy(in.bytes,q,32);bool ok=actor.onFrame(in,out,now);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" bool main_actor_tick(uint8_t* r,uint32_t now){Proto::Frame out;actor.tick(now);bool ok=actor.takeReply(out);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" int main_actor_drive(){return actor.outputs().drive;}
extern "C" void main_actor_adas(bool active){actor.adas(active);}
extern "C" void adas_freeze(bool freeze){freezeSample=freeze;}
extern "C" void adas_telemetry(uint8_t f,uint8_t count,uint16_t mm,uint16_t ms){flags=f;clearCount=count;distance=mm;age=ms;}
extern "C" void main_i2c_reset(){states[0]={};sessions[0]=0;flags=8;clearCount=3;distance=300;age=0;}
extern "C" void main_spi_reset(){states[1]={};sessions[1]=0;job=0;}
extern "C" void shared_state(uint8_t node,Proto::SharedState* out){*out=states[node];}
static bool sync(uint8_t node,const Proto::Frame& in,Proto::Frame& out){Proto::SharedState s;if(!Proto::decodeState(in,s)||s.session!=sessions[node]){Proto::ack(out,in,Proto::BAD_SESSION);return true;}if(s.revision>=states[node].revision)states[node]=s;Proto::ack(out,in);return true;}
extern "C" void main_i2c_exchange(const uint8_t* q,uint8_t* r){Proto::Frame in,out;memcpy(in.bytes,q,32);assert(Proto::valid(in));if(Proto::op(in)==Proto::HELLO){sessions[0]=Proto::get32(Proto::payload(in));uint8_t p[8]={Proto::ADAS_ROLE,Proto::HC_SR04,20,25,3,3,70,0};Proto::make(out,Proto::ADAS_META,Proto::id(in),0,p,8);}else if(Proto::op(in)==Proto::STATE_SYNC)sync(0,in,out);else{assert(Proto::op(in)==Proto::STATUS&&Proto::len(in)==0);uint8_t p[18]={Proto::ADAS_ROLE,flags};Proto::put16(p+2,distance);if(!freezeSample)++sample;Proto::put16(p+4,sample);Proto::put16(p+6,age);p[8]=20;p[9]=25;p[10]=clearCount;p[11]=3;Proto::put16(p+12,70);Proto::put32(p+14,states[0].revision);Proto::make(out,Proto::ADAS_STATUS_REPLY,Proto::id(in),0,p,18);}memcpy(r,out.bytes,32);}
extern "C" void main_spi_exchange(const uint8_t* q,uint8_t* r){Proto::Frame in,out;memcpy(in.bytes,q,32);assert(Proto::valid(in));uint8_t op=Proto::op(in);if(op==Proto::HELLO){sessions[1]=Proto::get32(Proto::payload(in));Proto::fullSlmMeta(out,Proto::id(in));}else if(op==Proto::STATE_SYNC)sync(1,in,out);else if(op==Proto::LOAD_INPUT){job=Proto::id(in);uint8_t start=Proto::seq(in)?12:0;for(uint8_t i=0;i<Proto::len(in)/2;i++)input.tokens[start+i]=Proto::get16(Proto::payload(in)+2*i);Proto::ack(out,in);}else if(op==Proto::START){input.count=0;while(input.count<16&&input.tokens[input.count]!=Proto::PAD)++input.count;Intent::logits(input,z);Proto::ack(out,in);}else if(op==Proto::STATUS){uint8_t p[6]={Proto::RESULT_READY,0,3,1};Proto::put16(p+4,job);Proto::make(out,Proto::STATUS_REPLY,Proto::id(in),0,p,6);}else{assert(op==Proto::GET_RESULT);uint8_t p[24],start=Proto::seq(in)?6:0,n=Proto::seq(in)?16:24;for(uint8_t i=0;i<n/4;i++)Proto::putFloat(p+4*i,z[start+i]);Proto::make(out,Proto::RESULT,Proto::id(in),Proto::seq(in),p,n);}memcpy(r,out.bytes,32);}
extern "C" void main_i2c_scores(float*){}
extern "C" void main_spi_scores(float* out){memcpy(out,z,40);}
