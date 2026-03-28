#ifndef __CONFIG_H__
#define __CONFIG_H__

// Name will be used as comm identifier
#define DEV_NAME "KWAK"
#define LOCK_TIMEOUT 1000 // ms
#define STATUS_REPEAT_PERIOD 30 * 60 * 1000

//#define LIFE_LED

// GPIO
#define LED_GREEN_PIN 12
#define BUTTON_PIN 13
#define TRIGGER_PIN 14
#define LOCK_PIN 15
#define SENSE_ADC_PIN 26

// Voltage sense ADC
#define SENSE_VIN_PIN 26
#define SENSE_VIN_ADC 0

#define ADC_POLL_PERIOD 2
#define ADC_OVERSAMPLE 4

#define PWM_PERIOD 62500 // 5kHz
#define VOLT_FULL_PWR 13000 // mV

//#define TIME_POLLING_PERIOD 1000 // ms

#endif // __CONFIG_H__
