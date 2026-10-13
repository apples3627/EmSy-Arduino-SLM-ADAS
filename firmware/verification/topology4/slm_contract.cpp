#include <cassert>
#include <cmath>
#include <cstdio>
#include SLM_SESSION_HEADER
#include SLM_PLANNER_HEADER
#include "frame_helpers.h"
#include "frozen_scores.h"
static void plan(SlmUart::Planner& p,const char* text,uint32_t g=1,uint32_t epoch=1){assert(p.begin(text,strlen(text),g,epoch));while(p.active){char clause[97];Intent::Input in;float z[10];bool kernel=p.prepare(text,clause,in);uint8_t effective=7;if(kernel){Intent::logits(in,z);++p.kernels;effective=Intent::decide(in,z).effective;}p.accept(effective);}}
int main(){
 const char* samples[]={"forward","reverse","stop","turn lights on","turn lights off","start wipers","stop wipers","honk horn","silence horn","please move forward"};
 for(int repeat=0;repeat<5;repeat++)for(int n=0;n<10;n++){Intent::Input in;Intent::parse(samples[n],strlen(samples[n]),in);assert(in.reason==Intent::OK);float z[10];Intent::logits(in,z);for(int i=0;i<10;i++)assert(std::fabs(z[i]-frozenScores[n][i])<1e-5f);}
 SlmUart::Planner p;plan(p," STOP!!! ");assert(p.decisions.stop&&p.kernels==0);plan(p,"go forward and start wipers");assert(p.decisions.rejected&&p.decisions.count==0);plan(p,"go forward and start wipers and stop");assert(p.decisions.stop&&!p.decisions.rejected&&p.decisions.count==1);plan(p,"do not stop");assert(!p.decisions.stop&&p.kernels==0);
 SlmUart::Session s;Proto::Frame r;auto q=hello();assert(s.receive(q,0,r)&&Proto::slmCompatible(r,100));
 Proto::SharedState st={100,1,1,2,0,0,0,0,0,true,0};q=sync(st);assert(!s.receive(q,1,r)&&s.ready(1));uint8_t ids[]={Proto::FORWARD,Proto::LIGHT_ON,Proto::HORN_ON};assert(s.issuePlan(ids,3,2));
 Proto::StreamParser parser;Proto::Frame framed;for(int n=0;n<32;n++)assert(parser.push(s.pendingFrame.bytes[n],framed)==(n==31));assert(Proto::valid(framed));
 uint32_t oldg=s.generation;plan(p,"go forward",oldg,1);st.revision=2;st.safetyEpoch=2;st.appliedKnown=false;st.actorFlags=8;q=sync(st);s.receive(q,3,r);assert(!s.pending&&s.stopNeeded&&s.generation!=oldg&&!p.valid(s.generation,s.epoch())&&!s.ready(3));
 assert(s.issueStop(4));uint16_t stopId=Proto::id(s.pendingFrame);uint8_t a[18]={0};Proto::put32(a,100);Proto::put32(a+4,2);Proto::put16(a+14,stopId);Proto::make(q,Proto::ACK,stopId,0,a,18);s.receive(q,5,r);assert(s.pending);Proto::put32(a+4,3);Proto::make(q,Proto::ACK,stopId,0,a,18);s.receive(q,6,r);assert(!s.pending&&s.ready(6));
 st.safetyEpoch=1;st.revision=1;q=sync(st);s.receive(q,7,r);assert(s.epoch()==3);s.tick(2006);assert(!s.ready(2006)&&s.stopNeeded);
 puts("PASS actual SLM full NN (50 golden evaluations), STOP/guard/wiper planner, UART framing and epoch cancellation");
}
