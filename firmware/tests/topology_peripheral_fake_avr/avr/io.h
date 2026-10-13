#pragma once
#include <stdint.h>
#define _BV(n) (1U<<(n))
#define E2END 1023
#ifndef F_CPU
#define F_CPU 16000000UL
#endif
enum {PD2=2,PD3=3,PD4=4,PB2=2,DDB2=2,DDB3=3,DDB4=4,DDB5=5,WGM01=1,CS01=1,CS00=0,OCIE0A=1,WGM21=1,CS22=2,OCIE2A=1,ISC01=1,INTF0=0,INT0=0,SPE=6,SPIE=7,SPIF=7,PCINT2=2,PCIF0=0,PCIE0=0,TWINT=7,TWEN=2,TWSTA=5,TWSTO=4,EEPE=1,EEMPE=2,EERE=0};
inline volatile uint8_t SREG,DDRD,PORTD,PIND,PINB,TCCR0A,TCCR0B,OCR0A,TCNT0,TIMSK0,TCCR2A,TCCR2B,OCR2A,TCNT2,TIMSK2,EICRA,EIFR,EIMSK,DDRB,PORTB,SPCR,SPSR,SPDR,PCMSK0,PCIFR,PCICR,TWBR,TWSR,TWCR,TWDR,EEDR;
inline volatile uint16_t EEAR;
inline uint8_t fakeEeprom[1024];
struct EepromControl {uint8_t value=0;operator uint8_t()const{return value;}EepromControl& operator|=(uint8_t v){value|=v;if(v&_BV(EERE)){EEDR=fakeEeprom[EEAR];value&=~_BV(EERE);}if(v&_BV(EEPE)){fakeEeprom[EEAR]=EEDR;value&=~(_BV(EEPE)|_BV(EEMPE));}return *this;}};
inline EepromControl EECR;
