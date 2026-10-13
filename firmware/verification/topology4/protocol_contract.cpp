#include <cassert>
#include <cstdio>
#include PROTOCOL_HEADER
int main(){
  Proto::Frame f; const uint32_t sid=1234;
  Proto::slmMeta(f,sid,11); assert(Proto::slmCompatible(f,sid)); assert(!Proto::slmCompatible(f,sid+1));
  assert(Proto::len(f)==21&&Proto::get32(Proto::payload(f))==0x95616d56UL);
  Proto::peripheralMeta(f,sid,12); assert(Proto::peripheralCompatible(f,sid)); assert(!Proto::peripheralCompatible(f,sid+1));
  assert(Proto::len(f)==10&&Proto::payload(f)[9]==7);
  assert(!Proto::supportedIntent(Proto::WIPER_ON)&&!Proto::supportedIntent(Proto::WIPER_OFF));
  Proto::SharedState s={sid,3,9,5,-1,-1,5,5,0,true,16},v;
  uint8_t p[22];Proto::encodeState(p,s);Proto::make(f,Proto::STATE_SYNC,13,0,p,22);
  assert(Proto::decodeState(f,v)&&v.session==sid&&v.safetyEpoch==9&&v.appliedDrive==-1&&v.actorFlags==16);
  Proto::StreamParser parser;Proto::Frame out; for(int repeat=0;repeat<32;repeat++){
    assert(!parser.push(0xff,out)); for(int i=0;i<32;i++) assert(parser.push(f.bytes[i],out)==(i==31));
    assert(Proto::valid(out)&&Proto::id(out)==13);
  }
  puts("PASS protocol4 metadata, observed state and repeated framed-link parsing");
}
