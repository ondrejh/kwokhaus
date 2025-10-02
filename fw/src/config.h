#ifndef __CONFIG_H__
#define __CONFIG_H__

// GPIO
#define LED_GREEN_PIN 12
#define BUTTON_PIN 13
#define TRIGGER_PIN 14
#define SENSE_ADC_PIN 26

// Display I2C pins and instance
#define DISP_I2C_SDA_PIN 4
#define DISP_I2C_SCL_PIN 5
#define DISP_I2C_PORT i2c0

// RTC I2C pins and instance
#define RTC_I2C_SDA_PIN 2
#define RTC_I2C_SCL_PIN 3
#define RTC_I2C_PORT i2c1

#endif // __CONFIG_H__
