#include <cstdio>
#ifndef ADAS_CAPTURE_HEADER
#define ADAS_CAPTURE_HEADER "../../adas_i2c/src/ownengine/adapters/capture.h"
#endif
#include ADAS_CAPTURE_HEADER
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){using namespace Capture;TIFR1=0;wraps=0;phase=1;ICR1=65500;TIMER1_CAPT_vect();CHECK(phase==2&&rise==65500);TIMER1_OVF_vect();ICR1=2284;TIMER1_CAPT_vect();uint32_t result;CHECK(take(result)&&result==2320);CHECK(!take(result));
 wraps=3;TIFR1=_BV(TOV1);CHECK(stamp(100)==(4UL<<16)+100);CHECK(stamp(65000)==(3UL<<16)+65000);TIFR1=0;
 phase=0;event=0;ICR1=10;TIMER1_CAPT_vect();CHECK(!event);PINB=0;TCNT1=65530;CHECK(arm());CHECK(PORTD&_BV(PORTD7));CHECK(OCR1A==14);TIMER1_COMPA_vect();CHECK(!(PORTD&_BV(PORTD7)));PINB=_BV(PINB0);CHECK(!arm());CHECK(!phase);puts("PASS actual capture ISR: wrap, pending overflow, ignored unarmed edge, 10us trigger, stuck-high refusal");}
