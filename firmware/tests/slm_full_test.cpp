#include <cassert>
#include <cstdio>
#include "../slm_spi/src/ownengine/adapters/worker.h"
#include "../slm_spi/src/ownengine/adapters/transport.h"
static Proto::Frame request(uint8_t op,uint16_t id,uint8_t seq=0,const uint8_t* p=0,uint8_t n=0){Proto::Frame f;assert(Proto::make(f,op,id,seq,p,n));return f;}
static void expect(Slm::Worker& w,const Proto::Frame& q,uint8_t op,uint8_t err=0){Proto::Frame r;w.handle(q,r);assert(Proto::valid(r));assert(Proto::op(r)==op);assert(Proto::id(r)==Proto::id(q));assert(Proto::seq(r)==Proto::seq(q));if(op==Proto::ERROR_REPLY)assert(Proto::payload(r)[1]==err);}
int main(){
 Intent::Input invalid;uint16_t invalidTokens[16];for(int i=0;i<16;i++)invalidTokens[i]=Proto::PAD;float invalidScores[10];Intent::decodeTokens(invalidTokens,invalid);Intent::logits(invalid,invalidScores);for(float x:invalidScores)assert(!isfinite(x));invalidTokens[0]=247;Intent::decodeTokens(invalidTokens,invalid);Intent::logits(invalid,invalidScores);for(float x:invalidScores)assert(!isfinite(x));
 Slm::Worker w;Proto::Frame r;w.meta(r);assert(Proto::fullSlmCompatible(r));
 uint8_t p[32];for(int i=0;i<16;i++)Proto::put16(p+2*i,Proto::PAD);Proto::put16(p,78);
 auto a=request(Proto::LOAD_INPUT,42,0,p,24),b=request(Proto::LOAD_INPUT,42,1,p+24,8);
 expect(w,b,Proto::ERROR_REPLY,Proto::BAD_JOB);expect(w,request(Proto::START,42),Proto::ERROR_REPLY,Proto::BAD_JOB);
 expect(w,a,Proto::ACK);expect(w,a,Proto::ACK);Proto::Frame changed=a;Proto::payload(changed)[0]++;changed.bytes[31]=Proto::crc8(changed.bytes,31);expect(w,changed,Proto::ERROR_REPLY,Proto::BAD_JOB);
 expect(w,request(Proto::START,42),Proto::ERROR_REPLY,Proto::BAD_JOB);
 expect(w,request(Proto::LOAD_INPUT,43,1,p+24,8),Proto::ERROR_REPLY,Proto::BAD_JOB);
 auto corrupt=b;corrupt.bytes[8]^=1;expect(w,corrupt,Proto::ERROR_REPLY,Proto::BAD_FRAME);assert(w.mask==1);
 expect(w,b,Proto::ACK);expect(w,b,Proto::ACK);expect(w,request(Proto::START,42),Proto::ACK);assert(w.pending&&w.state==Proto::COMPUTING);
 expect(w,request(Proto::START,42),Proto::ACK);assert(w.computeIfPending());assert(!w.computeIfPending());
 expect(w,request(Proto::START,42),Proto::ACK);assert(!w.computeIfPending());
 Node::Snapshot& snap=w.snapshot; snap.hello(123);
 Proto::SharedState state={123,10,2,1,1,1,0,0,0,true,0};uint8_t sync[22];Proto::encodeState(sync,state);
 expect(w,request(Proto::STATE_SYNC,8,0,sync,22),Proto::ACK);assert(snap.shared.desiredDrive==1);
 state.revision=11;state.desiredDrive=state.appliedDrive=0;state.stopSources=Proto::ADAS_OBSTACLE;Proto::encodeState(sync,state);
 expect(w,request(Proto::STATE_SYNC,9,0,sync,22),Proto::ACK);assert(w.state==Proto::RESULT_READY&&w.job==42&&w.mask==3&&!w.pending);assert(snap.shared.desiredDrive==0&&snap.shared.appliedDrive==0);
 expect(w,request(Proto::STATE_SYNC,9,0,sync,22),Proto::ACK);
 state.revision=10;Proto::encodeState(sync,state);expect(w,request(Proto::STATE_SYNC,9,0,sync,22),Proto::ERROR_REPLY,Proto::BAD_JOB);
 state.session=122;Proto::encodeState(sync,state);expect(w,request(Proto::STATE_SYNC,9,0,sync,22),Proto::ERROR_REPLY,Proto::BAD_SESSION);
 expect(w,request(Proto::GET_RESULT,41),Proto::ERROR_REPLY,Proto::NOT_READY);
 uint8_t bytes[40];for(int seq=0;seq<2;seq++){w.handle(request(Proto::GET_RESULT,42,seq),r);assert(Proto::op(r)==Proto::RESULT);assert(Proto::len(r)==(seq?16:24));memcpy(bytes+seq*24,Proto::payload(r),Proto::len(r));}for(int i=0;i<10;i++)assert(Proto::getFloat(bytes+4*i)==w.scores[i]);
 expect(w,request(Proto::HELLO,100,0,p,4),Proto::HELLO_ACK);assert(w.mask==0&&!w.pending);
 for(int i=0;i<16;i++)Proto::put16(p+2*i,Proto::PAD);
 expect(w,request(Proto::LOAD_INPUT,55,0,p,24),Proto::ACK);expect(w,request(Proto::LOAD_INPUT,55,1,p+24,8),Proto::ACK);expect(w,request(Proto::START,55),Proto::ERROR_REPLY,Proto::BAD_TOKEN);
 Proto::put16(p,247);expect(w,request(Proto::LOAD_INPUT,56,0,p,24),Proto::ERROR_REPLY,Proto::BAD_TOKEN);assert(w.job==55);
 Compute::TransmitCursor tx;assert(tx.begin(a)==0xa5);for(int i=1;i<32;i++)assert(tx.next(a)==a.bytes[i]);assert(tx.next(a)==0xff);assert(tx.begin(a)==0xa5);assert(tx.next(a)==a.bytes[1]);
 Compute::Receiver rx;
 rx.begin();for(int i=0;i<31;i++)rx.byte(a.bytes[i]);assert(!rx.finish(false));
 rx.begin();for(int i=0;i<33;i++)rx.byte(a.bytes[i%32]);assert(!rx.finish(false));
 rx.begin();for(int i=0;i<32;i++)rx.byte(0xff);assert(!rx.finish(true));assert(!rx.queue.pop(r));
 for(int j=0;j<3;j++){rx.begin();for(int i=0;i<32;i++)rx.byte(a.bytes[i]);assert(rx.finish(true)==(j<2));}assert(rx.queue.overflow);for(int j=0;j<2;j++){assert(rx.queue.pop(r));assert(!memcmp(r.bytes,a.bytes,32));}assert(!rx.queue.pop(r));
 const char* samples[]={"forward","reverse","stop","turn lights on","turn lights off","start wipers","stop wipers","honk horn","silence horn","please move forward","move forward","go forward","go backward","move reverse","please stop","halt","lights on","lights off","wipers on","wipers off","horn on","horn off","please reverse","please turn lights on","please turn lights off","please start wipers","please stop wipers","please honk horn","please silence horn","turn on the lights","turn off the lights","please move backward","please halt"};
 for(const char* sample:samples){Intent::Input in;Intent::parse(sample,strlen(sample),in);assert(in.reason==Intent::OK);for(int i=0;i<16;i++)Proto::put16(p+2*i,in.tokens[i]);expect(w,request(Proto::HELLO,0,0,p,4),Proto::HELLO_ACK);expect(w,request(Proto::LOAD_INPUT,77,0,p,24),Proto::ACK);expect(w,request(Proto::LOAD_INPUT,77,1,p+24,8),Proto::ACK);expect(w,request(Proto::START,77),Proto::ACK);assert(w.computeIfPending());for(int i=0;i<10;i++)printf("%.9g ",w.scores[i]);puts("");}
}
