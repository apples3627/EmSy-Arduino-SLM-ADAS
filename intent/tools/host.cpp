#include <iostream>
#include <string>
#include <iomanip>
#include "../shared/intent_model.h"
void evaluate(const std::string& s){
 Intent::Input input;Intent::parse(s.data(),s.size(),input);float z[10]={0};
 if(input.reason==Intent::OK)Intent::logits(input,z);
 auto result=Intent::decide(input,z);
 std::cout<<"{\"reason_id\":"<<(int)result.reason<<",\"count\":"<<(int)input.count<<",\"guard\":"<<(input.guard?"true":"false")<<",\"tokens\":[";
 for(int i=0;i<input.count;i++){if(i)std::cout<<',';std::cout<<input.tokens[i];}
 std::cout<<"],\"raw_id\":"<<(int)result.raw<<",\"effective_id\":"<<(int)result.effective<<",\"confidence\":"<<result.confidence<<",\"scores\":[";
 for(int i=0;i<10;i++){if(i)std::cout<<',';std::cout<<std::setprecision(9)<<z[i];}std::cout<<"]}\n";
}
int main(int argc,char** argv){if(argc==2)evaluate(argv[1]);else{std::string line;while(std::getline(std::cin,line))evaluate(line);}}
