#include <cassert>
#include <cstdio>
#include "../adas_i2c/src/ownengine/adapters/capture.h"
int main(){
 Capture::setup();assert((TCCR1B&_BV(ICES1))&&(TCCR1B&_BV(ICNC1)));assert(TIMSK1==(_BV(TOIE1)|_BV(ICIE1)));
 PINB=_BV(PINB0);assert(!Capture::arm());assert(Capture::phase==0);PINB=0;TCNT1=65530;assert(Capture::arm());assert(OCR1A==14);assert(PORTD&_BV(PORTD7));TIMER1_COMPA_vect();assert(!(PORTD&_BV(PORTD7))&&!(TIMSK1&_BV(OCIE1A)));
 // Capture just before overflow with overflow pending uses prior high word.
 Capture::wraps=10;TIFR1=_BV(TOV1);assert(Capture::stamp(65530)==((10UL<<16)|65530));assert(Capture::stamp(3)==((11UL<<16)|3));
 TIFR1=0;ICR1=65530;TIMER1_CAPT_vect();assert(Capture::phase==2&&!(TCCR1B&_BV(ICES1)));
 TIMER1_OVF_vect();ICR1=2314;TIMER1_CAPT_vect();uint32_t width;assert(Capture::take(width)&&width==2320);assert(!Capture::take(width));assert(Capture::phase==0);
 // Timestamp wrap remains an unsigned width calculation.
 Capture::wraps=65535;assert(Capture::arm());TIFR1=0;ICR1=65530;TIMER1_CAPT_vect();TIMER1_OVF_vect();ICR1=2314;TIMER1_CAPT_vect();assert(Capture::take(width)&&width==2320);
 // Missing rising and missing falling cancel without generating a sample.
 assert(Capture::arm());Capture::cancel();assert(!Capture::take(width)&&Capture::phase==0);assert(Capture::arm());TIFR1=0;ICR1=100;TIMER1_CAPT_vect();assert(Capture::phase==2);Capture::cancel();assert(!Capture::take(width));
 puts("PASS hardware capture edge selection, overflow race, timestamp wrap, trigger end, missing-edge cancellation");
}
