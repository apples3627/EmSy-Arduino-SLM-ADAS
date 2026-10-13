#pragma once
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);} }while(0)
inline Proto::Frame hello(uint32_t sid=100){uint8_t p[9];Proto::put32(p,sid);Proto::put32(p+4,Proto::MODEL_ID);p[8]=Proto::LABELS_VERSION;Proto::Frame f;Proto::make(f,Proto::HELLO,1,0,p,9);return f;}
inline Proto::Frame stop(uint16_t id,uint32_t ep,uint32_t sid=100){uint8_t p[10];Proto::put32(p,sid);Proto::put32(p+4,ep);Proto::put16(p+8,Proto::SLM_STOP);Proto::Frame f;Proto::make(f,Proto::STOP_ALL,id,0,p,10);return f;}
inline Proto::Frame batch(uint16_t id,uint32_t ep,std::initializer_list<uint8_t> ids,uint32_t sid=100){uint8_t p[24];Proto::put32(p,sid);Proto::put32(p+4,ep);p[8]=ids.size();uint8_t n=0;for(auto x:ids)p[9+n++]=x;Proto::put16(p+9+n,500);Proto::Frame f;Proto::make(f,Proto::ACTUATE_BATCH,id,0,p,11+n);return f;}
inline Proto::Frame policy(uint16_t id,uint32_t ep,uint8_t mode,uint32_t sid=100){uint8_t p[9];Proto::put32(p,sid);Proto::put32(p+4,ep);p[8]=mode;Proto::Frame f;Proto::make(f,23,id,0,p,9);return f;}
inline Proto::Frame telemetry(uint16_t id,uint32_t ep,uint16_t sample,uint16_t mm,uint16_t age,uint8_t flags,uint8_t mode=1,uint32_t sid=100){uint8_t p[16];Proto::put32(p,sid);Proto::put32(p+4,ep);Proto::put16(p+8,sample);Proto::put16(p+10,mm);Proto::put16(p+12,age);p[14]=flags;p[15]=mode;Proto::Frame f;Proto::make(f,24,id,0,p,16);return f;}
inline Proto::Frame sync(const Proto::SharedState& s){uint8_t p[22];Proto::encodeState(p,s);Proto::Frame f;Proto::make(f,Proto::STATE_SYNC,s.command,0,p,22);return f;}
