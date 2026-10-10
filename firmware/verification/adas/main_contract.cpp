#include <cstdio>
#include <initializer_list>
#ifndef MAIN_BOARD_HEADER
#define MAIN_BOARD_HEADER "../../main/src/board_app.h"
#endif
#include MAIN_BOARD_HEADER
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void clear(uint32_t t){MainIO::ticks=t;PIND=_BV(PIND4);MainIO::adasDanger=MainIO::adasEvent=0;MainBoard::adasMeta=MainBoard::adasHealthy=true;MainBoard::adasAt=t;MainBoard::adasAge=0;MainBoard::adasFlags=8;MainBoard::adasClear=3;MainBoard::adasBlocked=false;MainBoard::session=90;MainBoard::shared={90,1,5,1,1,1,7,7,0,true,0};MainBoard::safetySynced=true;MainBoard::stopNeeded=false;MainBoard::actorPending=false;MainBoard::generation=1;MainBoard::ready=true;MainIO::xr=MainIO::xw=MainIO::rr=MainIO::rw=0;MainIO::txFrameLeft=0;}
static void inject(const Proto::Frame& f){for(auto b:f.bytes){MainIO::rx[MainIO::rw]=b;MainIO::rw=(MainIO::rw+1)&63;}}
int main(){
 clear(100);CHECK(MainBoard::clearADAS(300));CHECK(!MainBoard::clearADAS(301));MainBoard::adasHealthy=false;CHECK(!MainBoard::clearADAS(100));clear(100);PIND=0;CHECK(!MainBoard::clearADAS(100));
 // 1..4 clause plans with interrupts before, during, and after result accumulation.
 for(int clauses=1;clauses<=4;clauses++)for(int phase=0;phase<3;phase++){clear(100);MainBoard::plan.count=clauses;MainBoard::clauseIndex=phase==2?clauses-1:0;MainBoard::active=true;MainBoard::scores.reset(7,MainBoard::generation);uint8_t p[24]={};for(int i=0;i<6;i++)Proto::putFloat(p+4*i,0.5f);Proto::Frame f;Proto::make(f,Proto::RESULT,7,0,p,24);if(phase)CHECK(MainBoard::scores.accept(1,f,MainBoard::generation));PIND=0;PCINT2_vect();MainBoard::priority(100);CHECK(!MainBoard::active&&!MainBoard::ready&&MainBoard::stopNeeded);CHECK(MainBoard::shared.desiredDrive==0&&MainBoard::shared.desiredFlags==0&&!MainBoard::shared.appliedKnown);CHECK(!MainBoard::scores.accept(1,f,MainBoard::generation));PIND=_BV(PIND4);PCINT2_vect();MainBoard::priority(101);MainBoard::priority(102);CHECK(!MainBoard::active&&MainBoard::shared.desiredDrive==0&&!MainBoard::safetySynced);}
 clear(100);MainBoard::stop("explicit_slm_stop",Proto::SLM_STOP);CHECK(MainBoard::shared.desiredDrive==0&&!MainBoard::shared.appliedKnown&&MainBoard::shared.stopSources==Proto::SLM_STOP);
 clear(100);MainBoard::shared.safetyEpoch=6;MainBoard::shared.desiredDrive=0;MainBoard::shared.appliedKnown=false;MainBoard::actorPending=true;MainBoard::actorOp=Proto::ACTUATE;MainBoard::actorCommand=7;MainBoard::actorAt=100;uint8_t p[18]={};Proto::put32(p,90);Proto::put32(p+4,5);p[8]=1;Proto::put16(p+14,7);Proto::Frame f;Proto::make(f,Proto::ACK,7,0,p,18);inject(f);MainBoard::receiveActor(100);CHECK(!MainBoard::shared.appliedKnown&&MainBoard::shared.desiredDrive==0&&MainBoard::actorPending);
 clear(100);Proto::put32(p+4,5);p[8]=0;p[9]=p[10]=p[11]=1;Proto::put16(p+16,Proto::DRIVE_LEASE);Proto::make(f,Proto::STATUS_REPLY,7,0,p,18);MainBoard::observe(f);CHECK(MainBoard::shared.desiredDrive==0&&MainBoard::shared.appliedDrive==0&&MainBoard::shared.appliedFlags==7&&MainBoard::shared.desiredFlags==7&&MainBoard::shared.appliedKnown);
 MainPolicy::Scores scores;scores.reset(3,20);for(int i=0;i<6;i++)Proto::putFloat(p+4*i,float(i));Proto::make(f,Proto::RESULT,3,0,p,24);CHECK(scores.accept(1,f,20));uint8_t tail[16];for(int i=0;i<4;i++)Proto::putFloat(tail+4*i,float(i+6));Proto::make(f,Proto::RESULT,3,1,tail,16);CHECK(scores.accept(1,f,20)&&scores.complete==1);for(int i=0;i<10;i++)CHECK(scores.z[i]==float(i));
 printf("PASS actual MAIN priority/shared-state/ACK/score policy %u assertions (host register shim)\n",checks);
}
