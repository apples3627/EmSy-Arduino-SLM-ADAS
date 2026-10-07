#pragma once
#include <stdint.h>
#include <string.h>
#define _BV(b) (1U<<(b))
enum {DDD2=2,DDD3=3,PORTD2=2,PORTD3=3,PIND2=2,WGM01=1,CS00=0,CS01=1,OCIE0A=1,WGM12=3,CS11=1,OCIE1A=1,OCIE1B=2,ISC01=1,INTF0=0,INT0=0,OCF1B=2,UCSZ00=1,UCSZ01=2,RXEN0=4,TXEN0=3,RXCIE0=7,UDRIE0=5,FE0=4,DOR0=3,UPE0=2,DDB2=2,DDB3=3,DDB4=4,DDB5=5,PORTB2=2,SPE=6,MSTR=4,SPR1=1,SPR0=0,SPIF=7,TWINT=7,TWEN=2,TWIE=0,TWSTA=5,TWSTO=4,TWEA=6,EEPE=1,EEMPE=2,EERE=0};
inline volatile uint8_t SREG,DDRD,PORTD,PIND,TCCR0A,TCCR0B,OCR0A,TIMSK0,TCNT0,TCCR1A,TCCR1B,TIMSK1,EICRA,EIFR,EIMSK,TIFR1,UCSR0A,UCSR0B,UCSR0C,UDR0,DDRB,PORTB,SPCR,SPSR,SPDR,TWBR,TWSR,TWCR,TWDR,EEDR;
inline volatile uint16_t OCR1A,OCR1B,TCNT1,UBRR0,EEAR;
inline uint8_t fakeEeprom[1024];
struct EepromControl {uint8_t value=0;operator uint8_t()const{return value;}EepromControl& operator|=(uint8_t v){value|=v;if(v&_BV(EERE)){EEDR=fakeEeprom[EEAR];value&=~_BV(EERE);}if(v&_BV(EEPE)){fakeEeprom[EEAR]=EEDR;value&=~(_BV(EEPE)|_BV(EEMPE));}return *this;}};
inline EepromControl EECR;
#define __builtin_avr_delay_cycles(n) ((void)(n))
