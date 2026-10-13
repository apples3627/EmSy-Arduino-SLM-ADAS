#pragma once
#include <stdint.h>
namespace MainConfig {
static const bool INVERT_MOTOR_A=false,INVERT_MOTOR_B=false;
static const uint8_t MOTOR_DUTY=75;
// ENA D9/OC1A, ENB D10/OC1B. SPI CS is D8/PB0; hardware SS D10 stays OUTPUT.
// IN1 D4, IN2 D7, IN3 A0, IN4 A1. ALERT D6/PCINT22. Soft UART RXD2/TXD3.
}
