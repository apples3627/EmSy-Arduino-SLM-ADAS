#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#ifdef __AVR__
#include <avr/pgmspace.h>
#define INTENT_STORAGE PROGMEM
inline int8_t rd8(const int8_t* p){return (int8_t)pgm_read_byte(p);}
inline char rdc(const char* p){return (char)pgm_read_byte(p);}
inline uint16_t rd16(const uint16_t* p){return pgm_read_word(p);}
inline float rdf(const float* p){return pgm_read_float(p);}
#else
#define INTENT_STORAGE
inline int8_t rd8(const int8_t* p){return *p;}
inline char rdc(const char* p){return *p;}
inline uint16_t rd16(const uint16_t* p){return *p;}
inline float rdf(const float* p){return *p;}
#endif
#ifndef INTENT_PART
#define INTENT_PART 0
#endif
#include "../generated/config.h"
#if INTENT_PART == 1
#include "../generated/model_i2c.h"
#elif INTENT_PART == 2
#include "../generated/model_spi.h"
#else
#include "../generated/model_full.h"
#include "../generated/main_bias.h"
#endif
namespace Intent {
enum Reason { OK, NON_ASCII, TOO_LONG, EMPTY_NUL, UNSUPPORTED, EMPTY, TOO_MANY, OOV, GUARDED, UNKNOWN, LOW_CONFIDENCE, NONFINITE };
struct Input {uint16_t tokens[16];uint8_t count;bool guard;Reason reason;};
inline bool equalsPgm(const char* word,const char* table,uint16_t off){size_t i=0;for(;;i++){char c=rdc(table+off+i);if(c!=word[i])return false;if(c==0)return true;}}
inline void parse(const char* raw,size_t length,Input& input){
 input.count=0;input.guard=false;input.reason=OK;
 for(int i=0;i<16;i++)input.tokens[i]=IntentConfig::PAD;
 if(length>96){input.reason=TOO_LONG;return;}
 if(!length){input.reason=EMPTY_NUL;return;}
 for(size_t i=0;i<length;i++){unsigned char c=raw[i];if(c>=128){input.reason=NON_ASCII;return;}if(!c){input.reason=EMPTY_NUL;return;}if(c<32||c>126){input.reason=UNSUPPORTED;return;}}
 size_t end=length;while(end&&raw[end-1]==' ')end--;
 while(end&&(raw[end-1]=='.'||raw[end-1]=='!'||raw[end-1]=='?'))end--;
 while(end&&raw[end-1]==' ')end--;
 size_t pos=0;
 while(pos<end){
  while(pos<end&&raw[pos]==' ')pos++;if(pos==end)break;
  char word[97];size_t n=0;
  while(pos<end&&raw[pos]!=' '){char c=raw[pos++];if(c>='A'&&c<='Z')c=(char)(c-'A'+'a');if(c<'a'||c>'z'){input.reason=UNSUPPORTED;return;}word[n++]=c;}word[n]=0;
  if(input.count>=16){input.reason=TOO_MANY;return;}
  int token=-1;
  for(int i=0;i<IntentConfig::VOCAB;i++)if(equalsPgm(word,IntentConfig::WORD_TEXT,rd16(IntentConfig::WORD_OFFSETS+i))){token=i;break;}
  if(token<0){input.reason=OOV;return;}
  input.tokens[input.count++]=(uint16_t)token;
  for(int i=0;i<IntentConfig::GUARD_COUNT;i++)if(equalsPgm(word,IntentConfig::GUARD_TEXT,rd16(IntentConfig::GUARD_OFFSETS+i)))input.guard=true;
 }
 if(!input.count)input.reason=EMPTY;
}
// Partial score kernel: copied embedding, local hidden neurons, no global bias on workers.
inline void logits(const Input& input,float* out){
 float pooled[12]={0};
 for(uint8_t i=0;i<input.count;i++)for(int k=0;k<12;k++)pooled[k]+=(float)rd8(IntentWeights::E+input.tokens[i]*12+k)*IntentWeights::SE;
 for(int k=0;k<12;k++)pooled[k]/=(float)input.count;
 for(int t=0;t<10;t++){
#if INTENT_PART == 0
 out[t]=rdf(IntentMain::C+t);
#else
 out[t]=0;
#endif
 }
 for(int j=0;j<IntentWeights::HIDDEN;j++){
  float z=rdf(IntentWeights::B+j);
  for(int k=0;k<12;k++)z+=pooled[k]*((float)rd8(IntentWeights::W+k*IntentWeights::HIDDEN+j)*IntentWeights::SW);
  if(z<0)z=0;
  for(int t=0;t<10;t++)out[t]+=z*((float)rd8(IntentWeights::O+j*10+t)*IntentWeights::SO);
 }
}
struct Result{uint8_t raw,effective;Reason reason;float confidence,margin;};
inline Result decide(const Input& input,const float* z){
 Result r={7,7,input.reason,0,0};if(input.reason!=OK)return r;
 uint8_t best=0;for(int i=0;i<10;i++){if(!isfinite(z[i])){r.reason=NONFINITE;return r;}if(z[i]>z[best])best=(uint8_t)i;}
 r.raw=best;float sum=0;float second=0;float p[10];for(int t=0;t<10;t++){p[t]=expf(z[t]-z[best]);sum+=p[t];}for(int t=0;t<10;t++)if(t!=best&&p[t]>second)second=p[t];
 r.confidence=1/sum;r.margin=(1-second)/sum;
 if(input.guard){r.reason=GUARDED;return r;}
 if(best==7){r.reason=UNKNOWN;return r;}
 if(r.confidence<IntentConfig::THRESHOLD||r.margin<IntentConfig::MARGIN){r.reason=LOW_CONFIDENCE;return r;}
 r.effective=best;r.reason=OK;return r;
}
struct OutputState{int8_t drive;bool lights,wiper,horn;};
// Software contract only, NOT an uploaded actuator implementation.
inline void applyPolicy(uint8_t intent,OutputState& s){
 if(intent==2||intent==7){s.drive=0;s.lights=false;s.wiper=false;s.horn=false;return;}
 switch(intent){case 0:s.drive=1;break;case 1:s.drive=-1;break;case 3:s.lights=true;break;case 4:s.lights=false;break;case 5:s.wiper=true;break;case 6:s.wiper=false;break;case 8:s.horn=true;break;case 9:s.horn=false;break;default:s.drive=0;s.lights=false;s.wiper=false;s.horn=false;}
}
}
