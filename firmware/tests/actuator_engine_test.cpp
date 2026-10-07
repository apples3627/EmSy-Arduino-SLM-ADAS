#include <cassert>
#include "../actuator_uart/src/control_engine.h"
using namespace Actuator;
static Proto::Frame req(uint8_t op,uint16_t id,uint32_t session,uint8_t intent=0,uint16_t duration=0){Proto::Frame f;uint8_t p[9]={};Proto::put32(p,session);uint8_t n=4;if(op==Proto::HELLO){Proto::put32(p+4,Proto::MODEL_ID);p[8]=2;n=9;}if(op==Proto::ACTUATE){p[4]=intent;Proto::put16(p+5,duration);n=7;}Proto::make(f,op,id,0,p,n);return f;}
int main(){Engine e;Proto::Frame out;auto call=[&](uint8_t op,uint16_t id,uint32_t sess,uint8_t intent,uint32_t t,uint16_t duration=0){return e.onFrame(req(op,id,sess,intent,duration),out,t);};
 assert(!e.outputs().drive&&!e.outputs().wiper);assert(call(Proto::HELLO,0,1,0,0));assert(e.takePersist());
 assert(call(Proto::ACTUATE,1,1,0,1));assert(e.outputs().drive==1&&e.outputs().duty==75);
 call(Proto::ACTUATE,2,1,3,2);call(Proto::ACTUATE,3,1,5,3);call(Proto::ACTUATE,4,1,8,4);assert(e.outputs().lights&&e.outputs().wiper&&e.outputs().horn);
 e.tick(30);assert(e.outputs().servoAngle==48);call(Proto::HEARTBEAT,40,1,0,400);e.tick(501);assert(!e.outputs().drive&&e.outputs().lights&&e.outputs().wiper&&e.outputs().horn);
 call(Proto::ACTUATE,4,1,8,600);assert(Proto::op(out)==Proto::ACK);call(Proto::ACTUATE,5,1,4,601);call(Proto::ACTUATE,6,1,6,602);call(Proto::ACTUATE,7,1,9,603);assert(!e.outputs().lights&&!e.outputs().wiper&&!e.outputs().horn);
 call(Proto::ACTUATE,8,1,0,610);assert(!call(Proto::ACTUATE,9,1,1,620));assert(!e.outputs().drive);e.tick(719);assert(!e.takeReply(out));e.tick(720);assert(e.outputs().drive==-1&&e.takeReply(out)&&Proto::id(out)==9);call(Proto::ACTUATE,9,1,1,1000);e.tick(1220);assert(!e.outputs().drive);
 call(Proto::ACTUATE,10,1,0,1230);call(Proto::ACTUATE,11,1,1,1240);call(Proto::STOP_ALL,12,1,0,1250);e.tick(1400);assert(!e.outputs().drive&&!e.takeReply(out));call(Proto::ACTUATE,11,1,1,1401);assert(Proto::payload(out)[1]==Proto::OLD_COMMAND);
 call(Proto::ACTUATE,13,1,3,1410);call(Proto::ACTUATE,14,1,7,1420);assert(!e.outputs().lights);call(Proto::ACTUATE,15,1,3,1430);call(Proto::ACTUATE,16,1,99,1440);assert(!e.outputs().lights&&Proto::op(out)==Proto::ERROR_REPLY);
 call(Proto::ACTUATE,17,1,0,1450);auto bad=req(Proto::ACTUATE,18,1,1);bad.bytes[31]^=1;e.onFrame(bad,out,1451);assert(!e.outputs().drive&&!e.currentSession());call(Proto::HELLO,0,1,0,1452);assert(Proto::payload(out)[1]==Proto::BAD_SESSION);call(Proto::HELLO,0,2,0,1453);
 e.localButton(true);call(Proto::HELLO,0,3,0,1454);assert(Proto::payload(out)[1]==Proto::LOCAL_STOP);e.localButton(false);call(Proto::HELLO,0,2,0,1455);assert(Proto::payload(out)[1]==Proto::BAD_SESSION);call(Proto::HELLO,0,3,0,1456);call(Proto::ACTUATE,1,3,3,1460);e.tick(3460);assert(!e.currentSession()&&!e.outputs().lights);
 Engine reboot;reboot.init(3);assert(reboot.onFrame(req(Proto::HELLO,0,3),out,0)&&Proto::payload(out)[1]==Proto::BAD_SESSION);
 call(Proto::HELLO,0,4,0,3500);assert(e.takePersist());call(Proto::ACTUATE,1,4,0,3501,65535);e.tick(4501);assert(!e.outputs().drive);
 auto length=req(Proto::ACTUATE,2,4,0);Proto::make(length,Proto::ACTUATE,2,0,Proto::payload(length),0);e.onFrame(length,out,4502);assert(Proto::payload(out)[1]==Proto::BAD_LENGTH);call(99,3,4,0,4503);assert(Proto::payload(out)[1]==Proto::BAD_OP);
 call(Proto::ACTUATE,2,4,3,4510);call(Proto::ACTUATE,3,99,0,4511);assert(e.outputs().lights&&Proto::payload(out)[1]==Proto::BAD_SESSION);
 call(Proto::HELLO,0,4,0,4512);assert(e.outputs().lights&&!e.takePersist());
 auto part=req(Proto::ACTUATE,3,4,0);part.bytes[5]=1;part.bytes[31]=Proto::crc8(part.bytes,31);e.onFrame(part,out,4513);assert(!e.outputs().lights&&Proto::payload(out)[1]==Proto::BAD_LENGTH);
 call(Proto::ACTUATE,3,4,0,4514);call(Proto::ACTUATE,3,4,0,4800);e.tick(5014);assert(!e.outputs().drive);call(Proto::ACTUATE,3,4,0,5015);assert(!e.outputs().drive);
 call(Proto::ACTUATE,4,4,3,5020);auto pad=req(Proto::ACTUATE,5,4,0);pad.bytes[30]=1;pad.bytes[31]=Proto::crc8(pad.bytes,31);e.onFrame(pad,out,5021);assert(!e.outputs().lights&&!e.currentSession());
 Proto::StreamParser parser;auto good=req(Proto::STATUS,5,4);bool got=false;for(auto b:good.bytes)got=parser.push(b,out);assert(got&&Proto::valid(out));good.bytes[31]^=1;got=false;for(auto b:good.bytes)got|=parser.push(b,out);assert(!got);
}
