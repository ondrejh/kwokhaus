#ifndef __LIGHT_H__
#define __LIGHT_H__

#include <stdint.h>
#include <stdbool.h>

#define LIGHT_UPDATE_PERIOD 40 // ms (dimming step period)

void light_init(void);
void light_set(bool on, uint32_t dimming);
bool light_is_on(void);
void light_update(uint32_t now, uint32_t pwm_max);

#endif // __LIGHT_H__
