#pragma once
#include <stdint.h>
#include <string.h>
#ifdef __AVR__
#if !defined(__AVR_ATmega328P__) || F_CPU != 16000000UL
#error "Use Arduino Uno R3 / ATmega328P at 16 MHz"
#endif
#endif
namespace Proto {
static const uint8_t FRAME_SIZE=32, PAYLOAD_SIZE=24, VERSION=4;
static const uint32_t MODEL_ID=0x95616d56UL, VOCAB_TAG=0xbfd74d7eUL;
static const uint16_t VOCAB_SIZE=245, PAD=0xffff;
static const uint8_t LABELS_VERSION=3, LABEL_COUNT=10, EMBED_SIZE=16, HIDDEN_TOTAL=64;
enum Op : uint8_t { HELLO=1, HELLO_ACK=2, LOAD_INPUT=3, START=4, ACK=5, STATUS=6, STATUS_REPLY=7, GET_RESULT=8, RESULT=9, ERROR_REPLY=10, ACTUATE=11, STOP_ALL=12, HEARTBEAT=13, ACTUATE_BATCH=14, ADAS_META=15, ADAS_STATUS_REPLY=16, ADAS_CONFIG=17, STATE_SYNC=18, SLM_META=19, PERIPHERAL_META=20, DIAGNOSTIC=21 };
enum Error : uint8_t { OK=0, BAD_FRAME=1, BAD_LENGTH=2, BAD_JOB=3, NOT_READY=4, BAD_TOKEN=5, BAD_MODEL=6, BAD_SESSION=7, OLD_COMMAND=8, BUSY=9, RX_OVERFLOW=10, LOCAL_STOP=11, BAD_OP=12, STALE_SAFETY=13, SAFETY_INHIBIT=14 };
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
// Controller protocol4 request/reply layouts are documented below.
// ACK frame id is the acknowledged command; STATUS id echoes its query.
static const uint8_t CONTROLLER_STATUS_SIZE=18;
inline bool newer16(uint16_t a,uint16_t b){return int16_t(a-b)>0;}
struct StreamParser {Frame frame;uint8_t used;StreamParser():used(0){} void reset(){used=0;} bool push(uint8_t b,Frame& out){if(!used&&b!=0xa5)return false;frame.bytes[used++]=b;if(used<FRAME_SIZE)return false;if(valid(frame)){out=frame;used=0;return true;}uint8_t next=1;while(next<FRAME_SIZE&&frame.bytes[next]!=0xa5)next++;used=uint8_t(FRAME_SIZE-next);if(used)memmove(frame.bytes,frame.bytes+next,used);return false;} };
}

// ACTUATE_BATCH uses protocol4 session and safety epoch; see layout below.
// Validate the full batch before output changes. Duplicate intents collapse, opposite intents conflict.
// ALL_STOP/UNKNOWN takes precedence and clears every output; no partial action on rejection.

namespace Proto {
enum StopSource : uint16_t { SLM_STOP=1, ADAS_OBSTACLE=2, ADAS_SENSOR_FAULT=4, COMM_FAULT=8, USER_ESTOP=16, BOOT_STOP=32, DRIVE_LEASE=64 };
static const uint8_t ADAS_ROLE=0xad, HC_SR04=1, SHARED_STATE_SIZE=22;
struct SharedState {uint32_t session,revision,safetyEpoch;uint16_t command;int8_t desiredDrive,appliedDrive;uint8_t desiredFlags,appliedFlags;uint16_t stopSources;bool appliedKnown;uint8_t actorFlags;};
inline void encodeState(uint8_t* p,const SharedState& s){put32(p,s.session);put32(p+4,s.revision);put32(p+8,s.safetyEpoch);put16(p+12,s.command);p[14]=uint8_t(s.desiredDrive);p[15]=uint8_t(s.appliedDrive);p[16]=s.desiredFlags;p[17]=s.appliedFlags;put16(p+18,s.stopSources);p[20]=s.appliedKnown?1:0;p[21]=s.actorFlags;}
inline bool decodeState(const Frame& f,SharedState& s){if(!valid(f)||op(f)!=STATE_SYNC||len(f)!=SHARED_STATE_SIZE)return false;const uint8_t* p=payload(f);s.session=get32(p);s.revision=get32(p+4);s.safetyEpoch=get32(p+8);s.command=get16(p+12);s.desiredDrive=int8_t(p[14]);s.appliedDrive=int8_t(p[15]);s.desiredFlags=p[16];s.appliedFlags=p[17];s.stopSources=get16(p+18);s.appliedKnown=p[20]!=0;s.actorFlags=p[21];return s.desiredDrive>=-1&&s.desiredDrive<=1&&s.appliedDrive>=-1&&s.appliedDrive<=1&&(s.desiredFlags&~7)==0&&(s.appliedFlags&~7)==0&&p[20]<=1;}
// SLM HELLO_ACK: existing metadata16 plus byte16=1 (full H64 scores INCLUDE global bias).
inline void fullSlmMeta(Frame& f,uint16_t job){computeMeta(f,job,0,64);f.bytes[6]=17;payload(f)[16]=1;f.bytes[31]=crc8(f.bytes,31);}
inline bool fullSlmCompatible(const Frame& f){if(!valid(f)||op(f)!=HELLO_ACK||len(f)!=17||payload(f)[16]!=1)return false;const uint8_t* p=payload(f);return get32(p)==MODEL_ID&&get32(p+4)==VOCAB_TAG&&p[8]==LABELS_VERSION&&p[9]==10&&p[10]==16&&p[11]==64&&p[12]==0&&p[13]==64&&get16(p+14)==VOCAB_SIZE;}
// ADAS_META len8: roleAD,sensorHC,stopCm,releaseCm,clearRequired,protocol4,periodMs16.
// ADAS_STATUS_REPLY len18: roleAD,flags,rangeMm16,sampleId16,ageMs16,stopCm,releaseCm,clearCount,clearRequired,periodMs16,sharedRevision32.
// ADAS flags: bit0 startup,bit1 obstacle,bit2 sensorFault,bit3 validSample,bit4 ALERT line asserted.
// Protocol4 ACTUATE len11: session32,safetyEpoch32,intent8,duration16.
// Protocol4 ACTUATE_BATCH len11+count: session32,safetyEpoch32,count8,intents[count],duration16.
// Protocol4 STOP_ALL len10: session32,newSafetyEpoch32,stopSources16.
// STATUS/HEARTBEAT still len4 session32. Controller ACK/STATUS_REPLY len18:
// session32,epoch32,drive8,lights8,wiper8,horn8,servo8,flags8,lastAppliedCmd16,stopSources16.
// flags bit0 reversalPending,bit1 localStopLatched,bit2 adasLineActive,bit3 safetySyncRequired.
}

// 2026-10-13 topology: PC -> SLM_UART -> MAIN; MAIN drives two L298N channels.
// MAIN HELLO to SLM/PERIPHERAL: len9 session32,model32,labelsVersion.
// SLM_META len21: full metadata16,full-bias marker1,session32 at offset17.
// PERIPHERAL_META len10: session32,model32,labelsVersion,capabilities(LED1|HORN2|LCD4).
// SLM -> MAIN ACTUATE_BATCH: len11+count, session32,observedEpoch32,count,intents,duration16.
// SLM -> MAIN STOP_ALL: len10 session32,observedEpoch32,source16. MAIN owns new epoch.
// MAIN -> SLM ACK/STATUS_REPLY len18 is authoritative applied state (wiper field ALWAYS0).
// MAIN -> SLM STATE_SYNC len22; actorFlags bit4 indicates command pending/busy.
// MAIN -> peripheral ACTUATE_BATCH uses same full plan; motor0/1 are display-only there.
// STATE_SYNC is observation only; it MUST NOT change peripheral output bits.
// MAIN grants motors only after matching peripheral command ACK; SPI timeout stops motors.
namespace Proto {
static const uint8_t PERIPHERAL_CAPABILITIES=7, BUSY_FLAG=16;
inline bool helloRequest(const Frame& f,uint32_t& session){if(!valid(f)||op(f)!=HELLO||seq(f)||len(f)!=9)return false;const uint8_t* p=payload(f);session=get32(p);return session&&session!=0xffffffffUL&&get32(p+4)==MODEL_ID&&p[8]==LABELS_VERSION;}
inline void slmMeta(Frame& f,uint32_t session,uint16_t id=0){fullSlmMeta(f,id);f.bytes[2]=SLM_META;f.bytes[6]=21;put32(payload(f)+17,session);f.bytes[31]=crc8(f.bytes,31);}
inline bool slmCompatible(const Frame& f,uint32_t session){if(!valid(f)||op(f)!=SLM_META||seq(f)||len(f)!=21||get32(payload(f)+17)!=session)return false;Frame meta=f;meta.bytes[2]=HELLO_ACK;meta.bytes[6]=17;memset(payload(meta)+17,0,7);meta.bytes[31]=crc8(meta.bytes,31);return fullSlmCompatible(meta);}
inline void peripheralMeta(Frame& f,uint32_t session,uint16_t id=0){uint8_t p[10];put32(p,session);put32(p+4,MODEL_ID);p[8]=LABELS_VERSION;p[9]=PERIPHERAL_CAPABILITIES;make(f,PERIPHERAL_META,id,0,p,10);}
inline bool peripheralCompatible(const Frame& f,uint32_t session){return valid(f)&&op(f)==PERIPHERAL_META&&!seq(f)&&len(f)==10&&get32(payload(f))==session&&get32(payload(f)+4)==MODEL_ID&&payload(f)[8]==LABELS_VERSION&&payload(f)[9]==PERIPHERAL_CAPABILITIES;}
inline bool supportedIntent(uint8_t id){return id==FORWARD||id==REVERSE||id==ALL_STOP||id==LIGHT_ON||id==LIGHT_OFF||id==HORN_ON||id==HORN_OFF;}
}

// DIAGNOSTIC21 len24 MAIN->SLM: SID32,epoch32,linkBits,cause,peerError,sensorFlags,
// uartBytes16,uartFrames16,uartErrors16,spiBadFrames16,adasAge16,sampleId16.
// linkBits: SLM1,PERIPH2,PERIPH_SAFE4,ADAS_META8,ADAS_HEALTHY16,ADAS_CLEAR32,ALERT64,READY128.
// Optional HELLO ERROR_REPLY BAD_SESSION len6: HELLO,error,highestPersistedSession32.
// Existing len2 errors remain supported. Diagnostics never grant readiness or motion.
namespace Proto {enum DiagnosticCause:uint8_t {DIAG_NONE=0,UART_RX_ERROR=1,UART_WATCHDOG=2,SPI_TIMEOUT=3,SPI_PEER_ERROR=4,SPI_INHIBITED=5,BAD_APPLIED_ACK=6,ADAS_UNHEALTHY=7,ALERT_EVENT=8,UART_BAD_COMMAND=9,SPI_SESSION_RECOVERY=10};}
