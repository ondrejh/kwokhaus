#ifndef __DISPLAY_H__
#define __DISPLAY_H__

uint8_t u8x8_byte_pico_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

void i2c_bus_scan(i2c_inst_t *port);

#endif // __DISPLAY_H__
