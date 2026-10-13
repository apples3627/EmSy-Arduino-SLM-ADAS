#pragma once
#define cli() (SREG &= uint8_t(~0x80))
#define sei() (SREG |= 0x80)
#define ISR(v) void v()
