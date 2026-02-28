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

// Display I2C pins and instance
#define DISP_I2C_SDA_PIN 4
#define DISP_I2C_SCL_PIN 5
#define DISP_I2C_PORT i2c0
#define DISP_I2C_ADDR 0x3C

// RTC I2C pins and instance
#define RTC_I2C_SDA_PIN 2
#define RTC_I2C_SCL_PIN 3
#define RTC_I2C_PORT i2c1
#define RTC_I2C_ADDR 0x68

//#define TIME_POLLING_PERIOD 1000 // ms

#endif // __CONFIG_H__
