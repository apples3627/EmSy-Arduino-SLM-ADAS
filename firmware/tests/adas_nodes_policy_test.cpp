#include <cassert>
#include <cstdio>
#include "../adas_i2c/src/ownengine/adapters/worker.h"
#include "../adas_i2c/src/ownengine/adapters/transport.h"
int main(){
 Adas::Worker w;auto& p=w.policy;assert(p.flags()==(Adas::STARTUP|Adas::ALERT));
 p.sample(2900,1);p.sample(2900,71);assert(p.sources.asserted());p.sample(2900,141);assert(!p.sources.asserted());
 p.sample(2320,211);assert(p.flags()&Adas::OBSTACLE);p.sample(2784,281);assert(p.clearCount==0&&p.sources.asserted());
 p.sample(2900,351);p.sample(2900,421);assert(p.sources.asserted());p.sample(2900,491);assert(!p.sources.asserted());
 p.sample(231,500);assert(p.flags()&Adas::SENSOR_FAULT);assert(p.rangeMm==0xffff);
 p.sample(46401,570);assert(p.flags()&Adas::SENSOR_FAULT);p.sample(2900,640);p.sample(2900,710);p.sample(2900,780);assert(!p.sources.asserted());
 p.tick(980);assert(!p.sources.asserted());p.tick(981);assert(p.flags()&Adas::SENSOR_FAULT);
 Adas::Policy wrap;wrap.bootTime=0xffffffc0UL;wrap.sample(2900,0xffffffc0UL);wrap.sample(2900,6);wrap.sample(2900,76);wrap.tick(276);assert(!wrap.sources.asserted());wrap.tick(277);assert(wrap.sources.asserted());
 Adas::Policy missing;missing.tick(201);assert(missing.flags()&Adas::SENSOR_FAULT);
 Adas::SourceRegistry sources;assert(sources.registerSource(0));assert(sources.registerSource(1));sources.update(0,Adas::OBSTACLE);sources.update(1,Adas::SENSOR_FAULT);sources.update(0,0);assert(sources.asserted()&&sources.flags()==Adas::SENSOR_FAULT);sources.update(1,0);assert(!sources.asserted());assert(!sources.update(2,0));sources.registerSource(1,false);
 Proto::Frame q,r;uint8_t session[4];Proto::put32(session,12);Proto::make(q,Proto::HELLO,9,1,session,4);w.handle(q,r,981);assert(Proto::valid(r)&&Proto::op(r)==Proto::ADAS_META&&Proto::len(r)==8&&Proto::seq(r)==1);assert(p.sources.asserted());
 Proto::SharedState state={12,1,2,1,0,0,0,0,Proto::ADAS_SENSOR_FAULT,true,4};uint8_t data[22];Proto::encodeState(data,state);Proto::make(q,Proto::STATE_SYNC,10,0,data,22);w.handle(q,r,981);assert(Proto::op(r)==Proto::ACK);assert(p.sources.asserted());
 state.session=11;Proto::encodeState(data,state);Proto::make(q,Proto::STATE_SYNC,11,0,data,22);w.handle(q,r,981);assert(Proto::payload(r)[1]==Proto::BAD_SESSION);
 state.session=12;state.revision=0;Proto::encodeState(data,state);Proto::make(q,Proto::STATE_SYNC,12,0,data,22);w.handle(q,r,981);assert(Proto::payload(r)[1]==Proto::BAD_JOB);
 Proto::make(q,Proto::STATUS,19);w.handle(q,r,981);assert(Proto::valid(r)&&Proto::len(r)==18&&Proto::get32(Proto::payload(r)+14)==1&&Proto::id(r)==19);uint8_t flags=p.flags();q.bytes[8]^=1;w.handle(q,r,981);assert(Proto::payload(r)[1]==Proto::BAD_FRAME&&p.flags()==flags);
 Compute::Receiver rx;Proto::make(q,Proto::STATUS,1);for(int n=31;n<=33;n++){rx.begin();for(int i=0;i<n;i++)rx.byte(q.bytes[i%32]);assert(rx.finish(false)==(n==32));}assert(rx.queue.pop(r));assert(!memcmp(q.bytes,r.bytes,32));Compute::TransmitCursor tx;assert(tx.begin(r)==r.bytes[0]);for(int i=1;i<32;i++)assert(tx.next(r)==r.bytes[i]);assert(tx.next(r)==255);
 puts("PASS ADAS hysteresis, startup, faults, stale/wrap, source registry, state sync, TWI frame cache");
}
