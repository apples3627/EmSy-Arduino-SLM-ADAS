#include "src/intent_model.h"
#include "src/command_plan.h"
static char line[193];static uint8_t used=0;static bool over=false;
static Intent::Input input;static float scores[10];static CommandPlan::Split splitResult;static CommandPlan::Decisions plan;
void printLabel(uint8_t label){const char* p=IntentConfig::LABEL_TEXT+rd16(IntentConfig::LABEL_OFFSETS+label);char c;while((c=rdc(p++)))Serial.print(c);}
void setup(){Serial.begin(9600);Serial.println(F("INTENT PROBE ONLY: no motors connected; READY"));}
void loop(){while(Serial.available()){
 char c=(char)Serial.read();if(c=='\r')continue;
 if(c=='\n'){
  if(over)Serial.println(F("ERROR too_long -> GLOBAL_STOP"));
  else if(!CommandPlan::split(line,used,splitResult)){Serial.print(F("PLAN 0 REASON "));Serial.println((int)splitResult.reason);}
  else {plan.reset();for(uint8_t j=0;j<splitResult.count;j++){
   // Use an ephemeral bounded clause; raw input spans never duplicate the 192-byte buffer.
   char clause[97];CommandPlan::extract(line,splitResult,j,clause);Intent::parse(clause,strlen(clause),input);
   if(input.reason==Intent::OK){Intent::logits(input,scores);
#if INTENT_PART == 0
    Intent::Result r=Intent::decide(input,scores);plan.append(r.effective);
    if(splitResult.count==1){Serial.print(F("RAW "));printLabel(r.raw);Serial.print(F(" EFFECTIVE "));printLabel(r.effective);Serial.print(F(" REASON "));Serial.println((int)r.reason);}
#else
    Serial.print(F("CLAUSE "));Serial.print(j);Serial.print(F(" PARTIAL "));for(int t=0;t<10;t++){Serial.print(scores[t],6);Serial.print(' ');}Serial.println();
#endif
   }else{plan.append(7);Serial.print(F("CLAUSE_ERROR "));Serial.println((int)input.reason);}
  }
#if INTENT_PART == 0
  plan.finish();Serial.print(F("PLAN "));Serial.print(plan.count);for(uint8_t j=0;j<plan.count;j++){Serial.print(' ');Serial.print(plan.intents[j]);}Serial.print(F(" REASON "));Serial.println((int)plan.reason);
#endif
  }
  used=0;over=false;Serial.println(F("READY"));
 }else if(!over){if(used<192)line[used++]=c;else over=true;}
}}
