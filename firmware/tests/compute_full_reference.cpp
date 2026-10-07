#include <cassert>
#include <cstdio>
#include "../main/src/model/intent_model.h"
int main(){const char* samples[]={"forward","reverse","stop","turn lights on","turn lights off","start wipers","stop wipers","honk horn","silence horn","please move forward"};for(const char* s:samples){Intent::Input in;Intent::parse(s,strlen(s),in);assert(in.reason==Intent::OK);float z[10];Intent::logits(in,z);for(int i=0;i<10;i++)printf("%.9g ",z[i]);puts("");}for(int i=0;i<10;i++)printf("%.9g ",rdf(IntentMain::C+i));puts("");}
