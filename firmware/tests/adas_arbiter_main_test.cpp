#include <assert.h>
#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include <vector>
#include "../main/src/board_app.h"
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
 void exchange(uint8_t w){assert(Proto::valid(writes[w]));workerCommands[w].push_back(writes[w]);if(w)main_spi_exchange(writes[w].bytes,cached[w].bytes);else main_i2c_exchange(writes[w].bytes,cached[w].bytes);if(Proto::op(cached[w])==Proto::STATUS_REPLY&&computeUntil>MainIO::ticks){Proto::payload(cached[w])[0]=Proto::COMPUTING;cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(Proto::op(cached[w])==Proto::RESULT&&w==1){if(fault==1){Proto::put16(cached[w].bytes+3,Proto::id(cached[w])-1);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==2){Proto::putFloat(Proto::payload(cached[w]),NAN);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==3)cached[w].bytes[31]^=1;}}
 void twi(){auto& b=MainLinks::bus[0];if((holdWorkers||(holdSecond&&MainBoard::clauseIndex==1))&&MainBoard::active)return;if(b.phase==4){exchange(0);TWCR&=~_BV(TWSTO);return;}if(b.phase==9||b.phase==10||b.phase==11){TWCR&=~_BV(TWSTO);return;}uint8_t state=0;if(b.phase==1||b.phase==6)state=8;else if(b.phase==2){if(!b.index)state=0x18;else{writes[0].bytes[b.index-1]=TWDR;state=0x28;}}else if(b.phase==7)state=0x40;else if(b.phase==8){TWDR=cached[0].bytes[b.index];state=b.index==31?0x58:0x50;}else return;TWSR=state;TWCR|=_BV(TWINT);TWI_vect();}
 void spi(){auto& b=MainLinks::bus[1];if((holdWorkers||(holdSecond&&MainBoard::clauseIndex==1))&&MainBoard::active)return;if(holdResult&&Proto::op(b.request)==Proto::GET_RESULT&&Proto::seq(b.request)==1)return;if(b.phase==1){writes[1].bytes[b.index]=SPDR;SPDR=0xff;SPSR|=_BV(SPIF);}else if(b.phase==3){SPDR=cached[1].bytes[b.index];SPSR|=_BV(SPIF);}else SPSR&=~_BV(SPIF);}
 void actor(){uint8_t old=MainIO::txBit;TIMER1_COMPA_vect();bool bit=PORTD&_BV(PORTD3);if(old==10&&MainIO::txBit==0){assert(!bit);wireByte=0;}else if(old<8){if(bit)wireByte|=1<<old;}else if(old==8){assert(bit);Proto::Frame f;if(uart.push(wireByte,f)){actorCommands.push_back(f);Proto::Frame r;if(main_actor_exchange(f.bytes,r.bytes,MainIO::ticks)){if(holdActAck&&(Proto::op(f)==Proto::ACTUATE||Proto::op(f)==Proto::ACTUATE_BATCH))lateActAck=r;else if(!(holdStop&&Proto::op(f)==Proto::STOP_ALL))replies.push_back({r,MainIO::ticks+5});}}}else if(old==9)assert(bit);
 Proto::Frame r;if(main_actor_tick(r.bytes,MainIO::ticks))replies.push_back({r,MainIO::ticks+5});if(!replies.empty()&&int32_t(MainIO::ticks-replies.front().due)>=0){inject(replies.front().frame);replies.pop_front();}}
 void pass(){if(++passes%10==0)TIMER0_COMPA_vect();actor();twi();uint8_t oldPhase=MainLinks::bus[1].phase;spi();if(MainBoard::active&&MainLinks::bus[0].phase&&MainLinks::bus[1].phase&&MainIO::txBit!=10)overlap=true;MainBoard::loop();if(oldPhase==1&&MainLinks::bus[1].phase==2)exchange(1);while(MainIO::tr!=MainIO::tw){USART_UDRE_vect();logs+=char(UDR0);}}
 void advance(uint32_t ms){for(uint32_t i=0;i<ms*10;i++)pass();}
 template<class F> void until(F predicate,uint32_t maxMs=1500){for(uint32_t i=0;i<maxMs*10&&!predicate();i++)pass();if(!predicate()){fprintf(stderr,"TIMEOUT t=%u ready=%u actor=%u sync=%u block=%u pending=%u flags=%u steps=%u,%u bus=%u,%u\n%s",unsigned(MainIO::ticks),MainBoard::ready,MainBoard::actorReady,MainBoard::safetySynced,MainBoard::adasBlocked,MainBoard::actorPending,MainBoard::adasFlags,MainBoard::workerStep[0],MainBoard::workerStep[1],MainLinks::bus[0].phase,MainLinks::bus[1].phase,logs.c_str());}assert(predicate());}
 void text(const char* s){while(*s){UCSR0A=0;UDR0=*s++;USART_RX_vect();}UDR0='\n';USART_RX_vect();}
 size_t count(uint8_t op)const{size_t n=0;for(const auto& f:actorCommands)if(Proto::op(f)==op)n++;return n;}
};
static void reset(){using namespace MainIO;ticks=0;pr=pw=tr=tw=pcOverflow=rr=rw=xr=xw=softOverflow=txFrameLeft=0;txBit=10;rxBit=255;memset(fakeEeprom,0xff,sizeof fakeEeprom);main_actor_reset();main_i2c_reset();main_spi_reset();MainBoard::fatal=false;MainBoard::lineSize=0;MainBoard::lineOverflow=false;MainBoard::diagnosticActive=MainBoard::diagnosticOverflow=false;MainBoard::diagnosticSize=0;PIND=_BV(PIND4);MainBoard::setup();}
extern "C" void main_actor_adas(bool);
extern "C" void adas_freeze(bool);
extern "C" void adas_telemetry(uint8_t,uint8_t,uint16_t,uint16_t);
extern "C" void shared_state(uint8_t,Proto::SharedState*);
static void alert(bool danger){if(danger)PIND&=uint8_t(~_BV(PIND4));else PIND|=_BV(PIND4);PCINT2_vect();main_actor_adas(danger);}
static void snapshots(Simulation& sim){sim.advance(550);for(uint8_t n=0;n<2;n++){Proto::SharedState s;shared_state(n,&s);assert(s.session==MainBoard::session&&s.safetyEpoch==MainBoard::shared.safetyEpoch);assert(!s.desiredDrive&&!s.appliedDrive&&!s.desiredFlags&&!s.appliedFlags&&s.appliedKnown);}}
int main(){
 reset();Simulation sim;sim.until([&]{return MainBoard::ready;});assert(MainBoard::safetySynced);sim.text("go forward");sim.until([&]{return sim.count(Proto::ACTUATE)==1&&!MainBoard::actorPending;});assert(main_actor_drive()==1&&MainBoard::shared.appliedKnown&&MainBoard::shared.desiredDrive==1);Proto::Frame oldAct=sim.actorCommands.back();
 float full[10];main_spi_scores(full);for(int i=0;i<10;i++)assert(fabs(MainBoard::scores.z[i]-full[i])<1e-6);for(auto f:sim.workerCommands[0])assert(Proto::op(f)!=Proto::LOAD_INPUT&&Proto::op(f)!=Proto::START);
 // ACT off independently before MAIN next foreground pass; MAIN keeps actual unknown until an ACK.
 uint32_t oldEpoch=MainBoard::shared.safetyEpoch;adas_telemetry(26,0,150,0);alert(true);assert(main_actor_drive()==0);MainBoard::loop();assert(!MainBoard::shared.desiredDrive&&!MainBoard::shared.appliedKnown&&MainBoard::shared.safetyEpoch>oldEpoch);sim.until([&]{return !MainBoard::actorPending;});snapshots(sim);assert(!MainBoard::ready);
 // Contradictory healthy telemetry cannot override a physically low sensor line.
 adas_telemetry(8,3,300,0);sim.advance(200);assert(!MainBoard::ready);sim.text("turn on lights");sim.advance(200);assert(main_actor_drive()==0);
 alert(false);sim.until([&]{return MainBoard::ready;});uint32_t newEpoch=MainBoard::shared.safetyEpoch;assert(newEpoch>oldEpoch);sim.advance(200);assert(main_actor_drive()==0);Proto::Frame rejected;assert(main_actor_exchange(oldAct.bytes,rejected.bytes,MainIO::ticks));assert(Proto::op(rejected)==Proto::ERROR_REPLY);
 sim.text("go forward");size_t before=sim.count(Proto::ACTUATE);sim.until([&]{return sim.count(Proto::ACTUATE)>before&&!MainBoard::actorPending;});assert(main_actor_drive()==1);sim.text("stop");sim.until([&]{return !MainBoard::actorPending&&main_actor_drive()==0;});snapshots(sim);assert(MainBoard::shared.stopSources==Proto::SLM_STOP);
 // Sensor fault/stale samples globally inhibit accessories as well as both drive directions.
 adas_telemetry(4,0,0,0);sim.advance(180);assert(!MainBoard::ready);sim.text("go backward");sim.advance(180);assert(!main_actor_drive());adas_telemetry(8,3,300,201);sim.advance(180);assert(!MainBoard::ready);adas_telemetry(8,3,300,0);sim.until([&]{return MainBoard::ready;});
 // Event after neural reply and before final UART action cancels the job.
 sim.holdResult=true;sim.text("go forward and turn on lights");sim.until([&]{return MainBoard::scores.masks[1]==1;});before=sim.count(Proto::ACTUATE_BATCH);adas_telemetry(26,0,150,0);alert(true);sim.holdResult=false;sim.advance(400);assert(sim.count(Proto::ACTUATE_BATCH)==before&&!main_actor_drive()&&!MainBoard::active);
 alert(false);adas_telemetry(8,3,300,0);sim.until([&]{return MainBoard::ready;});sim.text("go forward");before=sim.count(Proto::ACTUATE);sim.until([&]{return sim.count(Proto::ACTUATE)>before&&!MainBoard::actorPending;});sim.advance(650);assert(!MainBoard::shared.desiredDrive&&!MainBoard::shared.appliedDrive);
 uint32_t generation=MainBoard::generation;alert(true);alert(false);MainBoard::loop();assert(MainBoard::generation>generation&&!MainBoard::shared.desiredDrive);sim.until([&]{return MainBoard::ready;});assert(!main_actor_drive());adas_freeze(true);sim.advance(300);assert(!MainBoard::ready);adas_freeze(false);sim.until([&]{return MainBoard::ready;});
 puts("ADAS arbiter actual MAIN register scheduler + actual ACT Engine + full model fixture: PASS");
}
