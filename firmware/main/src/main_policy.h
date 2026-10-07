#pragma once
#include "protocol.h"
#include "model/intent_model.h"
namespace MainPolicy {
inline bool exactStop(const char* s,uint8_t n){uint8_t a=0;while(a<n&&s[a]==' ')++a;while(n>a&&s[n-1]==' ')--n;while(n>a&&(s[n-1]=='.'||s[n-1]=='!'||s[n-1]=='?'))--n;while(n>a&&s[n-1]==' ')--n;if(n-a!=4)return false;const char* p="stop";for(uint8_t i=0;i<4;i++){char c=s[a+i];if(c>='A'&&c<='Z')c+=32;if(c!=p[i])return false;}return true;}
inline bool appliedAck(const Proto::Frame& f,uint32_t session,uint16_t command){const uint8_t* p=Proto::payload(f);return Proto::valid(f)&&Proto::op(f)==Proto::ACK&&Proto::id(f)==command&&Proto::len(f)==12&&Proto::get32(p)==session&&Proto::get16(p+10)==command&&!(p[9]&1);}
struct Scores {
 uint16_t job; uint32_t generation; uint8_t masks[2], complete; float z[10];
 void reset(uint16_t id,uint32_t gen){job=id;generation=gen;masks[0]=masks[1]=complete=0;for(uint8_t i=0;i<10;i++)z[i]=rdf(IntentMain::C+i);}
 bool accept(uint8_t worker,const Proto::Frame& f,uint32_t current){if(worker>1||generation!=current||!Proto::valid(f)||Proto::op(f)!=Proto::RESULT||Proto::id(f)!=job||Proto::seq(f)>1)return false;uint8_t part=Proto::seq(f), size=part?16:24;if(Proto::len(f)!=size||(masks[worker]&(1<<part)))return false;for(uint8_t i=0;i<size/4;i++)if(!isfinite(Proto::getFloat(Proto::payload(f)+i*4)))return false;for(uint8_t i=0;i<size/4;i++)z[(part?6:0)+i]+=Proto::getFloat(Proto::payload(f)+i*4);masks[worker]|=1<<part;if(masks[worker]==3)complete|=1<<worker;return true;}
};
}
