#include <cassert>
#include <cstdio>
#include "../shared/intent_model.h"
int main(){
 Intent::OutputState state={1,true,true,true};Intent::applyPolicy(2,state);
 assert(state.drive==0&&!state.lights&&!state.wiper&&!state.horn);
 state={-1,true,true,true};Intent::applyPolicy(7,state);assert(state.drive==0&&!state.lights&&!state.wiper&&!state.horn);
 state={1,true,true,true};Intent::applyPolicy(6,state);assert(state.drive==1&&state.lights&&!state.wiper&&state.horn);
 Intent::applyPolicy(9,state);assert(!state.horn&&state.drive==1&&state.lights);
 Intent::applyPolicy(8,state);assert(state.horn);
 Intent::Input input;Intent::parse("move forward",12,input);float invalid[10]={0};invalid[0]=NAN;auto r=Intent::decide(input,invalid);assert(r.effective==7&&r.reason==Intent::NONFINITE);
 uint16_t block[16];for(int i=0;i<16;i++)block[i]=IntentConfig::PAD;
 block[0]=input.tokens[0];block[1]=input.tokens[1];Intent::Input decoded;Intent::decodeTokens(block,decoded);
 assert(decoded.reason==Intent::OK&&decoded.count==2&&decoded.tokens[0]==input.tokens[0]);
 float first[10],second[10];Intent::logits(input,first);Intent::logits(decoded,second);for(int i=0;i<10;i++)assert(first[i]==second[i]);
 block[3]=65534;Intent::decodeTokens(block,decoded);assert(decoded.reason==Intent::OOV);
 for(int i=0;i<16;i++)block[i]=IntentConfig::PAD;Intent::decodeTokens(block,decoded);assert(decoded.reason==Intent::EMPTY);
 puts("PASS: STOP/UNKNOWN all outputs off, component commands independent, horn latch, NaN rejection, padded transport token decoding");
}
