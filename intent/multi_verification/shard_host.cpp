#if INTENT_PART==1
#define Intent IntentLeft
#define IntentWeights IntentWeightsLeft
#define IntentConfig IntentConfigLeft
#else
#define Intent IntentRight
#define IntentWeights IntentWeightsRight
#define IntentConfig IntentConfigRight
#endif
#include "../uno_probe/src/intent_model.h"
#if INTENT_PART==1
extern "C" void left_scores(const uint16_t* tokens,float* out){Intent::Input in;Intent::decodeTokens(tokens,in);Intent::logits(in,out);}
#else
extern "C" void right_scores(const uint16_t* tokens,float* out){Intent::Input in;Intent::decodeTokens(tokens,in);Intent::logits(in,out);}
#endif
