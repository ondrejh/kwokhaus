#ifndef __CONFIG_H__
#define __CONFIG_H__

// GPIO
//#define FIRST_PROTOTYPE
#ifdef FIRST_PROTOTYPE

// pin configuration of first prototype
#warning "First prototype pin configuration"

#define BUTTON_L_PIN 14
#define BUTTON_R_PIN 15

#define ENABLE_R_PIN 8
#define ENABLE_L_PIN 7
#define PWM_R_PIN 6
#define PWM_L_PIN 5

#else // ifdef FIRST_PROTOTYPE

#define BUTTON_L_PIN 9
#define BUTTON_R_PIN 10

#define ENABLE_R_PIN 15
#define ENABLE_L_PIN 14
#define PWM_R_PIN 13
#define PWM_L_PIN 12

#endif // else FIRST_PROTOTYPE

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

#define MOTOR_GO_UP (1<<0)
#define MOTOR_GO_DOWN (1<<1)
#define MOTOR_RUNNING (MOTOR_GO_UP | MOTOR_GO_DOWN | MOTOR_FORCE_UP | MOTOR_FORCE_DOWN)
#define MOTOR_IS_UP (1<<2)
#define MOTOR_IS_DOWN (1<<3)
#define MOTOR_IS_END (MOTOR_IS_UP | MOTOR_IS_DOWN)
#define MOTOR_FORCE_UP (1<<4)
#define MOTOR_FORCE_DOWN (1<<5)
#define MOTOR_FORCE (MOTOR_FORCE_UP | MOTOR_FORCE_DOWN)

#define MOTOR_SAFETY_TIMEOUT 30000 // ms (stop motor if it runs for too long without reaching end position)
#define MOTOR_CURRENT_TIMEOUT 200 // ms (stop motor if it runs without current - end switch is reached
#define CURRENT_MIN 0x150 // no current adc value > 0x100, but we want to be safe

#endif // __CONFIG_H__
