#include <cstdio>
#include <string>
#include <iostream>
#include "../tiny_uno/tiny_model.h"
int main(int argc,char** argv) {
  if(argc!=3) return 2;
  uint8_t ctx[MODEL_CONTEXT]; float h[MODEL_HIDDEN]; TinyModel::reset(ctx);
  std::string input=argv[2]; for(unsigned char c:input) TinyModel::push(ctx,TinyModel::encode((char)c));
  if(std::string(argv[1])=="logits") {
    TinyModel::hidden(ctx,h); for(int t=0;t<MODEL_VOCAB;t++) std::printf("%.9g\n",TinyModel::logit(h,t));
  } else {
    for(int i=0;i<80;i++) {uint8_t t=TinyModel::next(ctx,h);if(!t)break;std::cout<<TinyModel::decode(t);TinyModel::push(ctx,t);}
  }
}
