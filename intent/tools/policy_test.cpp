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
 puts("PASS: STOP/UNKNOWN all outputs off, component commands independent, horn latch, NaN rejection");
}
