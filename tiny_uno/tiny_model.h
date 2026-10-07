#pragma once
#include <stdint.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#define MODEL_STORAGE PROGMEM
inline int8_t modelReadI8(const int8_t* p) { return (int8_t)pgm_read_byte(p); }
inline float modelReadFloat(const float* p) { return pgm_read_float(p); }
#else
#define MODEL_STORAGE
inline int8_t modelReadI8(const int8_t* p) { return *p; }
inline float modelReadFloat(const float* p) { return *p; }
#endif
#include "model_weights.h"
namespace TinyModel {
// IDs: newline=0, space=1, a..z=2..27, period=28.
inline uint8_t encode(char c) {
  if(c>='A'&&c<='Z') c=(char)(c-'A'+'a');
  if(c=='\n') return 0;
  if(c>='a'&&c<='z') return (uint8_t)(c-'a'+2);
  if(c=='.') return 28;
  return 1; // All other bytes (including UTF-8) become spaces.
}
inline char decode(uint8_t c) { return c==0?'\n':c==1?' ':c==28?'.':(char)('a'+c-2); }
inline void reset(uint8_t* context) { for(int i=0;i<MODEL_CONTEXT;i++) context[i]=0; }
inline void push(uint8_t* context,uint8_t token) {
  for(int i=1;i<MODEL_CONTEXT;i++) context[i-1]=context[i];
  context[MODEL_CONTEXT-1]=token<MODEL_VOCAB?token:1;
}
// Caller owns hidden workspace (192 bytes). No heap or recursive calls.
inline void hidden(const uint8_t* context,float* h) {
  for(int j=0;j<MODEL_HIDDEN;j++) {
    float sum=modelReadFloat(MODEL_B+j);
    for(int i=0;i<MODEL_CONTEXT;i++) {
      uint8_t id=context[i]<MODEL_VOCAB?context[i]:1;
      for(int k=0;k<MODEL_EMBED;k++) {
        float e=(float)modelReadI8(MODEL_E+id*MODEL_EMBED+k)*MODEL_SE;
        float w=(float)modelReadI8(MODEL_W+(i*MODEL_EMBED+k)*MODEL_HIDDEN+j)*MODEL_SW;
        sum+=e*w;
      }
    }
    h[j]=sum>0?sum:0;
  }
}
inline float logit(const float* h,int token) {
  float sum=modelReadFloat(MODEL_C+token);
  for(int j=0;j<MODEL_HIDDEN;j++) sum+=h[j]*((float)modelReadI8(MODEL_O+j*MODEL_VOCAB+token)*MODEL_SO);
  return sum;
}
inline uint8_t next(const uint8_t* context,float* h) {
  hidden(context,h); uint8_t best=0; float score=logit(h,0);
  for(int t=1;t<MODEL_VOCAB;t++) { float z=logit(h,t); if(z>score) {score=z; best=(uint8_t)t;} }
  return best;
}
}
