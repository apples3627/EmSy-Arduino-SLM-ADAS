#include <cassert>
#include <initializer_list>
#include <cmath>
#include <cstdio>
#include "frozen_scores.h"
#ifndef SLM_WORKER_HEADER
#define SLM_WORKER_HEADER "../../slm_spi/src/ownengine/adapters/worker.h"
#endif
#include SLM_WORKER_HEADER
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(){
 Slm::Worker w;Proto::Frame q,r;uint8_t sid[4];Proto::put32(sid,123);Proto::make(q,Proto::HELLO,1,0,sid,4);w.handle(q,r);CHECK(Proto::fullSlmCompatible(r));
 const char* samples[]={"forward","reverse","stop","turn lights on","turn lights off","start wipers","stop wipers","honk horn","silence horn","please move forward"};
 for(unsigned n=0;n<10;n++){Intent::Input in;Intent::parse(samples[n],strlen(samples[n]),in);CHECK(in.reason==Intent::OK);float expected[10];Intent::logits(in,expected);uint8_t data[24];for(int part=0;part<2;part++){int count=part?4:12,first=part?12:0;for(int i=0;i<count;i++)Proto::put16(data+i*2,in.tokens[first+i]);Proto::make(q,Proto::LOAD_INPUT,10+n,part,data,count*2);w.handle(q,r);CHECK(Proto::op(r)==Proto::ACK);}Proto::make(q,Proto::START,10+n);w.handle(q,r);CHECK(w.computeIfPending());CHECK(w.state==Proto::RESULT_READY);for(int i=0;i<10;i++)CHECK(std::fabs(expected[i]-w.scores[i])<1e-6f&&std::fabs(frozenScores[n][i]-w.scores[i])<1e-5f);}
 Proto::SharedState s={123,2,9,42,0,0,0,0,Proto::SLM_STOP,true,0};uint8_t state[22];Proto::encodeState(state,s);Proto::make(q,Proto::STATE_SYNC,90,0,state,22);w.handle(q,r);CHECK(w.snapshot.known&&w.snapshot.shared.desiredDrive==0&&w.snapshot.shared.appliedDrive==0);state[14]=1;Proto::make(q,Proto::STATE_SYNC,91,0,state,22);w.handle(q,r);CHECK(Proto::op(r)==Proto::ERROR_REPLY&&w.snapshot.shared.desiredDrive==0);s.revision=1;Proto::encodeState(state,s);Proto::make(q,Proto::STATE_SYNC,92,0,state,22);w.handle(q,r);CHECK(Proto::op(r)==Proto::ERROR_REPLY&&w.snapshot.shared.revision==2);
 // State dissemination is orthogonal to an in-progress neural job.
 w.state=Proto::COMPUTING;w.pending=true;uint16_t before=w.tokens[0],jobBefore=w.job;s.revision=3;Proto::encodeState(state,s);Proto::make(q,Proto::STATE_SYNC,93,0,state,22);w.handle(q,r);CHECK(w.state==Proto::COMPUTING&&w.pending&&w.job==jobBefore&&w.tokens[0]==before);
 uint8_t load[24];for(int i=0;i<12;i++)Proto::put16(load+2*i,w.tokens[i]);Proto::make(q,Proto::LOAD_INPUT,w.job,0,load,24);w.handle(q,r);CHECK(Proto::op(r)==Proto::ACK);Proto::put16(load,(w.tokens[0]+1)%245);Proto::make(q,Proto::LOAD_INPUT,w.job,0,load,24);w.handle(q,r);CHECK(Proto::op(r)==Proto::ERROR_REPLY&&w.tokens[0]==before);
 printf("PASS SLM worker %u assertions\n",checks);
}
