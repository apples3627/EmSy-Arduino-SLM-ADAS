#include <cassert>
#include <cstdio>
#include "../../common/protocol.h"
using namespace Proto;
int main(){
 Frame f;fullSlmMeta(f,3);assert(fullSlmCompatible(f));
 computeMeta(f,3,0,32);assert(!fullSlmCompatible(f));
 fullSlmMeta(f,3);payload(f)[16]=0;f.bytes[31]=crc8(f.bytes,31);assert(!fullSlmCompatible(f));
 fullSlmMeta(f,3);payload(f)[8]=2;f.bytes[31]=crc8(f.bytes,31);assert(!fullSlmCompatible(f));
 fullSlmMeta(f,3);f.bytes[1]=2;f.bytes[31]=crc8(f.bytes,31);assert(!fullSlmCompatible(f));
 SharedState in={0x12345678,0x76543210,0xfedcba98,65535,-1,1,7,4,ADAS_SENSOR_FAULT|SLM_STOP,false,12},out={};
 uint8_t p[SHARED_STATE_SIZE];encodeState(p,in);make(f,STATE_SYNC,4,0,p,sizeof p);assert(decodeState(f,out));
 assert(out.session==in.session&&out.revision==in.revision&&out.safetyEpoch==in.safetyEpoch&&out.command==in.command&&out.desiredDrive==-1&&out.appliedDrive==1&&out.stopSources==5&&!out.appliedKnown&&out.actorFlags==12);
 for(int k=0;k<5;k++){encodeState(p,in);const uint8_t pos[]={14,15,16,17,20};p[pos[k]]=k<2?2:k<4?8:2;make(f,STATE_SYNC,4,0,p,sizeof p);assert(!decodeState(f,out));}
 make(f,STATE_SYNC,4,0,p,sizeof p-1);assert(!decodeState(f,out));
 assert(newer16(1,65535)&&!newer16(65535,1));
 puts("PASS protocol: full H64 bias metadata, incompatible metadata rejection, shared state validation");
}
