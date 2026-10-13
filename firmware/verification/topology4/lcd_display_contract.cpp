#include <cassert>
#include <cstdio>
#include <vector>
#include PERIPHERAL_LCD_HEADER
#include "frame_helpers.h"
struct WireLcd {
 char cells[32];uint8_t address=0;
 WireLcd(){memset(cells,' ',32);}
 void transfer(Peripheral::Lcd& lcd,uint32_t now){
  TWSR=8;TWCR=_BV(TWINT);lcd.poll(now);assert(TWDR==(PERIPHERAL_LCD_ADDRESS<<1));
  TWSR=0x18;TWCR=_BV(TWINT);lcd.poll(now);std::vector<uint8_t> data;
  while(!(TWCR&_BV(TWSTO))){assert(data.size()<6);data.push_back(uint8_t(TWDR));TWSR=0x28;TWCR=_BV(TWINT);lcd.poll(now);}
  if(data.size()==6){uint8_t value=uint8_t((data[0]&0xf0)|(data[3]>>4));bool rs=data[0]&1;if(rs){uint8_t pos=address<16?address:address>=0x40&&address<0x50?address-0x40+16:255;if(pos<32)cells[pos]=char(value);address++;}else if(value&0x80)address=value&0x7f;else if(value==1){memset(cells,' ',32);address=0;}}
  TWCR&=uint8_t(~_BV(TWSTO));lcd.poll(now);
 }
 void advance(Peripheral::Lcd& lcd,Peripheral::Engine& engine,uint32_t from,uint32_t to){for(uint32_t now=from;now<to;now++){lcd.display(engine);lcd.poll(now);if(lcd.busy())transfer(lcd,now);}assert(!lcd.fault());}
};
int main(){Peripheral::Engine e;Peripheral::Lcd lcd;WireLcd wire;Proto::Frame r;auto q=hello();e.handle(q,r,0);q=stop(2,1);e.handle(q,r,0);q=batch(3,1,{Proto::LIGHT_ON,Proto::HORN_ON});e.handle(q,r,0);Proto::SharedState s={100,1,1,3,1,1,5,5,0,true,0};q=sync(s);e.handle(q,r,0);lcd.setup(0);wire.advance(lcd,e,0,200);assert(wire.cells[4]=='F'&&wire.cells[10]=='1'&&wire.cells[14]=='1');
 e.adas(true,true);wire.advance(lcd,e,200,350);assert(wire.cells[4]=='?'&&wire.cells[10]=='0'&&wire.cells[14]=='0');e.adas(false,false);q=stop(4,2);e.handle(q,r,350);wire.advance(lcd,e,350,500);assert(wire.cells[4]=='?'&&wire.cells[10]=='0'&&wire.cells[14]=='0');
 puts("PASS actual LCD expander output: matching known drive, immediate local accessories, inhibited/old-epoch drive unknown");}
