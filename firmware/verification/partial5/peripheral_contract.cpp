#include PERIPHERAL_ENGINE_HEADER
#include "frame_helpers.h"
static bool aux(const Peripheral::Engine& e){return e.outputs().lights||e.outputs().horn;}
static Proto::Frame send(Peripheral::Engine& e,const Proto::Frame& q,uint32_t at=0){Proto::Frame r;CHECK(e.handle(q,r,at));CHECK(Proto::valid(r));return r;}
int main(){
 Peripheral::Engine e;send(e,hello());e.adas(true,true);send(e,stop(2,1));CHECK(e.inhibited()&&!aux(e));CHECK(Proto::op(send(e,batch(3,1,{3,8})))==Proto::ERROR_REPLY);CHECK(!aux(e));
 send(e,policy(4,2,1));CHECK(e.inhibited()&&!aux(e));CHECK(Proto::op(send(e,batch(5,1,{3,8})))==Proto::ERROR_REPLY);send(e,stop(6,2));CHECK(!e.inhibited());send(e,batch(7,2,{3,8}));CHECK(e.outputs().lights&&e.outputs().horn);e.adas(true,true);CHECK(aux(e)&&!e.inhibited());
 // A mode transition consumes no cached action. The same-epoch STOP cannot undo a fresh transition.
 CHECK(Proto::op(send(e,policy(8,2,0)))==Proto::ERROR_REPLY&&aux(e));send(e,policy(9,3,0));CHECK(!aux(e)&&e.inhibited());send(e,stop(10,3));CHECK(e.inhibited());e.adas(false,false);send(e,stop(11,3));CHECK(e.inhibited());send(e,stop(12,4));CHECK(!e.inhibited()&&!aux(e));
 send(e,batch(13,4,{3,8}));CHECK(aux(e));e.adas(false,true);CHECK(!aux(e)&&e.inhibited());send(e,policy(14,5,1));send(e,stop(15,5));CHECK(!aux(e)&&!e.inhibited());
 CHECK(Proto::op(send(e,policy(16,4,0)))==Proto::ERROR_REPLY&&e.bypass());CHECK(Proto::op(send(e,policy(17,6,0,101)))==Proto::ERROR_REPLY&&e.bypass());CHECK(Proto::op(send(e,batch(18,4,{3,8})))==Proto::ERROR_REPLY&&!aux(e));
 // Observation frames cannot enable outputs or clear the STOP barrier.
 send(e,telemetry(19,5,65535,250,0,3));CHECK(e.hasSensor()&&!aux(e));CHECK(Proto::op(send(e,telemetry(20,5,0,199,0,19)))==Proto::ACK);CHECK(e.sensor().sampleId==0);CHECK(Proto::op(send(e,telemetry(21,5,65535,250,0,3)))==Proto::ERROR_REPLY&&e.sensor().rangeMm==199);CHECK(Proto::op(send(e,telemetry(22,5,1,250,0,3,1,101)))==Proto::ERROR_REPLY);CHECK(Proto::op(send(e,telemetry(23,4,1,250,0,3)))==Proto::ERROR_REPLY);CHECK(Proto::op(send(e,telemetry(24,5,1,19,0,3)))==Proto::ERROR_REPLY);CHECK(Proto::op(send(e,telemetry(25,5,1,4001,0,3)))==Proto::ERROR_REPLY);CHECK(Proto::op(send(e,telemetry(26,5,1,250,201,3)))==Proto::ERROR_REPLY);CHECK(!aux(e));
 send(e,telemetry(27,5,1,250,100,3),100);CHECK(e.sensorStale(201));auto replay=send(e,telemetry(28,5,1,250,0,3),201);CHECK((Proto::op(replay)==Proto::ACK||Proto::op(replay)==Proto::ERROR_REPLY)&&e.sensorStale(201));
 send(e,batch(29,5,{3,8}),202);CHECK(aux(e));e.tick(2202);CHECK(!aux(e)&&e.inhibited());send(e,stop(30,5),2202);CHECK(e.inhibited());send(e,stop(31,6),2203);CHECK(!e.inhibited()&&!aux(e));
 puts("PASS peripheral policy/STOP boot barrier, physical-LOW bypass, transitions, telemetry SID/epoch/sample wrap/age replay and watchdog");
}
