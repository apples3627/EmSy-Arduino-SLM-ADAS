#pragma once
#include "model/intent_model.h"
#include "model/command_plan.h"
namespace SlmUart {
inline bool exactStop(const char* s,size_t n){size_t a=0;while(a<n&&s[a]==' ')++a;while(n>a&&s[n-1]==' ')--n;while(n>a&&(s[n-1]=='.'||s[n-1]=='!'||s[n-1]=='?'))--n;while(n>a&&s[n-1]==' ')--n;return n-a==4&&CommandPlan::lower(s[a])=='s'&&CommandPlan::lower(s[a+1])=='t'&&CommandPlan::lower(s[a+2])=='o'&&CommandPlan::lower(s[a+3])=='p';}
struct Planner {
 CommandPlan::Split split;CommandPlan::Decisions decisions;uint8_t clause,kernels;bool active;uint32_t generation,epoch;
 Planner():clause(0),kernels(0),active(false),generation(0),epoch(0){decisions.reset();}
 bool begin(const char* line,size_t n,uint32_t g,uint32_t e){decisions.reset();clause=kernels=0;generation=g;epoch=e;active=false;if(exactStop(line,n)){decisions.append(2);decisions.finish();return true;}if(!CommandPlan::split(line,n,split))return false;active=true;return true;}
 bool prepare(const char* line,char* clauseText,Intent::Input& input){if(!active)return false;CommandPlan::extract(line,split,clause,clauseText);Intent::parse(clauseText,strlen(clauseText),input);return input.reason==Intent::OK&&!input.guard;}
 void accept(uint8_t intent){if(intent==5||intent==6)intent=7;decisions.append(intent);if(decisions.stop||++clause>=split.count){active=false;decisions.finish();}}
 bool valid(uint32_t g,uint32_t e)const{return generation==g&&epoch==e;}
};
}
