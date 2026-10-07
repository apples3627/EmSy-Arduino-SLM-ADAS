// Separate TUs keep each actual peer's model shard and class symbols distinct.
#include <stdint.h>
#ifndef MAIN_PEER
#error MAIN_PEER must be 0 (actuator), 1 (I2C) or 2 (SPI)
#endif
#if MAIN_PEER == 0
#ifndef ACTOR_HEADER
#define ACTOR_HEADER "../actuator_uart/src/control_engine.h"
#endif
#include ACTOR_HEADER
static Actuator::Engine actor;
extern "C" void main_actor_reset(){actor.init(0);}
extern "C" bool main_actor_exchange(const uint8_t* q,uint8_t* r,uint32_t now){Proto::Frame in,out;memcpy(in.bytes,q,32);bool ok=actor.onFrame(in,out,now);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" bool main_actor_tick(uint8_t* r,uint32_t now){Proto::Frame out;actor.tick(now);bool ok=actor.takeReply(out);if(ok)memcpy(r,out.bytes,32);return ok;}
extern "C" int main_actor_drive(){return actor.outputs().drive;}
#else
#define INTENT_PART MAIN_PEER
#define WORKER_LO ((MAIN_PEER-1)*32)
#define WORKER_ROLE MAIN_PEER
#if MAIN_PEER == 1
#define Compute ComputeI2C
#define Intent IntentI2C
#define IntentWeights WeightsI2C
#define IntentConfig ConfigI2C
#ifndef COMPUTE_HEADER
#define COMPUTE_HEADER "../compute_i2c/src/neural_worker.h"
#endif
#define EXCHANGE main_i2c_exchange
#define RESET main_i2c_reset
#define SCORES main_i2c_scores
#else
#define Compute ComputeSPI
#define Intent IntentSPI
#define IntentWeights WeightsSPI
#define IntentConfig ConfigSPI
#ifndef COMPUTE_HEADER
#define COMPUTE_HEADER "../compute_spi/src/neural_worker.h"
#endif
#define EXCHANGE main_spi_exchange
#define RESET main_spi_reset
#define SCORES main_spi_scores
#endif
#include COMPUTE_HEADER
static Compute::Worker worker;
extern "C" void RESET(){worker=Compute::Worker();}
extern "C" void EXCHANGE(const uint8_t* q,uint8_t* r){Proto::Frame in,out;memcpy(in.bytes,q,32);worker.handle(in,out);memcpy(r,out.bytes,32);worker.computeIfPending();}
extern "C" void SCORES(float* out){memcpy(out,worker.scores,40);}
#endif
