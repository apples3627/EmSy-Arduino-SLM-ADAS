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
 uint32_t passes=0,computeUntil=0;uint8_t wireByte=0;int fault=0;bool holdStop=false,holdWorkers=false,holdResult=false,holdActAck=false,overlap=false;Proto::Frame lateActAck;
 void inject(const Proto::Frame& f){for(uint8_t b:f.bytes){uint8_t n=(MainIO::rw+1)&63;assert(n!=MainIO::rr);MainIO::rx[MainIO::rw]=b;MainIO::rw=n;}}
 void exchange(uint8_t w){assert(Proto::valid(writes[w]));workerCommands[w].push_back(writes[w]);if(w)main_spi_exchange(writes[w].bytes,cached[w].bytes);else main_i2c_exchange(writes[w].bytes,cached[w].bytes);if(Proto::op(cached[w])==Proto::STATUS_REPLY&&computeUntil>MainIO::ticks){Proto::payload(cached[w])[0]=Proto::COMPUTING;cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(Proto::op(cached[w])==Proto::RESULT&&w==1){if(fault==1){Proto::put16(cached[w].bytes+3,Proto::id(cached[w])-1);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==2){Proto::putFloat(Proto::payload(cached[w]),NAN);cached[w].bytes[31]=Proto::crc8(cached[w].bytes,31);}if(fault==3)cached[w].bytes[31]^=1;}}
 void twi(){auto& b=MainLinks::bus[0];if(holdWorkers&&MainBoard::active)return;if(b.phase==4){exchange(0);TWCR&=~_BV(TWSTO);return;}if(b.phase==9||b.phase==10||b.phase==11){TWCR&=~_BV(TWSTO);return;}uint8_t state=0;if(b.phase==1||b.phase==6)state=8;else if(b.phase==2){if(!b.index)state=0x18;else{writes[0].bytes[b.index-1]=TWDR;state=0x28;}}else if(b.phase==7)state=0x40;else if(b.phase==8){TWDR=cached[0].bytes[b.index];state=b.index==31?0x58:0x50;}else return;TWSR=state;TWCR|=_BV(TWINT);TWI_vect();}
 void spi(){auto& b=MainLinks::bus[1];if(holdWorkers&&MainBoard::active)return;if(holdResult&&Proto::op(b.request)==Proto::GET_RESULT&&Proto::seq(b.request)==1)return;if(b.phase==1){writes[1].bytes[b.index]=SPDR;SPDR=0xff;SPSR|=_BV(SPIF);}else if(b.phase==3){SPDR=cached[1].bytes[b.index];SPSR|=_BV(SPIF);}else SPSR&=~_BV(SPIF);}
 void actor(){uint8_t old=MainIO::txBit;TIMER1_COMPA_vect();bool bit=PORTD&_BV(PORTD3);if(old==10&&MainIO::txBit==0){assert(!bit);wireByte=0;}else if(old<8){if(bit)wireByte|=1<<old;}else if(old==8){assert(bit);Proto::Frame f;if(uart.push(wireByte,f)){actorCommands.push_back(f);Proto::Frame r;if(main_actor_exchange(f.bytes,r.bytes,MainIO::ticks)){if(holdActAck&&Proto::op(f)==Proto::ACTUATE)lateActAck=r;else if(!(holdStop&&Proto::op(f)==Proto::STOP_ALL))replies.push_back({r,MainIO::ticks+5});}}}else if(old==9)assert(bit);
 Proto::Frame r;if(main_actor_tick(r.bytes,MainIO::ticks))replies.push_back({r,MainIO::ticks+5});if(!replies.empty()&&int32_t(MainIO::ticks-replies.front().due)>=0){inject(replies.front().frame);replies.pop_front();}}
 void pass(){if(++passes%10==0)TIMER0_COMPA_vect();actor();twi();uint8_t oldPhase=MainLinks::bus[1].phase;spi();if(MainBoard::active&&MainLinks::bus[0].phase&&MainLinks::bus[1].phase&&MainIO::txBit!=10)overlap=true;MainBoard::loop();if(oldPhase==1&&MainLinks::bus[1].phase==2)exchange(1);while(MainIO::tr!=MainIO::tw){USART_UDRE_vect();logs+=char(UDR0);}}
 void advance(uint32_t ms){for(uint32_t i=0;i<ms*10;i++)pass();}
 template<class F> void until(F predicate,uint32_t maxMs=1500){for(uint32_t i=0;i<maxMs*10&&!predicate();i++)pass();assert(predicate());}
 void text(const char* s){while(*s){UCSR0A=0;UDR0=*s++;USART_RX_vect();}UDR0='\n';USART_RX_vect();}
 size_t count(uint8_t op)const{size_t n=0;for(const auto& f:actorCommands)if(Proto::op(f)==op)n++;return n;}
};
static void reset(){using namespace MainIO;ticks=0;pr=pw=tr=tw=pcOverflow=rr=rw=xr=xw=softOverflow=txFrameLeft=0;txBit=10;rxBit=255;memset(fakeEeprom,0xff,sizeof fakeEeprom);main_actor_reset();main_i2c_reset();main_spi_reset();MainBoard::fatal=false;MainBoard::lineSize=0;MainBoard::lineOverflow=false;MainBoard::setup();}
static void softRx(){MainIO::rr=MainIO::rw=0;MainIO::rxBit=255;TCNT1=200;PIND=0;INT0_vect();assert(OCR1B==96);TIMER1_COMPB_vect();uint8_t v=0xa6;for(uint8_t i=0;i<8;i++){PIND=(v&(1<<i))?_BV(PIND2):0;TIMER1_COMPB_vect();}PIND=_BV(PIND2);TIMER1_COMPB_vect();assert(MainIO::rxBit==255&&MainIO::rx[0]==v&&MainIO::rw==1);MainIO::rr=MainIO::rw=0;}
int main(){
 // Foreground timeout must run even when TWI never interrupts, including bootstrap.
 reset();Proto::Frame missing;Proto::make(missing,Proto::HELLO,0);MainLinks::begin(0,missing,0);MainIO::ticks=100;MainLinks::service(0,100);assert(MainLinks::bus[0].phase==10&&!MainLinks::bus[0].done);MainLinks::service(0,199);assert(MainLinks::bus[0].phase==10);MainLinks::service(0,200);assert(MainLinks::bus[0].phase==0&&MainLinks::bus[0].done&&!MainLinks::bus[0].ok&&TWCR==_BV(TWEN));
 reset();softRx();Simulation boot;boot.holdStop=true;boot.until([&]{return boot.count(Proto::STOP_ALL)==1;});boot.advance(100);assert(MainBoard::actorReady&&MainBoard::workerReady[0]&&MainBoard::workerReady[1]);assert(!MainBoard::ready&&MainBoard::actorPending);assert(boot.logs.find("READY ")==std::string::npos);boot.holdStop=false;boot.until([&]{return MainBoard::ready;});assert(!MainBoard::actorPending&&!MainBoard::stopNeeded);assert(boot.logs.find("READY ")!=std::string::npos);
 boot.text("go forward");boot.until([&]{return boot.count(Proto::ACTUATE)==1;});boot.until([&]{return !MainBoard::actorPending;});assert(main_actor_drive()==1);assert(boot.logs.find("ERROR busy")==std::string::npos);assert(boot.overlap);for(int w=0;w<2;w++){int chunks=0,start=0,result=0;for(const auto& f:boot.workerCommands[w]){if(Proto::op(f)==Proto::LOAD_INPUT){assert(Proto::len(f)==(Proto::seq(f)?8:24));chunks++;}if(Proto::op(f)==Proto::START)start++;if(Proto::op(f)==Proto::GET_RESULT)result++;}assert(chunks==2&&start==1&&result==2);}float a[10],b[10];main_i2c_scores(a);main_spi_scores(b);for(int i=0;i<10;i++)assert(std::fabs(MainBoard::scores.z[i]-(a[i]+b[i]+rdf(IntentMain::C+i)))<1e-5f);
 // Reverse is acknowledged only after the real actor's 100 ms neutral deadtime.
 boot.text("go backward");boot.until([&]{return boot.count(Proto::ACTUATE)==2;});assert(MainBoard::actorPending&&main_actor_drive()==0);boot.advance(50);assert(MainBoard::actorPending&&main_actor_drive()==0);boot.until([&]{return !MainBoard::actorPending;});assert(main_actor_drive()==-1);
 // Actual USB ISR rejects framing/overrun/parity errors and requests global stop.
 for(uint8_t error:{uint8_t(FE0),uint8_t(DOR0),uint8_t(UPE0)}){uint8_t old=MainIO::pw;UCSR0A=_BV(error);UDR0='x';USART_RX_vect();assert(MainIO::pw==old&&MainIO::pcOverflow);MainBoard::loop();assert(!MainBoard::active);}UCSR0A=0;
 for(int fault=1;fault<=3;fault++){reset();Simulation sim;sim.until([&]{return MainBoard::ready;});sim.fault=fault;sim.text("go forward");sim.until([&]{return sim.count(Proto::STOP_ALL)>=2;});assert(sim.count(Proto::ACTUATE)==0&&!MainBoard::active&&!MainBoard::ready);assert(main_actor_drive()==0);}
 // Withhold both actual bus machines; literal STOP cancels inference before late results.
 reset();Simulation stopped;stopped.until([&]{return MainBoard::ready;});stopped.holdWorkers=true;stopped.text("go forward");stopped.advance(10);assert(MainBoard::active);uint32_t gen=MainBoard::generation;stopped.text("  StOp!?  ");stopped.advance(1);assert(!MainBoard::active&&MainBoard::generation!=gen);stopped.holdWorkers=false;stopped.advance(400);assert(stopped.count(Proto::ACTUATE)==0);assert(main_actor_drive()==0);
 // Missing results have bounded transaction failure and no single-worker fallback.
 reset();Simulation delayed;delayed.until([&]{return MainBoard::ready;});delayed.holdWorkers=true;delayed.text("go forward");delayed.until([&]{return delayed.count(Proto::STOP_ALL)>=2;});assert(delayed.count(Proto::ACTUATE)==0&&!MainBoard::ready);
 // Two workers remain COMPUTING for 3 seconds; STATUS carries the job tag and keeps actor session alive.
 reset();Simulation longJob;longJob.until([&]{return MainBoard::ready;});longJob.computeUntil=MainIO::ticks+3000;longJob.text("go forward");longJob.until([&]{return longJob.count(Proto::ACTUATE)==1;},4500);longJob.until([&]{return !MainBoard::actorPending;});assert(main_actor_drive()==1);int queries=0;for(const auto& f:longJob.actorCommands)if(Proto::op(f)==Proto::STATUS){assert(Proto::id(f)==MainBoard::job);queries++;}assert(queries>=5);assert(longJob.logs.find("bad_session")==std::string::npos);
 // More than half the uint16 range of idle queries never spends command IDs.
 reset();Simulation idle;idle.until([&]{return MainBoard::ready;});uint16_t saved=MainBoard::counter;for(uint32_t i=0;i<32769;i++){MainIO::ticks+=500;idle.advance(45);assert(!MainBoard::actorPending&&MainBoard::ready);assert(MainBoard::counter==saved);}idle.text("go forward");idle.until([&]{return idle.count(Proto::ACTUATE)==1;});idle.until([&]{return !MainBoard::actorPending;});assert(main_actor_drive()==1);
 // STOP after one worker result and one partial cancels the other worker's late final chunk.
 reset();Simulation lateResult;lateResult.until([&]{return MainBoard::ready;});lateResult.holdResult=true;lateResult.text("go forward");lateResult.until([&]{return MainBoard::scores.complete==1&&MainBoard::scores.masks[1]==1;});lateResult.text("stop");lateResult.advance(1);lateResult.holdResult=false;lateResult.advance(400);assert(lateResult.count(Proto::ACTUATE)==0&&main_actor_drive()==0);
 // An ACT ACK delayed past a newer STOP is never reported APPLIED.
 reset();Simulation lateAck;lateAck.until([&]{return MainBoard::ready;});lateAck.holdActAck=true;lateAck.text("go forward");lateAck.until([&]{return lateAck.count(Proto::ACTUATE)==1;});assert(MainBoard::actorPending&&main_actor_drive()==1);uint16_t stale=Proto::id(lateAck.lateActAck);lateAck.text("stop");lateAck.until([&]{return lateAck.count(Proto::STOP_ALL)==2&&!MainBoard::actorPending;});lateAck.inject(lateAck.lateActAck);lateAck.advance(5);assert(main_actor_drive()==0);assert(lateAck.logs.find("APPLIED cmd="+std::to_string(stale)+" ")==std::string::npos);
 // A full 32-byte UART queue defers the next frame without losing its pending command.
 reset();Simulation full;full.until([&]{return MainBoard::ready;});MainBoard::actorIssue(Proto::STATUS,MainBoard::counter);assert(((MainIO::xw-MainIO::xr)&63)==32);MainBoard::action(Proto::FORWARD);assert(MainBoard::actorPending&&MainBoard::actorOp==Proto::ACTUATE);full.until([&]{return full.count(Proto::ACTUATE)==1&&!MainBoard::actorPending;});assert(main_actor_drive()==1);
 // EEPROM erased marker is reserved, and incrementing the final usable session is forbidden.
 MainIO::persist(0xfffffffeUL);MainBoard::boot();assert(MainBoard::fatal&&!MainBoard::ready);
 puts("MAIN actual register scheduler + real actor/two worker bridges: PASS");
}
