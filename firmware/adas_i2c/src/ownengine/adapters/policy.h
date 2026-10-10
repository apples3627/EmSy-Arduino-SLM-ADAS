#pragma once
#include "../../../config.h"
#include "../../protocol.h"
namespace Adas {
enum Flag:uint8_t{STARTUP=1,OBSTACLE=2,SENSOR_FAULT=4,VALID=8,ALERT=16};
struct SourceRegistry {
 uint8_t enabled,hazards[AdasConfig::SOURCE_CAPACITY];
 SourceRegistry():enabled(0),hazards(){}
 bool registerSource(uint8_t index,bool enable=true){if(index>=AdasConfig::SOURCE_CAPACITY)return false;if(enable){enabled|=1<<index;hazards[index]=STARTUP;}else{enabled&=~(1<<index);hazards[index]=0;}return true;}
 bool update(uint8_t index,uint8_t flags){if(index>=AdasConfig::SOURCE_CAPACITY||!(enabled&(1<<index)))return false;hazards[index]=flags&(STARTUP|OBSTACLE|SENSOR_FAULT);return true;}
 uint8_t flags()const{uint8_t f=0;for(uint8_t i=0;i<AdasConfig::SOURCE_CAPACITY;i++)if(enabled&(1<<i))f|=hazards[i];return f;}
 bool asserted()const{return flags()!=0;}
};
struct Policy {
 SourceRegistry sources; uint32_t lastValid,bootTime; uint16_t rangeMm,sampleId; uint8_t hazard,clearCount;bool valid,haveSample;
 Policy():lastValid(0),bootTime(0),rangeMm(0xffff),sampleId(0),hazard(STARTUP),clearCount(0),valid(false),haveSample(false){sources.registerSource(0);}
 void fault(){valid=false;rangeMm=0xffff;clearCount=0;hazard|=SENSOR_FAULT;sources.update(0,hazard);}
 void sample(uint32_t widthTicks,uint32_t now){
  sampleId++;
  if(widthTicks<232||widthTicks>46400){fault();return;}
  rangeMm=uint16_t((widthTicks*5UL+29)/58);lastValid=now;haveSample=valid=true;
  if(rangeMm<=AdasConfig::STOP_CM*10U){hazard|=OBSTACLE;clearCount=0;}
  else if(rangeMm>=AdasConfig::RELEASE_CM*10U){if(clearCount<AdasConfig::CLEAR_REQUIRED)clearCount++;if(clearCount>=AdasConfig::CLEAR_REQUIRED)hazard=0;}
  else clearCount=0;
  sources.update(0,hazard);
 }
 void tick(uint32_t now){if(uint32_t(now-(haveSample?lastValid:bootTime))>AdasConfig::STALE_MS)fault();}
 uint8_t flags()const{return sources.flags()|(valid?VALID:0)|(sources.asserted()?ALERT:0);}
 uint16_t age(uint32_t now)const{if(!haveSample)return 0xffff;uint32_t age=now-lastValid;return age>65534?65534:uint16_t(age);}
};
}
