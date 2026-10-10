#pragma once
#include "../../protocol.h"
namespace Node {
struct Snapshot {
 uint32_t session; bool greeted,known; Proto::SharedState shared;
 Snapshot():session(0),greeted(false),known(false),shared(){}
 void hello(uint32_t value){session=value;greeted=true;known=false;memset(&shared,0,sizeof(shared));shared.session=value;}
 uint8_t accept(const Proto::Frame& q){
  Proto::SharedState next;
  if(Proto::len(q)!=Proto::SHARED_STATE_SIZE)return Proto::BAD_LENGTH;
  if(!Proto::decodeState(q,next))return Proto::BAD_FRAME;
  if(!greeted||next.session!=session)return Proto::BAD_SESSION;
  if(known&&int32_t(next.revision-shared.revision)<0)return Proto::BAD_JOB;
  if(known&&next.revision==shared.revision){uint8_t old[22];Proto::encodeState(old,shared);return memcmp(old,Proto::payload(q),22)?Proto::BAD_JOB:Proto::OK;}
  shared=next;known=true;return Proto::OK;
 }
};
}
