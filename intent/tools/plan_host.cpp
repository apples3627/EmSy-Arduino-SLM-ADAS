#include <iostream>
#include <string>
#include "../uno_probe/src/intent_model.h"
#include "../uno_probe/src/command_plan.h"
void printJsonString(const char* text){
 std::cout << '"';
 for(const char* p=text;*p;++p){if(*p=='"'||*p=='\\')std::cout << '\\';std::cout << *p;}
 std::cout << '"';
}
int main(){std::string text;while(std::getline(std::cin,text)){CommandPlan::Split s;CommandPlan::Decisions d;d.reset();bool ok=CommandPlan::split(text.data(),text.size(),s);std::cout<<"{\"clauses\":[";if(ok){for(uint8_t i=0;i<s.count;i++){char clause[97];CommandPlan::extract(text.data(),s,i,clause);if(i)std::cout<<',';printJsonString(clause);Intent::Input in;Intent::parse(clause,strlen(clause),in);float z[10]={0};if(in.reason==Intent::OK)Intent::logits(in,z);d.append(Intent::decide(in,z).effective);}d.finish();}std::cout<<"],\"reason\":"<<(int)(ok?d.reason:s.reason)<<",\"actions\":[";if(ok)for(uint8_t i=0;i<d.count;i++){if(i)std::cout<<',';std::cout<<(int)d.intents[i];}std::cout<<"]}\n";}}
