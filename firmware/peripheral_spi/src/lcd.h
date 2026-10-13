#pragma once
#include <avr/io.h>
#include "../config.h"
#include "engine.h"
namespace Peripheral {
// One TWI transition per poll. Never wait for TWINT, TWSTO, or LCD busy in a loop.
class Lcd {
 uint8_t bus_,count_,sent_,bytes_[6],init_,cursor_;uint32_t started_,due_;bool fault_,initialized_,command_;char wanted_[32],shown_[32];uint8_t pendingValue_;bool pendingRs_;
 static bool reached(uint32_t now,uint32_t due){return int32_t(now-due)>=0;}
 uint8_t expander(uint8_t nibble,bool rs){uint8_t v=_BV(PERIPHERAL_LCD_BL_BIT);if(rs)v|=_BV(PERIPHERAL_LCD_RS_BIT);if(nibble&1)v|=_BV(PERIPHERAL_LCD_D4_BIT);if(nibble&2)v|=_BV(PERIPHERAL_LCD_D5_BIT);if(nibble&4)v|=_BV(PERIPHERAL_LCD_D6_BIT);if(nibble&8)v|=_BV(PERIPHERAL_LCD_D7_BIT);return v;}
 void nibble(uint8_t n,bool rs,uint8_t offset){uint8_t v=expander(n,rs);bytes_[offset]=v;bytes_[offset+1]=v|_BV(PERIPHERAL_LCD_EN_BIT);bytes_[offset+2]=v;}
 void start(uint8_t value,bool rs,bool half,uint32_t now){pendingValue_=value;pendingRs_=rs;count_=half?3:6;nibble(half?value:value>>4,rs,0);if(!half)nibble(value&15,rs,3);sent_=0;bus_=1;started_=now;TWCR=_BV(TWINT)|_BV(TWSTA)|_BV(TWEN);}
 void fail(uint32_t now){TWCR=0;TWCR=_BV(TWEN);bus_=0;fault_=true;initialized_=false;init_=0;cursor_=0;command_=true;due_=now+1000;memset(shown_,0,32);}
 void done(uint32_t now){bus_=0;if(!initialized_){init_++;static const uint8_t waits[8]={0,5,1,1,1,1,1,2};due_=now+(init_<8?waits[init_]:2);if(init_==8){initialized_=true;fault_=false;cursor_=0;command_=true;}}
  else {if(command_)command_=false;else {shown_[cursor_]=char(pendingValue_);cursor_=(cursor_+1)&31;command_=true;}due_=now+1;}}
 static void decimal(char* p,uint32_t value){value%=100000;for(int8_t i=4;i>=0;i--){p[i]=char('0'+value%10);value/=10;}}
 public:
 Lcd():bus_(0),count_(0),sent_(0),init_(0),cursor_(0),started_(0),due_(40),fault_(false),initialized_(false),command_(true){memset(wanted_,' ',32);memset(shown_,0,32);}
 void setup(uint32_t now){TWBR=72;TWSR=0;TWCR=_BV(TWEN);due_=now+40;}
 bool fault()const{return fault_;} bool busy()const{return bus_!=0;}
 void display(const Engine& engine){memcpy(wanted_,"DRV:? LED:0 H:0  ",16);memcpy(wanted_+16,"E:00000 S:00000 ",16);const Proto::SharedState& s=engine.snapshot();wanted_[10]=engine.outputs().lights?'1':'0';wanted_[14]=engine.outputs().horn?'1':'0';if(engine.hasSnapshot()){wanted_[4]=s.appliedKnown&&s.safetyEpoch==engine.epoch()&&!engine.inhibited()?(s.appliedDrive>0?'F':s.appliedDrive<0?'R':'0'):'?';decimal(wanted_+18,engine.inhibited()?engine.epoch():s.safetyEpoch);decimal(wanted_+26,s.stopSources);}else {decimal(wanted_+18,engine.epoch());}if(engine.inhibited())wanted_[31]='!';}
 void poll(uint32_t now){
  if(bus_){if(uint32_t(now-started_)>=5){fail(now);return;}if(bus_==4){if(!(TWCR&_BV(TWSTO)))done(now);return;}if(!(TWCR&_BV(TWINT)))return;uint8_t status=TWSR&0xf8;
   if(bus_==1){if(status!=0x08&&status!=0x10){fail(now);return;}TWDR=PERIPHERAL_LCD_ADDRESS<<1;TWCR=_BV(TWINT)|_BV(TWEN);bus_=2;return;}
   if((bus_==2&&status!=0x18)||(bus_==3&&status!=0x28)){fail(now);return;}
   if(sent_<count_){TWDR=bytes_[sent_++];TWCR=_BV(TWINT)|_BV(TWEN);bus_=3;}else {TWCR=_BV(TWINT)|_BV(TWSTO)|_BV(TWEN);bus_=4;}return;
  }
  if(!reached(now,due_))return;
  if(!initialized_){static const uint8_t sequence[8]={3,3,3,2,0x28,0x0c,0x06,0x01};start(sequence[init_],false,init_<4,now);return;}
  if(command_){for(uint8_t i=0;i<32;i++){if(wanted_[cursor_]!=shown_[cursor_]){start(uint8_t(0x80|(cursor_<16?cursor_:0x40+cursor_-16)),false,false,now);return;}cursor_=(cursor_+1)&31;}due_=now+10;return;}
  start(uint8_t(wanted_[cursor_]),true,false,now);
 }
};
}
