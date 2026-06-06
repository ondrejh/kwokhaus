#ifndef __CONFIG_H__
#define __CONFIG_H__

#define TESTING

// Name will be used as comm identifier
#ifdef TESTING
#define DEV_NAME "TST"
#define LIFE_LED
#define COMM_TIMEOUT 1000 // ms
#else // not TESTING (production)
#define DEV_NAME "KWAK"
#define COMM_TIMEOUT 100 // ms
#endif // TESTING
#define LOCK_TIMEOUT 1000 // ms
#define STATUS_REPEAT_PERIOD 30 * 60 * 1000
#define STATUS_REPEAT_MIN_PERIOD 30 * 1000

// GPIO
#define LED_GREEN_PIN 13
#define BUTTON_PIN 12
#define TRIGGER_PIN 14
#define LOCK_PIN 15
#define SENSE_ADC_PIN 26
#define PIR_INPUT_PIN 6

// Voltage sense ADC
#define SENSE_VIN_PIN 26
#define SENSE_VIN_ADC 0

#define ADC_POLL_PERIOD 2
#define ADC_OVERSAMPLE 4

#define PWM_PERIOD 62500 // 5kHz
#define VOLT_FULL_PWR 13000 // mV

#ifdef TESTING
#define PIR_OCUPY_TIMEOUT 1000 // 1s
#define PIR_RELEASE_TIMEOUT 3000 // 3s
#define LIGHT_CHANGE_SLOW 60 // 1min
#define LIGHT_CHANGE_FAST 3 // 3s
#define LIGHT_OFF_TIMEOUT 60 // 1min
#define ENVIRONMENT_PERIOD 5 // 5s
#else
#define PIR_OCUPY_TIMEOUT 10000 // 10s
#define PIR_RELEASE_TIMEOUT 30000 // 30s
#define LIGHT_CHANGE_SLOW 600 // 10min
#define LIGHT_CHANGE_FAST 10 // 10s
#define LIGHT_OFF_TIMEOUT 600 // 10min
#define ENVIRONMENT_PERIOD 600 // 10min
#endif
#define LIGHT_PWR_MAX 1000


//#define TIME_POLLING_PERIOD 1000 // ms

#endif // __CONFIG_H__
