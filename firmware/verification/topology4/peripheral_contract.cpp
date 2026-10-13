#include <cassert>
#include <cstdio>
#include PERIPHERAL_ENGINE_HEADER
#include "frame_helpers.h"
int main(){
 Peripheral::Engine e;Proto::Frame r;auto q=hello();assert(e.handle(q,r,0)&&Proto::peripheralCompatible(r,100));
 q=stop(2,1);e.handle(q,r,1);assert(!e.inhibited());
 q=batch(3,1,{Proto::FORWARD,Proto::LIGHT_ON,Proto::HORN_ON});e.handle(q,r,2);assert(Proto::op(r)==Proto::ACK&&e.outputs().lights&&e.outputs().horn);
 assert(!Proto::payload(r)[8]&&!Proto::payload(r)[10]);
 auto old=q;q=batch(4,1,{Proto::LIGHT_OFF,Proto::WIPER_ON});e.handle(q,r,3);assert(Proto::op(r)==Proto::ERROR_REPLY&&!e.outputs().lights&&!e.outputs().horn);
 q=batch(5,1,{Proto::WIPER_ON,Proto::FORWARD,Proto::REVERSE,Proto::ALL_STOP});e.handle(q,r,4);assert(Proto::op(r)==Proto::ACK&&!e.outputs().lights&&!e.outputs().horn);
 q=batch(6,1,{Proto::LIGHT_ON,Proto::HORN_ON});e.handle(q,r,5);assert(e.outputs().lights&&e.outputs().horn);
 Proto::SharedState s={100,1,1,6,0,0,0,0,0,true,0};q=sync(s);e.handle(q,r,6);assert(e.hasSnapshot()&&e.snapshot().appliedDrive==0&&e.outputs().lights&&e.outputs().horn);
 s.revision=0;s.appliedDrive=1;q=sync(s);e.handle(q,r,7);assert(e.snapshot().revision==1&&e.snapshot().appliedDrive==0);
 e.adas(true,true);assert(e.inhibited()&&!e.outputs().lights&&!e.outputs().horn);e.adas(false);assert(e.inhibited());
 q=stop(7,1);e.handle(q,r,8);assert(e.inhibited());q=stop(8,2);e.handle(q,r,9);assert(!e.inhibited());
 e.handle(old,r,10);assert(Proto::op(r)==Proto::ERROR_REPLY&&!e.outputs().lights&&!e.outputs().horn);
 q=batch(9,2,{Proto::LIGHT_ON,Proto::HORN_ON});e.handle(q,r,11);assert(e.outputs().lights&&e.outputs().horn);e.lcdFault(true);e.status(r,Proto::STATUS_REPLY,12);assert((Proto::payload(r)[13]&16)&&!e.inhibited());
 e.tick(2011);assert(e.inhibited()&&!e.outputs().lights&&!e.outputs().horn);
 puts("PASS actual peripheral engine: whole-plan validation, STOP dominance, snapshots, epochs, sticky safety and LCD diagnostic");
}
