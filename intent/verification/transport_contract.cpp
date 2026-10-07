#include <cassert>
#include <cmath>
#include <cstdio>
#include "../shared/intent_model.h"
int main(){
 Intent::Input x;Intent::parse("please turn the horn on",sizeof("please turn the horn on")-1,x);assert(x.reason==Intent::OK);
 uint16_t block[16];for(int k=0;k<16;k++)block[k]=IntentConfig::PAD;
 for(int k=0;k<x.count;k++)block[k*2]=x.tokens[k];
 Intent::Input d;Intent::decodeTokens(block,d);assert(d.reason==Intent::OK&&d.count==x.count);
 float a[10],b[10];Intent::logits(x,a);Intent::logits(d,b);for(int k=0;k<10;k++)assert(a[k]==b[k]);
 block[15]=IntentConfig::VOCAB;Intent::decodeTokens(block,d);assert(d.reason==Intent::OOV);
 for(int k=0;k<16;k++)block[k]=IntentConfig::PAD;Intent::decodeTokens(block,d);assert(d.reason==Intent::EMPTY);
 d.reason=Intent::OK;d.count=0;Intent::logits(d,b);assert(std::isnan(b[0])&&Intent::decide(d,b).effective==7);
 d.reason=Intent::OK;d.count=1;d.tokens[0]=IntentConfig::VOCAB;Intent::logits(d,b);assert(std::isnan(b[0])&&Intent::decide(d,b).effective==7);
 Intent::OutputState s={-1,true,true,true};Intent::applyPolicy(6,s);assert(s.drive==-1&&s.lights&&!s.wiper&&s.horn);
 Intent::applyPolicy(9,s);assert(s.drive==-1&&s.lights&&!s.horn);Intent::applyPolicy(8,s);assert(s.horn);
 const int globals[2]={2,7};for(int label: globals){s={-1,true,true,true};Intent::applyPolicy(label,s);assert(!s.drive&&!s.lights&&!s.wiper&&!s.horn);}
 puts("PASS independent padded IDs, invalid bounds, empty kernel, component stop, global stop, horn state");
}
