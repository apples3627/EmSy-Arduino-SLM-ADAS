#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
#include "../main/src/protocol.h"
#define _BV(x) (1u<<(x))
enum {TWINT=7,TWEA=6,TWSTA=5,TWSTO=4,TWEN=2,TWIE=0,SPIF=7,PORTB0=0};
#define __builtin_avr_delay_cycles(x) ((void)0)
#define ISR(vector) void vector()
uint8_t PORTB=0,SPSR=0,SPDR=0,TWSR=0,TWDR=0;
struct TwcrShim {
 uint8_t value=0; bool master=false, clearIdleRecovery=false;
 unsigned stopRequests=0,idleStopRequests=0,disables=0;
 operator uint8_t() const {return value;}
 TwcrShim& operator=(uint8_t v) {
  if(!(v&_BV(TWEN))) {master=false;disables++;}
  if(v&_BV(TWSTA)) master=true;
  if(v&_BV(TWSTO)){stopRequests++;if(!master)idleStopRequests++;}
  value=v&uint8_t(~_BV(TWINT)); // TWINT is write-one-to-clear.
  if((v&_BV(TWSTO))&&!master&&clearIdleRecovery)value&=uint8_t(~_BV(TWSTO));
  return *this;
 }
 void realStopCompletes(){if(!master||!(value&_BV(TWSTO))){puts("BAD SHIM STOP");return;}value&=uint8_t(~_BV(TWSTO));master=false;TWSR=0xf8;}
} TWCR;
namespace MainIO{uint32_t ticks=0;}
#ifndef DRIVER_HEADER
#define DRIVER_HEADER "../main/src/compute_links.h"
#endif
#include DRIVER_HEADER
unsigned failures=0;
void check(bool ok,const char* msg){printf("%s %s\n",ok?"PASS":"FAIL",msg);if(!ok)failures++;}
void reset(){std::memset(MainLinks::bus,0,sizeof MainLinks::bus);TWCR=TwcrShim{};MainIO::ticks=0;PORTB=SPSR=SPDR=TWSR=TWDR=0;}
void poll(uint32_t t){MainIO::ticks=t;MainLinks::service(0,t);}
void event(uint8_t status,uint32_t t,uint8_t data=0){MainIO::ticks=t;TWSR=status;TWDR=data;TWCR.value|=_BV(TWINT);TWI_vect();}
Proto::Frame req(){Proto::Frame f;uint8_t sid[4]={1,0,0,0};Proto::make(f,Proto::HELLO,1,0,sid,4);return f;}
Proto::Frame reply(){Proto::Frame f;uint8_t meta[8]={Proto::ADAS_ROLE,Proto::HC_SR04,25,30,3,Proto::VERSION,100,0};Proto::make(f,Proto::ADAS_META,1,0,meta,8);return f;}
void fullRead(bool corrupt=false){
 auto request=req(),response=reply();if(corrupt)response.bytes[31]^=1;
 MainLinks::begin(0,request,0);event(8,1);event(0x18,2);
 for(unsigned i=0;i<32;i++)event(0x28,3+i);
 check(MainLinks::bus[0].phase==4,"request phase waits for its real STOP");
 check(!(uint8_t(TWCR)&_BV(TWIE)),"request STOP has TWI interrupt disabled");
 TWCR.realStopCompletes();poll(35);poll(37);event(8,38);event(0x40,39);
 for(unsigned i=0;i<32;i++)event(i==31?0x58:0x50,40+i,response.bytes[i]);
 check(MainLinks::bus[0].phase==9,"last byte advances to response STOP wait");
 check(!(uint8_t(TWCR)&_BV(TWIE)),"response STOP has TWI interrupt disabled");
 TWCR.realStopCompletes();poll(72);
}
void validReply(){reset();fullRead();auto& b=MainLinks::bus[0];check(TWCR.stopRequests==2&&TWCR.idleStopRequests==0,"one STOP for request and one for response; no idle STOP");check(b.phase==0&&b.done&&b.ok,"foreground response STOP completion publishes CRC-valid success immediately");
 printf("OBSERVE phase=%u done=%u ok=%u stopRequests=%u idleStopRequests=%u deadline=%u\n",b.phase,b.done,b.ok,TWCR.stopRequests,TWCR.idleStopRequests,b.deadline);
 // Holding any redundant idle-recovery bit is fault injection, not hardware proof.
 if(b.phase)poll(b.deadline);
 check(b.done&&b.ok,"valid reply survives a synthetic uncleared redundant idle request");
}
void badCrc(){reset();fullRead(true);check(MainLinks::bus[0].phase==0&&MainLinks::bus[0].done&&!MainLinks::bus[0].ok,"CRC-invalid response completes as failure without another STOP");}
void nack(bool read){reset();MainLinks::begin(0,req(),0);if(read){MainLinks::bus[0].phase=7;}else{event(8,1);}event(read?0x48:0x20,2);auto& b=MainLinks::bus[0];check(b.phase==10&&!b.ok&&TWCR.stopRequests==1,"address NACK requests one bounded error STOP");check(!(uint8_t(TWCR)&_BV(TWIE)),"NACK error STOP has TWI interrupt disabled");poll(b.deadline-1);check(b.phase==10&&!b.done,"stuck NACK STOP waits only until deadline");poll(b.deadline);check(b.phase==0&&b.done&&!b.ok&&TWCR.disables==1,"stuck NACK STOP resets TWI and completes failure at deadline");}
void timeout(){reset();MainLinks::begin(0,req(),0);poll(100);auto& b=MainLinks::bus[0];check(b.phase==10&&!b.done&&!b.ok&&TWCR.stopRequests==1,"missing interrupt reaches bounded error STOP at transaction timeout");poll(199);check(!b.done,"error STOP retains its bound before deadline");poll(200);check(b.done&&!b.ok&&b.phase==0&&TWCR.disables==1,"missing interrupt plus stuck STOP terminates after two 100ms bounds");}
int main(){validReply();badCrc();nack(false);nack(true);timeout();printf("TOTAL_FAILURES=%u\n",failures);return failures?1:0;}
