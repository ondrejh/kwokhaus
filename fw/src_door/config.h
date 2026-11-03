#ifndef __CONFIG_H__
#define __CONFIG_H__

// Default configuration (used when no valid config is stored in flash).
// Actual values are stored in nvdata (last flash sector, retained over FW update)
// and can be changed over USB: CFG?, NAME=<name>, BRPER=<seconds>
#define DEV_NAME "KWOK"
//#define DEV_NAME "KMUT" // second door for (meat) broiler chicken
//#define DEV_NAME "TEST"
//#define DEBUG

// Status timing
#define STATUS_REPEAT_PERIOD (30 * 60 * 1000) // default, config.brper used
#define STATUS_CHANGE_TIMEOUT (42 * 1000)
#define STATUS_REPEAT_PERIOD_MIN (5 * 1000)

// broadcast period limits for configuration [s]
#define CONF_BRPER_MIN_S 10
#define CONF_BRPER_MAX_S (24 * 60 * 60)

// GPIO
//#define FIRST_PROTOTYPE
#ifdef FIRST_PROTOTYPE

// pin configuration of first prototype
#warning "First prototype pin configuration"

#define BUTTON_LIGHT_PIN 11
#define BUTTON_L_PIN 14
#define BUTTON_R_PIN 15

#define ENABLE_R_PIN 8
#define ENABLE_L_PIN 7
#define PWM_R_PIN 6
#define PWM_L_PIN 5

#define LIGHT_PIN 12

#else // ifdef FIRST_PROTOTYPE

#define BUTTON_L_PIN 9
#define BUTTON_R_PIN 10
#define BUTTON_LIGHT_PIN 11

#define ENABLE_R_PIN 15
#define ENABLE_L_PIN 14
#define PWM_R_PIN 13
#define PWM_L_PIN 12

#define LIGHT_PIN 8

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
#define MOTOR_ERROR (1<<6) // position probe failed (no current in either direction)

#define MOTOR_SAFETY_TIMEOUT 30000 // ms (stop motor if it runs for too long without reaching end position)
#define MOTOR_CURRENT_TIMEOUT 400 // ms (stop motor if it runs without current - end switch is reached
#define MOTOR_CURRENT_OFFSET_TIMEOUT 100 // ms (calibrate current offset after motor start)
#define MOTOR_PROBE_DELAY 5000 // ms (probe real gate position this long after reset)
#define MOTOR_PROBE_TIME 500 // ms (max run time of each probe direction)
#define MOTOR_PROBE_CURRENT_MIN 100 // ms (current must last this long to count, filters start spikes/noise)
#define MOTOR_PROBE_PAUSE 300 // ms (pause between probe moves)
#define CURRENT1_MIN 0x150 // ADC value (current threshold for motor 1)
#define CURRENT2_MIN 0x040 // ADC value (current threshold for motor 2)

#define LIGHT_DIMMING 10
#define LIGHT_DIMMING_BTN 1000

#define LIGHT_AUTO_OFF 120000 // 2 min
#define LIGHT_DOOR_AUTO_OFF 60 * 5 * 1000 // 5 min

#endif // __CONFIG_H__
