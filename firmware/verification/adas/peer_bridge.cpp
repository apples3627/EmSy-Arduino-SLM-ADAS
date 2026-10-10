#include <cstring>
#if PEER==0
#include ACTOR_ENGINE_HEADER
static Actuator::Engine actor;
extern "C" void main_actor_reset(){actor.init(0);}
extern "C" bool main_actor_exchange(const uint8_t* q,uint8_t* r,uint32_t now){Proto::Frame in,out;memcpy(in.bytes,q,32);bool ok=actor.onFrame(in,out,now);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" bool main_actor_tick(uint8_t* r,uint32_t now){Proto::Frame out;actor.tick(now);bool ok=actor.takeReply(out);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" int main_actor_drive(){return actor.outputs().drive;}
extern "C" void main_actor_adas(bool active){actor.adas(active);}
#elif PEER==1
#include ADAS_WORKER_HEADER
static Adas::Worker adas;static uint16_t age;
extern "C" void main_i2c_reset(){adas=Adas::Worker();age=0;for(int i=0;i<3;i++)adas.policy.sample(3480,0);}
extern "C" void adas_telemetry(uint8_t f,uint8_t count,uint16_t mm,uint16_t ms){age=ms;adas.policy=Adas::Policy();if(f&Adas::SENSOR_FAULT)adas.policy.fault();else{for(uint8_t i=0;i<count;i++)adas.policy.sample(uint32_t(mm)*58/5,0);if(f&Adas::OBSTACLE)adas.policy.sample(uint32_t(mm)*58/5,0);}}
extern "C" void main_i2c_exchange(const uint8_t* q,uint8_t* r){Proto::Frame in,out;memcpy(in.bytes,q,32);if(Proto::op(in)==Proto::STATUS&&adas.policy.valid)adas.policy.sample(uint32_t(adas.policy.rangeMm)*58/5,0);adas.handle(in,out,age);memcpy(r,out.bytes,32);}
extern "C" void main_i2c_scores(float*){}
extern "C" void adas_shared(void* p){memcpy(p,&adas.snapshot.shared,sizeof(Proto::SharedState));}
#else
#include SLM_WORKER_HEADER
static Slm::Worker slm;
extern "C" void main_spi_reset(){slm=Slm::Worker();}
extern "C" void main_spi_exchange(const uint8_t* q,uint8_t* r){Proto::Frame in,out;memcpy(in.bytes,q,32);slm.handle(in,out);slm.computeIfPending();memcpy(r,out.bytes,32);}
extern "C" void main_spi_scores(float* p){memcpy(p,slm.scores,40);}
extern "C" void slm_shared(void* p){memcpy(p,&slm.snapshot.shared,sizeof(Proto::SharedState));}
#endif
