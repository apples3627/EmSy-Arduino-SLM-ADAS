#pragma once
#include <stdint.h>
#ifndef MOTOR_HAS_STBY
#define MOTOR_HAS_STBY 1
#endif
#ifndef ACTIVE_BUZZER
#define ACTIVE_BUZZER 0
#endif
namespace Actuator {
struct Config {
 uint8_t motorDuty; bool invertDirection; uint16_t motionMs, maxMotionMs, reversalMs;
 uint8_t servoMin, servoMax, servoStep; uint16_t servoStepMs, watchdogMs;
 Config():motorDuty(75),invertDirection(false),motionMs(500),maxMotionMs(1000),reversalMs(100),servoMin(45),servoMax(135),servoStep(3),servoStepMs(25),watchdogMs(2000){}
};
}
