#include "tiny_model.h"
// Global SRAM: 64-byte prompt + 16-byte context + 192-byte neural workspace.
static char prompt[64];
static uint8_t context[MODEL_CONTEXT];
static float workspace[MODEL_HIDDEN];
static uint8_t promptLength=0;
static bool overflow=false;
static const uint8_t MAX_NEW_TOKENS=80;
void generate() {
  TinyModel::reset(context);
  for(uint8_t i=0;i<promptLength;i++) TinyModel::push(context,TinyModel::encode(prompt[i]));
  Serial.print(F("TEXT "));
  for(uint8_t i=0;i<promptLength;i++) Serial.print(TinyModel::decode(TinyModel::encode(prompt[i])));
  digitalWrite(LED_BUILTIN,HIGH);
  for(uint8_t i=0;i<MAX_NEW_TOKENS;i++) {
    uint8_t t=TinyModel::next(context,workspace);
    if(t==0) break;
    Serial.print(TinyModel::decode(t)); TinyModel::push(context,t);
  }
  digitalWrite(LED_BUILTIN,LOW);
  Serial.println(); Serial.println(F("READY"));
}
void setup() {
  pinMode(LED_BUILTIN,OUTPUT); Serial.begin(9600);
  Serial.println(F("TINY UNO neural character demo; ASCII prompts, max 63 bytes"));
  Serial.println(F("READY"));
}
void loop() {
  while(Serial.available()>0) {
    char c=(char)Serial.read();
    if(c=='\r') continue;
    if(c=='\n') {
      if(overflow) {Serial.println(F("ERROR prompt too long; max 63 bytes")); Serial.println(F("READY"));}
      else generate();
      promptLength=0; overflow=false;
    } else if(!overflow) {
      if(promptLength<sizeof(prompt)-1) prompt[promptLength++]=c;
      else overflow=true; // Discard through newline, then recover for next prompt.
    }
  }
}
