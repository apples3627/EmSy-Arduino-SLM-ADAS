#include <assert.h>
#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>
#ifndef MAIN_BOARD_HEADER
#define MAIN_BOARD_HEADER "../../main/src/board_app.h"
#endif
#include MAIN_BOARD_HEADER
extern "C" void main_actor_reset();
extern "C" bool main_actor_exchange(const uint8_t*,uint8_t*,uint32_t);
extern "C" bool main_actor_tick(uint8_t*,uint32_t);
extern "C" int main_actor_drive();
extern "C" void main_i2c_reset();
extern "C" void main_spi_reset();
extern "C" void main_i2c_exchange(const uint8_t*,uint8_t*);
extern "C" void main_spi_exchange(const uint8_t*,uint8_t*);
extern "C" void main_i2c_scores(float*);
extern "C" void main_spi_scores(float*);
struct Pending {Proto::Frame frame;uint32_t due;};
struct Simulation {
 Proto::Frame writes[2],cached[2];Proto::StreamParser uart;
 std::deque<Pending> replies;std::string logs;std::vector<Proto::Frame> actorCommands,workerCommands[2];
 uint32_t passes=0,computeUntil=0;uint8_t wireByte=0;int fault=0;bool holdSecond=false;bool holdStop=false,holdWorkers=false,holdResult=false,holdActAck=false,overlap=false;Proto::Frame lateActAck;
 void inject(const Proto::Frame& f){for(uint8_t b:f.bytes){uint8_t n=(MainIO::rw+1)&63;assert(n!=MainIO::rr);MainIO::rx[MainIO::rw]=b;MainIO::rw=n;}}
 void exchange(uint8_t w){if(!Proto::valid(writes[w])){fprintf(stderr,"BAD SIM WRITE w=%u requestOp=%u writtenOp=%u index=%u phase=%u t=%u\n",w,Proto::op(MainLinks::bus[w].request),Proto::op(writes[w]),MainLinks::bus[w].index,MainLinks::bus[w].phase,unsigned(MainIO::ticks));}assert(Proto::valid(writes[w]));workerCommands[w].push_back(writes[w]);if(w)main_spi_exchange(writes[w].bytes,cached[w].bytes);else main_i2c_exchange(writes[w].bytes,cached[w].bytes);if(Proto::op(cached[w])==Proto::STATUS_REPLY&&computeUntil>MainIO::ticks){Proto::payload(cached[w])[0]=Proto::COMPUTING;cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(Proto::op(cached[w])==Proto::RESULT&&w==1){if(fault==1){Proto::put16(cached[w].bytes+3,Proto::id(cached[w])-1);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==2){Proto::putFloat(Proto::payload(cached[w]),NAN);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==3)cached[w].bytes[31]^=1;}}
 void twi(){auto& b=MainLinks::bus[0];if((holdWorkers||(holdSecond&&MainBoard::clauseIndex==1))&&MainBoard::active)return;if(b.phase==4){exchange(0);TWCR&=~_BV(TWSTO);return;}if(b.phase==9||b.phase==10||b.phase==11){TWCR&=~_BV(TWSTO);return;}uint8_t state=0;if(b.phase==1||b.phase==6)state=8;else if(b.phase==2){if(!b.index)state=0x18;else{writes[0].bytes[b.index-1]=TWDR;state=0x28;}}else if(b.phase==7)state=0x40;else if(b.phase==8){TWDR=cached[0].bytes[b.index];state=b.index==31?0x58:0x50;}else return;TWSR=state;TWCR|=_BV(TWINT);TWI_vect();}
 void spi(){auto& b=MainLinks::bus[1];if((holdWorkers||(holdSecond&&MainBoard::clauseIndex==1))&&MainBoard::active){SPSR&=~_BV(SPIF);return;}if(holdResult&&Proto::op(b.request)==Proto::GET_RESULT&&Proto::seq(b.request)==1){SPSR&=~_BV(SPIF);return;}if(b.phase==1){writes[1].bytes[b.index]=SPDR;SPDR=0xff;SPSR|=_BV(SPIF);}else if(b.phase==3){SPDR=cached[1].bytes[b.index];SPSR|=_BV(SPIF);}else SPSR&=~_BV(SPIF);}
 void actor(){uint8_t old=MainIO::txBit;TIMER1_COMPA_vect();bool bit=PORTD&_BV(PORTD3);if(old==10&&MainIO::txBit==0){assert(!bit);wireByte=0;}else if(old<8){if(bit)wireByte|=1<<old;}else if(old==8){assert(bit);Proto::Frame f;if(uart.push(wireByte,f)){actorCommands.push_back(f);Proto::Frame r;if(main_actor_exchange(f.bytes,r.bytes,MainIO::ticks)){if(holdActAck&&(Proto::op(f)==Proto::ACTUATE||Proto::op(f)==Proto::ACTUATE_BATCH))lateActAck=r;else if(!(holdStop&&Proto::op(f)==Proto::STOP_ALL))replies.push_back({r,MainIO::ticks+5});}}}else if(old==9)assert(bit);
 Proto::Frame r;if(main_actor_tick(r.bytes,MainIO::ticks))replies.push_back({r,MainIO::ticks+5});if(!replies.empty()&&int32_t(MainIO::ticks-replies.front().due)>=0){inject(replies.front().frame);replies.pop_front();}}
 void pass(){if(++passes%10==0)TIMER0_COMPA_vect();actor();twi();uint8_t oldPhase=MainLinks::bus[1].phase;spi();if(MainBoard::active&&MainLinks::bus[0].phase&&MainLinks::bus[1].phase&&MainIO::txBit!=10)overlap=true;MainBoard::loop();if(oldPhase==1&&MainLinks::bus[1].phase==2)exchange(1);while(MainIO::tr!=MainIO::tw){USART_UDRE_vect();logs+=char(UDR0);}}
 void advance(uint32_t ms){for(uint32_t i=0;i<ms*10;i++)pass();}
 template<class F> void until(F predicate,uint32_t maxMs=1500){for(uint32_t i=0;i<maxMs*10&&!predicate();i++)pass();if(!predicate()){fprintf(stderr,"TIMEOUT t=%u ready=%u actor=%u sync=%u block=%u pending=%u flags=%u steps=%u,%u bus=%u,%u\n%s",unsigned(MainIO::ticks),MainBoard::ready,MainBoard::actorReady,MainBoard::safetySynced,MainBoard::adasBlocked,MainBoard::actorPending,MainBoard::adasFlags,MainBoard::workerStep[0],MainBoard::workerStep[1],MainLinks::bus[0].phase,MainLinks::bus[1].phase,logs.c_str());}if(!predicate()){fprintf(stderr,"actorFrames=%zu actorTx=%u/%u bit=%u PIND=%u danger=%u meta=%u healthy=%u age=%u at=%u clear=%u\n",actorCommands.size(),MainIO::xr,MainIO::xw,MainIO::txBit,PIND,MainIO::adasDanger,MainBoard::adasMeta,MainBoard::adasHealthy,MainBoard::adasAge,unsigned(MainBoard::adasAt),MainBoard::adasClear);for(auto f:actorCommands)fprintf(stderr,"op=%u id=%u len=%u\n",Proto::op(f),Proto::id(f),Proto::len(f));}assert(predicate());}
 void text(const char* s){while(*s){UCSR0A=0;UDR0=*s++;USART_RX_vect();}UDR0='\n';USART_RX_vect();}
 size_t count(uint8_t op)const{size_t n=0;for(const auto& f:actorCommands)if(Proto::op(f)==op)n++;return n;}
};
static void reset(){using namespace MainIO;ticks=0;pr=pw=tr=tw=pcOverflow=rr=rw=xr=xw=softOverflow=txFrameLeft=0;txBit=10;rxBit=255;memset(fakeEeprom,0xff,sizeof fakeEeprom);main_actor_reset();main_i2c_reset();main_spi_reset();MainBoard::fatal=false;MainBoard::lineSize=0;MainBoard::lineOverflow=false;MainBoard::diagnosticActive=MainBoard::diagnosticOverflow=false;MainBoard::diagnosticSize=0;PIND=_BV(PIND4);MainBoard::setup();}
extern "C" void main_actor_adas(bool);
extern "C" void adas_telemetry(uint8_t,uint8_t,uint16_t,uint16_t);
extern "C" void adas_shared(void*);
extern "C" void slm_shared(void*);
static void shared_state(uint8_t n,Proto::SharedState* p){if(n)slm_shared(p);else adas_shared(p);}
static void alert(bool danger){if(danger)PIND&=uint8_t(~_BV(PIND4));else PIND|=_BV(PIND4);PCINT2_vect();main_actor_adas(danger);}
static void snapshots(Simulation& sim){sim.advance(550);for(uint8_t n=0;n<2;n++){Proto::SharedState s;shared_state(n,&s);assert(s.session==MainBoard::session&&s.safetyEpoch==MainBoard::shared.safetyEpoch);assert(!s.desiredDrive&&!s.appliedDrive&&!s.desiredFlags&&!s.appliedFlags&&s.appliedKnown);}}
int main(){
 reset();Simulation baseline;baseline.until([&]{return MainBoard::ready;});baseline.text("go forward");baseline.until([&]{return baseline.count(Proto::ACTUATE)==1&&!MainBoard::actorPending;});assert(main_actor_drive()==1);baseline.text("stop");baseline.until([&]{return !MainBoard::actorPending&&main_actor_drive()==0;});snapshots(baseline);
 const char* plans[]={"go forward","go forward and turn on lights","go forward and turn on lights and turn on wipers","go forward and turn on lights and turn on wipers and turn on horn"};
 unsigned cases=0;
 for(int n=0;n<4;n++)for(int phase=0;phase<3;phase++){
  reset();Simulation sim;sim.until([&]{return MainBoard::ready;});
  if(phase==0)sim.holdWorkers=true;else sim.holdResult=true;
  sim.text(plans[n]);
  if(phase==0)sim.until([&]{return MainBoard::active;});
  else sim.until([&]{return MainBoard::scores.masks[1]==1;});
  uint32_t gen=MainBoard::generation;size_t acts=sim.count(Proto::ACTUATE)+sim.count(Proto::ACTUATE_BATCH);
  adas_telemetry(26,0,150,0);alert(true);assert(main_actor_drive()==0);
  if(phase==2){alert(false);adas_telemetry(8,3,300,0);}
  sim.advance(2);assert(!MainBoard::active&&MainBoard::generation!=gen&&!MainBoard::shared.desiredDrive);
  sim.holdWorkers=sim.holdResult=false;sim.advance(400);assert(sim.count(Proto::ACTUATE)+sim.count(Proto::ACTUATE_BATCH)==acts);assert(!main_actor_drive());
  alert(false);adas_telemetry(8,3,300,0);sim.until([&]{return MainBoard::ready;});sim.advance(200);assert(!main_actor_drive());cases++;
 }
 reset();Simulation invalid;invalid.until([&]{return MainBoard::ready;});adas_telemetry(8,3,300,201);invalid.advance(180);assert(!MainBoard::ready);invalid.text("go forward");invalid.advance(100);assert(!main_actor_drive());
 reset();Simulation contradicted;contradicted.until([&]{return MainBoard::ready;});alert(true);adas_telemetry(8,3,300,0);contradicted.advance(180);assert(!MainBoard::ready);contradicted.text("go forward");contradicted.advance(100);assert(!main_actor_drive());
 printf("PASS independent full register scheduler with actual SLM/ADAS/ACT engines: %u interrupt schedules, explicit STOP snapshots, stale and physical-line contradictions\n",cases);
}
