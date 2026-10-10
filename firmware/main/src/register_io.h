#pragma once
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/pgmspace.h>
namespace MainIO {
volatile uint32_t ticks;
volatile uint8_t adasDanger=1,adasEvent=1;
inline bool adasLow(){return !(PIND&_BV(PIND4));}
inline uint32_t now(){uint8_t s=SREG;cli();uint32_t n=ticks;SREG=s;return n;}
volatile uint8_t pcRx[128], pcTx[256], pr, pw, tr, tw, pcOverflow;
volatile uint8_t rx[64],tx[64],rr,rw,xr,xw,softOverflow;
volatile uint8_t txBit=10,txByte,txFrameLeft,rxBit=255,rxByte;
inline bool pcWrite(char c){uint8_t n=tw+1;if(n==tr)return false;pcTx[tw]=c;tw=n;UCSR0B|=_BV(UDRIE0);return true;}
inline void say(const char* s){while(*s){if(!pcWrite(*s++))break;}}
inline void sayP(const char* s){char c;while((c=pgm_read_byte(s++))){if(!pcWrite(c))break;}}
inline void number(uint32_t v){char b[11];uint8_t n=0;do{b[n++]=char('0'+v%10);v/=10;}while(v);while(n)pcWrite(b[--n]);}
inline bool actorSend(const Proto::Frame& f){if(uint8_t((xr-xw-1)&63)<32)return false;for(uint8_t i=0;i<32;i++){tx[xw]=f.bytes[i];xw=(xw+1)&63;}return true;}
inline void actorFlush(){uint8_t s=SREG;cli();rr=rw;xw=(xr+txFrameLeft)&63;SREG=s;}
inline uint32_t readSession(){uint32_t v=0;for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){} EEAR=1020+i;EECR|=_BV(EERE);v|=uint32_t(EEDR)<<(i*8);}return v;}
inline bool persist(uint32_t v){for(uint8_t i=0;i<4;i++){while(EECR&_BV(EEPE)){} EEAR=1020+i;EEDR=uint8_t(v>>(i*8));uint8_t s=SREG;cli();EECR|=_BV(EEMPE);EECR|=_BV(EEPE);SREG=s;}return true;}
inline void setup(){cli();DDRD|=_BV(DDD3);PORTD|=_BV(PORTD3)|_BV(PORTD2);DDRD&=uint8_t(~(_BV(DDD2)|_BV(DDD4)));PORTD|=_BV(PORTD4);PCMSK2|=_BV(PCINT20);PCIFR=_BV(PCIF2);PCICR|=_BV(PCIE2);adasDanger=adasLow();TCCR0A=_BV(WGM01);TCCR0B=_BV(CS01)|_BV(CS00);OCR0A=249;TIMSK0=_BV(OCIE0A);TCNT0=0;TCCR1A=0;TCCR1B=_BV(WGM12)|_BV(CS11);OCR1A=207;TIMSK1=_BV(OCIE1A);EICRA=_BV(ISC01);EIFR=_BV(INTF0);EIMSK=_BV(INT0);UBRR0=103;UCSR0A=0;UCSR0C=_BV(UCSZ01)|_BV(UCSZ00);UCSR0B=_BV(RXEN0)|_BV(TXEN0)|_BV(RXCIE0);DDRB|=_BV(DDB2)|_BV(DDB3)|_BV(DDB5);DDRB&=~_BV(DDB4);PORTB|=_BV(PORTB2);SPCR=_BV(SPE)|_BV(MSTR)|_BV(SPR1)|_BV(SPR0);SPSR=0;TWBR=72;TWSR=0;TWCR=_BV(TWEN);sei();}
}
ISR(TIMER0_COMPA_vect){MainIO::ticks++;}
ISR(USART_RX_vect){using namespace MainIO;uint8_t flags=UCSR0A,b=UDR0,n=(pw+1)&127;if(flags&(_BV(FE0)|_BV(DOR0)|_BV(UPE0))){pcOverflow=1;return;}if(n==pr)pcOverflow=1;else{pcRx[pw]=b;pw=n;}}
ISR(USART_UDRE_vect){using namespace MainIO;if(tr==tw)UCSR0B&=~_BV(UDRIE0);else{UDR0=pcTx[tr];tr++;}}
ISR(TIMER1_COMPA_vect){using namespace MainIO;if(txBit==10){if(xr==xw)return;if(!txFrameLeft)txFrameLeft=32;txFrameLeft--;txByte=tx[xr];xr=(xr+1)&63;txBit=0;PORTD&=~_BV(PORTD3);}else if(txBit<8){if(txByte&(1<<txBit))PORTD|=_BV(PORTD3);else PORTD&=~_BV(PORTD3);txBit++;}else{PORTD|=_BV(PORTD3);txBit++;}}
// Timer1 CTC wraps each bit; COMPB is a fixed half-bit phase after INT0.
ISR(INT0_vect){using namespace MainIO;if(rxBit!=255)return;rxBit=0;rxByte=0;OCR1B=(TCNT1+104)%208;TIFR1=_BV(OCF1B);TIMSK1|=_BV(OCIE1B);EIMSK&=~_BV(INT0);}
ISR(TIMER1_COMPB_vect){using namespace MainIO;bool high=PIND&_BV(PIND2);if(rxBit==0){if(high){rxBit=255;}else rxBit=1;}else if(rxBit<=8){if(high)rxByte|=1<<(rxBit-1);rxBit++;}else{if(high){uint8_t n=(rw+1)&63;if(n==rr)softOverflow=1;else{rx[rw]=rxByte;rw=n;}}else softOverflow=1;rxBit=255;}if(rxBit==255){TIMSK1&=~_BV(OCIE1B);EIFR=_BV(INTF0);EIMSK|=_BV(INT0);}}

ISR(PCINT2_vect){if(MainIO::adasLow())MainIO::adasDanger=1;MainIO::adasEvent=1;}
