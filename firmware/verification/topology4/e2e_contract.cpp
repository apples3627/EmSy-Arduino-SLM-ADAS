#include <cassert>
#include <cstdio>
#include <vector>
#include MAIN_ENGINE_HEADER
#include "frame_helpers.h"
extern "C" void peer_reset();
extern "C" void peer_exchange(const uint8_t*,uint8_t*,uint32_t);
extern "C" void peer_alert(bool,bool);
extern "C" int peer_flags();
extern "C" void peer_snapshot(void*);
extern "C" void slm_reset();
extern "C" bool slm_exchange(const uint8_t*,uint8_t*,uint32_t);
extern "C" bool slm_command(const char*,uint8_t*,uint32_t);
extern "C" void slm_snapshot(void*);
extern "C" bool slm_pending();
struct Sim {
 MainVehicle::Controller c;uint32_t t=0;uint16_t sample=0;bool holdP=false,holdU=false;std::vector<Proto::Frame> held,requests;
 Sim(){peer_reset();slm_reset();c.init(100,0);c.alert(false,false,0);Proto::Frame req,r;Proto::make(req,Proto::HELLO,0);uint8_t p[8]={Proto::ADAS_ROLE,1,20,25,3,Proto::VERSION,70,0};Proto::make(r,Proto::ADAS_META,0,0,p,8);assert(c.acceptADAS(r,req,0));}
 void adas(){uint8_t p[18]={Proto::ADAS_ROLE,8};Proto::put16(p+2,300);Proto::put16(p+4,++sample);p[8]=20;p[9]=25;p[10]=p[11]=3;Proto::put16(p+12,70);Proto::Frame q,r;Proto::make(q,Proto::STATUS,0);Proto::make(r,Proto::ADAS_STATUS_REPLY,0,0,p,18);assert(c.acceptADAS(r,q,t));}
 void uart(){Proto::Frame q,r;while(c.uart.pop(q)){if(holdU)continue;if(slm_exchange(q.bytes,r.bytes,t))c.acceptUART(r,t);}}
 void periph(){Proto::Frame q,r;while(c.periph.pop(q)){requests.push_back(q);if(holdP){held.push_back(q);continue;}peer_exchange(q.bytes,r.bytes,t);c.acceptPeripheral(r,q,t);}}
 void step(){if(t%50==0)adas();c.tick(t);periph();uart();t++;}
 void advance(uint32_t n){while(n--)step();}
 void boot(){advance(20);if(!c.ready)fprintf(stderr,"BOOT failure ready=%u slm=%u periph=%u safe=%u ADAS=%u needs=%u epoch=%u flags=%u pendingP=%u\n",c.ready,c.slmReady,c.periphReady,c.periphSafe,c.adasClear,c.needsStop(),unsigned(c.state.safetyEpoch),c.peripheralFlags,c.awaitingPeripheral());assert(c.ready&&c.motorDirection==0);}
 Proto::Frame text(const char* s){uart();Proto::Frame f;if(!slm_command(s,f.bytes,t)){Proto::SharedState v;slm_snapshot(&v);fprintf(stderr,"SLM command blocked text=%s t=%u mainReady=%u epoch=%u slmEpoch=%u known=%u flags=%u pending=%u\n",s,unsigned(t),c.ready,unsigned(c.state.safetyEpoch),unsigned(v.safetyEpoch),v.appliedKnown,v.actorFlags,slm_pending());assert(false);}assert(c.acceptUART(f,t));return f;}
 size_t count(uint8_t op)const{size_t n=0;for(auto f:requests)if(Proto::op(f)==op)n++;return n;}
};
int main(){
 Sim s;s.boot();s.holdP=true;s.text("go forward and turn lights on and honk horn");s.advance(5);assert(s.c.pending&&!s.c.motorDirection&&peer_flags()==0);assert(!s.held.empty());auto request=s.held.back();Proto::Frame ack;peer_exchange(request.bytes,ack.bytes,s.t);auto bad=ack;Proto::put16(bad.bytes+3,Proto::id(ack)-1);bad.bytes[31]=Proto::crc8(bad.bytes,31);assert(!s.c.acceptPeripheral(bad,request,s.t)&&!s.c.motorDirection);assert(s.c.acceptPeripheral(ack,request,s.t)&&s.c.motorDirection==1&&peer_flags()==5);s.holdP=false;s.uart();uint32_t applied=s.t;s.advance(500);assert(s.c.motorDirection==1);s.step();assert(!s.c.motorDirection&&s.c.state.appliedDrive==0&&s.c.state.appliedFlags==5&&peer_flags()==5&&s.c.state.stopSources==Proto::DRIVE_LEASE);assert(s.t>=applied+500);
 s.text("stop");s.advance(600);assert(!s.c.motorDirection&&peer_flags()==0);Proto::SharedState a,b;slm_snapshot(&a);peer_snapshot(&b);assert(a.safetyEpoch==s.c.state.safetyEpoch&&b.safetyEpoch==s.c.state.safetyEpoch&&!a.desiredDrive&&!a.appliedDrive&&!b.desiredFlags&&!b.appliedFlags);
 Sim rev;rev.boot();rev.text("go forward");rev.advance(2);assert(rev.c.motorDirection==1);size_t batches=rev.count(Proto::ACTUATE_BATCH);rev.text("reverse and honk horn");uint32_t start=rev.t;assert(!rev.c.motorDirection&&rev.c.reversing);rev.advance(100);assert(rev.count(Proto::ACTUATE_BATCH)==batches&&!rev.c.motorDirection);rev.step();assert(rev.t>=start+100&&rev.c.motorDirection==-1&&peer_flags()==4);
 Sim pulse;pulse.boot();pulse.holdP=true;pulse.text("go forward and turn lights on");pulse.advance(2);assert(pulse.c.pending&&!pulse.held.empty());request=pulse.held.back();peer_exchange(request.bytes,ack.bytes,pulse.t);uint32_t epoch=pulse.c.state.safetyEpoch;pulse.c.alert(true,true,pulse.t);peer_alert(true,true);pulse.c.alert(false,false,pulse.t);peer_alert(false,false);assert(!pulse.c.motorDirection&&!pulse.c.pending&&pulse.c.state.safetyEpoch>epoch&&peer_flags()==0);assert(!pulse.c.acceptPeripheral(ack,request,pulse.t));pulse.holdP=false;pulse.advance(600);assert(!pulse.c.motorDirection&&peer_flags()==0&&pulse.c.ready);pulse.text("go forward");pulse.advance(2);assert(pulse.c.motorDirection==1);
 // A replayed prior STOP must not increment epoch or restore cached outputs.
 auto stopFrame=pulse.text("stop");pulse.advance(2);epoch=pulse.c.state.safetyEpoch;assert(pulse.c.acceptUART(stopFrame,pulse.t));pulse.advance(2);assert(pulse.c.state.safetyEpoch==epoch&&!pulse.c.motorDirection&&peer_flags()==0);
 Sim retry;retry.boot();retry.holdP=true;stopFrame=retry.text("stop");epoch=retry.c.state.safetyEpoch;assert(retry.c.acceptUART(stopFrame,retry.t));assert(retry.c.state.safetyEpoch==epoch);Proto::Frame response;while(retry.c.uart.pop(response))assert(Proto::op(response)!=Proto::ERROR_REPLY);retry.holdP=false;retry.advance(600);assert(retry.c.ready&&!retry.c.motorDirection);
 Sim cancel;cancel.boot();cancel.holdP=true;cancel.advance(85);assert(!cancel.held.empty());request=cancel.held.back();assert(Proto::op(request)==Proto::STATUS);peer_exchange(request.bytes,ack.bytes,cancel.t);cancel.text("stop");epoch=cancel.c.state.safetyEpoch;assert(!cancel.c.acceptPeripheral(ack,request,cancel.t));assert(cancel.c.state.safetyEpoch==epoch&&!cancel.c.motorDirection);cancel.holdP=false;cancel.advance(600);assert(cancel.c.ready);
 Sim wiper;wiper.boot();auto unsupported=batch(1,wiper.c.state.safetyEpoch,{Proto::FORWARD,Proto::LIGHT_ON,Proto::WIPER_ON});assert(!wiper.c.acceptUART(unsupported,wiper.t));assert(!wiper.c.motorDirection&&!wiper.c.state.desiredFlags);wiper.advance(600);auto dominates=batch(2,wiper.c.state.safetyEpoch,{Proto::FORWARD,Proto::REVERSE,Proto::WIPER_ON,Proto::ALL_STOP});assert(wiper.c.acceptUART(dominates,wiper.t));wiper.advance(600);assert(!wiper.c.motorDirection&&peer_flags()==0);
 Sim spi;spi.boot();spi.text("go forward");spi.advance(2);assert(spi.c.motorDirection==1);spi.holdP=true;spi.advance(1200);assert(!spi.c.motorDirection&&spi.c.state.stopSources==Proto::COMM_FAULT);
 Sim uart;uart.boot();uart.text("go forward");uart.advance(2);uart.holdU=true;uart.advance(2100);assert(!uart.c.motorDirection&&!uart.c.slmReady&&(uart.c.state.stopSources&Proto::COMM_FAULT));
 puts("PASS actual MAIN + SLM + peripheral engines: matching ACK, old ACK/epoch, STOP snapshots, 100ms reversal, 500ms lease, sticky pulse and link failure");
}
