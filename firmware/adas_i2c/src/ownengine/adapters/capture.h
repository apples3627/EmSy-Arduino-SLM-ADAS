#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
namespace Capture {
volatile uint16_t wraps=0;
volatile uint32_t rise=0,width=0;
volatile uint8_t phase=0,event=0;
// Pending overflow belongs to a low captured counter; high counters preceded it.
inline uint32_t stamp(uint16_t low){uint16_t hi=wraps;if((TIFR1&_BV(TOV1))&&low<0x8000)hi++;return (uint32_t(hi)<<16)|low;}
void setup(){DDRD|=_BV(DDD7);PORTD&=~_BV(PORTD7);DDRB&=~_BV(DDB0);PORTB&=~_BV(PORTB0);TCCR1A=0;TCCR1B=_BV(ICNC1)|_BV(ICES1)|_BV(CS11);TCNT1=0;TIFR1=_BV(TOV1)|_BV(ICF1)|_BV(OCF1A);TIMSK1=_BV(TOIE1)|_BV(ICIE1);}
bool arm(){uint8_t s=SREG;cli();if(PINB&_BV(PINB0)){phase=0;SREG=s;return false;}event=0;phase=1;TCCR1B|=_BV(ICES1);TIFR1=_BV(ICF1)|_BV(OCF1A);PORTD|=_BV(PORTD7);OCR1A=uint16_t(TCNT1+20);TIMSK1|=_BV(OCIE1A);SREG=s;return true;}
bool take(uint32_t& result){uint8_t s=SREG;cli();bool ready=event;if(ready){result=width;event=0;}SREG=s;return ready;}
void cancel(){uint8_t s=SREG;cli();phase=event=0;SREG=s;}
}
ISR(TIMER1_OVF_vect){Capture::wraps++;}
ISR(TIMER1_COMPA_vect){PORTD&=~_BV(PORTD7);TIMSK1&=~_BV(OCIE1A);}
ISR(TIMER1_CAPT_vect){using namespace Capture;uint16_t low=ICR1;uint32_t time=stamp(low);if(phase==1){rise=time;phase=2;TCCR1B&=~_BV(ICES1);}else if(phase==2){width=time-rise;event=1;phase=0;TCCR1B|=_BV(ICES1);}}
