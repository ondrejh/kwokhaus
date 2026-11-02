#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

uint32_t v2pwm(uint16_t voltage);
uint32_t light_curve(uint32_t level, uint32_t pwm_max);

#endif // __UTILS_H__