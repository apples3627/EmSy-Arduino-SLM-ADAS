#include <cassert>
#include <cstdio>
#include "../main/src/control_engine.h"
static void heartbeat(MainVehicle::Controller& c,uint32_t now){Proto::Frame f;Proto::slmMeta(f,c.state.session);assert(c.acceptUART(f,now));}
static Proto::Frame plan(MainVehicle::Controller& c,uint16_t id,uint8_t intent,uint16_t duration=0){uint8_t p[12]={};Proto::put32(p,c.state.session);Proto::put32(p+4,c.state.safetyEpoch);p[8]=1;p[9]=intent;Proto::put16(p+10,duration);Proto::Frame f;Proto::make(f,Proto::ACTUATE_BATCH,id,0,p,12);return f;}
int main(){
 MainVehicle::Controller c;c.init(100,0);heartbeat(c,0);c.alert(true,true,0);c.tick(0);
 assert(c.acceptUART(plan(c,1,Proto::FORWARD),1));c.tick(1);assert(c.motorDirection==1);
 for(uint32_t now=500;now<=10000;now+=500){heartbeat(c,now);c.tick(now);assert(c.motorDirection==1&&c.state.appliedDrive==1);}
 // Missing accessories cannot time out a continuous motor command.
 assert(c.acceptUART(plan(c,2,Proto::LIGHT_ON,500),10001));c.tick(10001);heartbeat(c,11000);c.tick(11000);assert(c.motorDirection==1);
 assert(c.acceptUART(plan(c,3,Proto::REVERSE),11001));c.tick(11001);assert(c.reversing&&!c.motorDirection);c.tick(11100);assert(!c.motorDirection);c.tick(11101);assert(c.motorDirection==-1);
 uint8_t p[10]={};Proto::put32(p,c.state.session);Proto::put32(p+4,c.state.safetyEpoch);Proto::Frame stop;Proto::make(stop,Proto::STOP_ALL,4,0,p,10);assert(c.acceptUART(stop,11102));assert(!c.motorDirection&&c.state.appliedDrive==0);
 // Explicit timed motion is still available and an accessory plan cannot renew its lease.
 assert(c.acceptUART(plan(c,5,Proto::FORWARD,500),11200));c.tick(11200);assert(c.motorDirection==1);
 assert(c.acceptUART(plan(c,6,Proto::HORN_OFF,0),11500));c.tick(11500);c.tick(11699);assert(c.motorDirection==1);c.tick(11700);assert(!c.motorDirection&&c.state.stopSources==Proto::DRIVE_LEASE);
 assert(c.acceptUART(plan(c,7,Proto::FORWARD),11800));c.tick(11800);assert(c.motorDirection==1);
 // Continuous motion must still stop on loss of the live UART authority.
 c.tick(13799);assert(c.motorDirection==1);c.tick(13800);assert(!c.motorDirection&&!c.slmReady&&(c.state.stopSources&Proto::COMM_FAULT));
 heartbeat(c,13801);c.tick(13801);assert(c.ready);
 assert(c.acceptUART(plan(c,8,Proto::FORWARD,500),13802));c.tick(13802);
 // An auxiliary plan staged just before the deadline cannot revive expired motion.
 assert(c.acceptUART(plan(c,9,Proto::LIGHT_ON),14301));c.tick(14302);
 assert(!c.motorDirection&&c.state.desiredDrive==0&&c.state.appliedDrive==0);
 c.tick(14303);assert(!c.motorDirection);
 puts("PASS continuous motion >10s, STOP, reverse deadtime, accessory independence, timed move and UART watchdog");
}
