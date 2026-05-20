#ifndef __BME280_H__
#define __BME280_H__

#include "includes.h"

extern bool env_valid;
extern int32_t env_temp, env_press, env_humi;

bool bme280_init(void);
bool bme280_read_raw(int32_t* temp, int32_t* pressure, int32_t* humidity);
int32_t bme280_convert_temp(int32_t adc_T);
int32_t bme280_convert_pressure(int32_t adc_P);
int32_t bme280_convert_humidity(int32_t adc_H);
bool env_poll(uint32_t now);

#endif // __BME280_H__