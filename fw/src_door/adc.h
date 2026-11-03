#ifndef __ADC_H__
#define __ADC_H__

#include <stdint.h>
#include <stdbool.h>

void adc_module_init(void);
uint16_t adc2u(int32_t adc);
bool adc_poll(uint32_t now, int32_t *adc);

#endif // __ADC_H__