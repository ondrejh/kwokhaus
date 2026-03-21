#ifndef __CONFIG_H__
#define __CONFIG_H__

// GPIO
#define BUTTON_L_PIN 14
#define BUTTON_R_PIN 15

#define ENABLE_R_PIN 8
#define ENABLE_L_PIN 7
#define PWM_R_PIN 6
#define PWM_L_PIN 5

#define RGB_LED_PIN 16

#define SENSE_R_PIN 29
#define SENSE_R_ADC 3
#define SENSE_L_PIN 28
#define SENSE_L_ADC 2

#define SENSE_VIN_PIN 26
#define SENSE_VIN_ADC 0

#define ADC_POLL_PERIOD 2
#define ADC_OVERSAMPLE 8

#define PWM_PERIOD 62500 // 5kHz
#define VOLT_FULL_PWR 13000 // mV

#endif // __CONFIG_H__
