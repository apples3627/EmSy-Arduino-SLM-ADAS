#include "../shared/intent_model.h"
static char line[97];static uint8_t used=0;static bool over=false;
static Intent::Input input;static float scores[10];
void printLabel(uint8_t label){const char* p=IntentConfig::LABEL_TEXT+rd16(IntentConfig::LABEL_OFFSETS+label);char c;while((c=rdc(p++)))Serial.print(c);}
void setup(){Serial.begin(9600);Serial.println(F("INTENT PROBE ONLY: no motors connected; READY"));}
void loop(){while(Serial.available()){
 char c=(char)Serial.read();if(c=='\r')continue;
 if(c=='\n'){
  if(over)Serial.println(F("ERROR too_long -> GLOBAL_STOP"));
  else{Intent::parse(line,used,input);if(input.reason==Intent::OK){Intent::logits(input,scores);
#if INTENT_PART == 0
   Intent::Result r=Intent::decide(input,scores);Serial.print(F("RAW "));printLabel(r.raw);Serial.print(F(" EFFECTIVE "));printLabel(r.effective);Serial.print(F(" REASON "));Serial.println((int)r.reason);
#else
   Serial.print(F("PARTIAL "));for(int t=0;t<10;t++){Serial.print(scores[t],6);Serial.print(' ');}Serial.println();
#endif
  }else{Serial.print(F("ERROR "));Serial.println((int)input.reason);}}
  used=0;over=false;Serial.println(F("READY"));
 }else if(!over){if(used<96)line[used++]=c;else over=true;}
}}
