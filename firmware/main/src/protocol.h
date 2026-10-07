#pragma once
#include <stdint.h>
#include <string.h>
#ifdef __AVR__
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Use Arduino Uno R3 / ATmega328P at 16 MHz"
#endif
#endif
namespace Proto {
static const uint8_t FRAME_SIZE=32, PAYLOAD_SIZE=24, VERSION=1;
static const uint32_t MODEL_ID=0x30918170UL, VOCAB_TAG=0x766b5682UL;
static const uint16_t VOCAB_SIZE=247, PAD=0xffff;
static const uint8_t LABELS_VERSION=2, LABEL_COUNT=10, EMBED_SIZE=16, HIDDEN_TOTAL=64;
enum Op : uint8_t { HELLO=1, HELLO_ACK=2, LOAD_INPUT=3, START=4, ACK=5, STATUS=6, STATUS_REPLY=7, GET_RESULT=8, RESULT=9, ERROR_REPLY=10, ACTUATE=11, STOP_ALL=12, HEARTBEAT=13 };
enum Error : uint8_t { OK=0, BAD_FRAME=1, BAD_LENGTH=2, BAD_JOB=3, NOT_READY=4, BAD_TOKEN=5, BAD_MODEL=6, BAD_SESSION=7, OLD_COMMAND=8, BUSY=9, RX_OVERFLOW=10, LOCAL_STOP=11, BAD_OP=12 };
enum WorkerState : uint8_t { IDLE=0, RECEIVING=1, COMPUTING=2, RESULT_READY=3, FAULT=4 };
enum IntentId : uint8_t { FORWARD=0, REVERSE=1, ALL_STOP=2, LIGHT_ON=3, LIGHT_OFF=4, WIPER_ON=5, WIPER_OFF=6, UNKNOWN_INTENT=7, HORN_ON=8, HORN_OFF=9 };
struct Frame { uint8_t bytes[FRAME_SIZE]; };
inline uint8_t crc8(const uint8_t* p,uint8_t n){uint8_t c=0;while(n--){c^=*p++;for(uint8_t b=0;b<8;b++)c=(c&0x80)?uint8_t((c<<1)^0x07):uint8_t(c<<1);}return c;}
inline void put16(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
inline uint16_t get16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline void put32(uint8_t* p,uint32_t v){for(uint8_t i=0;i<4;i++)p[i]=uint8_t(v>>(8*i));}
inline uint32_t get32(const uint8_t* p){uint32_t v=0;for(uint8_t i=0;i<4;i++)v|=uint32_t(p[i])<<(8*i);return v;}
inline void putFloat(uint8_t* p,float f){static_assert(sizeof(float)==4,"float32 required");uint32_t v;memcpy(&v,&f,4);put32(p,v);}
inline float getFloat(const uint8_t* p){uint32_t v=get32(p);float f;memcpy(&f,&v,4);return f;}
inline uint8_t op(const Frame& f){return f.bytes[2];}
inline uint16_t id(const Frame& f){return get16(f.bytes+3);}
inline uint8_t seq(const Frame& f){return f.bytes[5];}
inline uint8_t len(const Frame& f){return f.bytes[6];}
inline uint8_t* payload(Frame& f){return f.bytes+7;}
inline const uint8_t* payload(const Frame& f){return f.bytes+7;}
inline bool valid(const Frame& f){if(f.bytes[0]!=0xa5||f.bytes[1]!=VERSION||len(f)>PAYLOAD_SIZE||crc8(f.bytes,31)!=f.bytes[31])return false;for(uint8_t i=len(f);i<PAYLOAD_SIZE;i++)if(payload(f)[i])return false;return true;}
inline bool make(Frame& f,uint8_t operation,uint16_t job,uint8_t part=0,const uint8_t* data=0,uint8_t size=0){if(size>PAYLOAD_SIZE||(size&&!data))return false;memset(f.bytes,0,FRAME_SIZE);f.bytes[0]=0xa5;f.bytes[1]=VERSION;f.bytes[2]=operation;put16(f.bytes+3,job);f.bytes[5]=part;f.bytes[6]=size;if(size)memcpy(payload(f),data,size);f.bytes[31]=crc8(f.bytes,31);return true;}
inline void ack(Frame& f,const Frame& req,uint8_t err=OK){uint8_t p[2]={op(req),err};make(f,err?ERROR_REPLY:ACK,id(req),seq(req),p,2);}
// Compute HELLO_ACK payload: model4,vocabTag4,labelsVersion,labels,embed,Htotal,lo,hi,vocab16.
inline void computeMeta(Frame& f,uint16_t job,uint8_t lo,uint8_t hi){uint8_t p[16];put32(p,MODEL_ID);put32(p+4,VOCAB_TAG);p[8]=LABELS_VERSION;p[9]=LABEL_COUNT;p[10]=EMBED_SIZE;p[11]=HIDDEN_TOTAL;p[12]=lo;p[13]=hi;put16(p+14,VOCAB_SIZE);make(f,HELLO_ACK,job,0,p,16);}
inline bool compatible(const Frame& f,uint8_t lo,uint8_t hi){const uint8_t* p=payload(f);return valid(f)&&op(f)==HELLO_ACK&&len(f)==16&&get32(p)==MODEL_ID&&get32(p+4)==VOCAB_TAG&&p[8]==LABELS_VERSION&&p[9]==LABEL_COUNT&&p[10]==EMBED_SIZE&&p[11]==HIDDEN_TOTAL&&p[12]==lo&&p[13]==hi&&get16(p+14)==VOCAB_SIZE;}
// Compute STATUS_REPLY len6: state,error,inputMask,workerRole,activeJob16.
// Controller HELLO request len9: session32,model32,labelsVersion. ACK len10: session32,model32,labelsVersion,capabilities0x0f.
// Controller requests ACTUATE len7: session32,intent8,duration16; STOP_ALL/STATUS/HEARTBEAT len4: session32.
// Controller ACK/STATUS_REPLY len12: session32,drive(int8),lights,wiper,horn,servoAngle,flags,lastAppliedCommand16.
// flags: bit0 reversing pending, bit1 local stop latched. ACK frame id is the acknowledged command; STATUS id echoes query.
static const uint8_t CONTROLLER_STATUS_SIZE=12;
inline bool newer16(uint16_t a,uint16_t b){return int16_t(a-b)>0;}
struct StreamParser {Frame frame;uint8_t used;StreamParser():used(0){} void reset(){used=0;} bool push(uint8_t b,Frame& out){if(!used&&b!=0xa5)return false;frame.bytes[used++]=b;if(used<FRAME_SIZE)return false;if(valid(frame)){out=frame;used=0;return true;}uint8_t next=1;while(next<FRAME_SIZE&&frame.bytes[next]!=0xa5)next++;used=uint8_t(FRAME_SIZE-next);if(used)memmove(frame.bytes,frame.bytes+next,used);return false;} };
}
