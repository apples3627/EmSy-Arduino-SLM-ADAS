#pragma once
#include <stdint.h>
namespace AdasConfig {
static const uint8_t STOP_CM=20,RELEASE_CM=25,CLEAR_REQUIRED=3;
static const uint16_t PERIOD_MS=70,TIMEOUT_MS=30,STALE_MS=200;
static_assert(STOP_CM>0 && STOP_CM<RELEASE_CM,"invalid hysteresis thresholds");
static_assert(PERIOD_MS>60 && TIMEOUT_MS<PERIOD_MS && STALE_MS>PERIOD_MS && CLEAR_REQUIRED>0,"invalid sampling timing");
// Register enabled additional sources explicitly; disabled sources contribute nothing.
static const uint8_t SOURCE_CAPACITY=4;
}
