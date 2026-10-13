#include <cassert>
#include <initializer_list>
#include <cmath>
#include <cstdio>
#ifndef ADAS_POLICY_HEADER
#define ADAS_POLICY_HEADER "../../adas_i2c/src/ownengine/adapters/policy.h"
#endif
#include ADAS_POLICY_HEADER
static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static uint32_t pulse(uint16_t mm){return (uint32_t(mm)*58+2)/5;}
int main(){
 for(uint16_t mm: {199,200,201,249,250}){Adas::Policy p;for(int i=0;i<3;i++)p.sample(pulse(300),i);CHECK(!p.sources.asserted());p.sample(pulse(mm),10);CHECK(bool(p.hazard&Adas::OBSTACLE)==(mm<=200));}
 {Adas::Policy p;CHECK(p.sources.asserted());p.sample(pulse(250),1);p.sample(pulse(250),2);CHECK(p.sources.asserted());p.sample(pulse(250),3);CHECK(!p.sources.asserted());p.sample(pulse(200),4);CHECK(p.sources.asserted());p.sample(pulse(250),5);p.sample(0,6);p.sample(pulse(250),7);p.sample(pulse(250),8);CHECK(p.sources.asserted());p.sample(pulse(250),9);CHECK(!p.sources.asserted());}
 {Adas::Policy p;for(int i=0;i<3;i++)p.sample(pulse(300),10);p.tick(210);CHECK(!p.sources.asserted());p.tick(211);CHECK(p.hazard&Adas::SENSOR_FAULT);CHECK(!p.valid&&p.rangeMm==0xffff);}
 {Adas::Policy p;for(int i=0;i<3;i++)p.sample(pulse(300),0xffffffA0);p.tick(104);CHECK(!p.sources.asserted());p.tick(105);CHECK(p.sources.asserted());}
 for(uint32_t ticks: {231,232,46400,46401}){Adas::Policy p;p.sample(ticks,1);CHECK(p.valid==(ticks>=232&&ticks<=46400));}
 {Adas::SourceRegistry r;CHECK(r.registerSource(0));CHECK(r.registerSource(1));CHECK(r.update(0,Adas::OBSTACLE));CHECK(r.update(1,Adas::SENSOR_FAULT));CHECK(r.update(0,0));CHECK(r.asserted()&&(r.flags()&Adas::SENSOR_FAULT));CHECK(r.update(1,0));CHECK(!r.asserted());}
 printf("PASS ADAS policy %u assertions\n",checks);
}
