#include <iostream>
#include "../shared/intent_model.h"
int main(){
 for(int label=0;label<12;label++){
  Intent::OutputState s={1,true,true,true};Intent::applyPolicy(label,s);
  std::cout<<label<<" "<<(int)s.drive<<" "<<s.lights<<" "<<s.wiper<<" "<<s.horn<<"\n";
 }
 Intent::Input x;Intent::parse("move forward",12,x);float z[10];Intent::logits(x,z);
 z[3]=NAN;auto r=Intent::decide(x,z);return r.effective==7&&r.reason==Intent::NONFINITE?0:1;
}
